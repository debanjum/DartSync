#include "monitor.h"

char sync_dir[FILE_NAME_LEN];   // sync_dir
DirTree *dir_tree;
int dirNum;
file_t *filetable;
int conn;

// this function is used to detect if str is ended with suffix
int endsWith(const char *str, const char *suffix) {
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix > lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

// read sync_dir from config file
int readConfigFile(char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL){
        printf("cannot open config file %s!\n", filename);
        return -1;
    }
    
    char line[FILE_NAME_LEN];
    fgets(line, sizeof(line), fp);
    
    // check the tail of dir
    if (line[strlen(line) - 1] != '/'){
        strcat(line, "/");
    }
    strcpy(sync_dir, line);
    printf("sync_dir = %s read from config.data\n", sync_dir);
    
    // check if the dir exists, if not, create a new folder
    struct stat st = {0};
    if (stat(line, &st) == -1) {
        printf("%s does not exist!\n", line);
        printf("creating a new folder %s...\n", line);
        mkdir(line, 0700);
    }
    else{
        printf("%s exists.\n", line);
    }
    
    return 0;
}

// print out dir tree
void printDirTree(DirTree *dirTree) {
    printf("number of nodes in dir tree = %d\n", dirNum);
    DirNode *node = dirTree->head;
    int cnt = 1;
    while (node != NULL) {
        printf("%d node in dir tree: %s\n", cnt, node->dirpath);
        node = node->next;
        cnt++;
    }
}

// add a subdir to dir tree
void addDirNode(DirTree *dirTree, const char *dirpath) {
    DirNode *node = (DirNode *)malloc(sizeof(DirNode));
    strcpy(node->dirpath, dirpath);
    strcat(node->dirpath, "/");
    node->next = NULL;
    
    if (dirNum == 0){   // dirTree is empty
        dir_tree->head = node;
        dir_tree->tail = node;
    }
    else{
        dirTree->tail->next = node;
        dirTree->tail = node;
    }
    //printf("%d node in dir tree : %s\n", dirNum, node->dirpath);
    dirNum++;
    printDirTree(dirTree);
}

// delete a subdir from dir tree
int deleteDirNode(DirTree *dirTree, const char *dirpath) {
    // look up this dirpath in dirTree
    DirNode *prev = NULL;
    DirNode *node = dirTree->head;
    
    while (node != NULL){
        if (strstr(node->dirpath, dirpath) != NULL){
            prev->next = node->next;
            free(node);
            dirNum--;
            if (node == dirTree->tail){ // tail is deleted
                dirTree->tail = prev;
            }
            printDirTree(dirTree);
            return 0;
        }
        prev = node;
        node = node->next;
    }
    return -1;
}

// print each subdirectory
int print_entry(const char *filepath, const struct stat *info,
                const int typeflag, struct FTW *pathinfo) {
    // filepath is a directory, add it to the directory tree
    if (typeflag == FTW_D || typeflag == FTW_DP){
        addDirNode(dir_tree, filepath);
    }
    return 0;
}

// this function is used for recursively traversing directories.
int print_directory_tree(const char *dirpath) {
    // initialize directory tree
    dir_tree = (DirTree *) malloc(sizeof(DirTree));
    dirNum = 0;
    
    // file tree walk
    int result;
    result = nftw(dirpath, print_entry, USE_FDS, FTW_PHYS);
    if (result >= 0)
        errno = result;
    
    return errno;
}

// this function is used to extract file attributes: size and timestamp
FileInfo *getFileInfo(char *filepath) {
    if (filepath == NULL || strlen(filepath) == 0)
        return NULL;
    
    FileInfo *fileInfo = NULL;
    struct stat statbuf;
    if (stat(filepath, &statbuf) != -1) {
        fileInfo = (FileInfo *)malloc(sizeof(FileInfo));
        long fileSize = statbuf.st_size;
        
        fileInfo->size = fileSize;
        fileInfo->lastModifyTime = (unsigned long int) (statbuf.st_mtime);
        
        strcpy(fileInfo->filepath, &filepath[strlen(sync_dir)]); //filepath excluding the root sync folder
        //printf("fileInfo->filepath = %s\n", fileInfo->filepath);
        //printf("fileInfo->size = %lu\n", fileInfo->size);
        //printf("fileInfo->lastModifyTime = %lu\n", fileInfo->lastModifyTime);
    }
    
    return fileInfo;
}

int fileAdded(char *filepath) {
    FileInfo *fileInfo = getFileInfo(filepath);
    Node *node = filetable->head;
    Node *prev = NULL;
    
    // loop up this node in filetable
    while (node != NULL){
        if (strcmp(node->name, fileInfo->filepath) == 0){
            printf("%s already exists in filetable!\n", node->name);
            print_filetable();
            return 0;
        }
        prev = node;
        node = node->pNext;
    }
    
    // add this node to filetable
    Node *filenode = (Node *)malloc(sizeof(Node));
    filenode->status = IS_FILE;
    strcpy(filenode->name, fileInfo->filepath);
    filenode->size = fileInfo->size;
    filenode->timestamp = fileInfo->lastModifyTime;
    filenode->pNext = NULL;
    for (int i = 0; i < MAX_PEER_NUM; i++){
        memset(filenode->newpeerip[i], 0, sizeof(IP_LEN +1));
    }
    strcpy(filenode->newpeerip[0], getmyip());  // only store the peer's ip
    
    if (prev == NULL){
        filetable->head = filenode;
    }
    else {
        prev->pNext = filenode;
    }
    
    print_filetable();
    peer_sendpkt(conn, filetable, FILE_UPDATE);
    return 0;
}

int fileDeleted(char *filepath) {
    FileInfo *fileInfo = getFileInfo(filepath);
    Node *node = filetable->head;
    Node *prev = NULL;
    
    // loop up this node in filetable
    while (node != NULL){
        if (strcmp(node->name, fileInfo->filepath) == 0){
            if (filetable->head == node) {
                filetable->head = node->pNext;
            }
            else {
                prev->pNext = node->pNext;
            }
            free(node);
            print_filetable();
            peer_sendpkt(conn, filetable, FILE_UPDATE);
            return 0;
        }
        prev = node;
        node = node->pNext;
    }
    print_filetable();
    printf("cannot find %s in filetable\n", fileInfo->filepath);
    return -1;
}

int fileModified(char *filepath) {
    FileInfo *fileInfo = getFileInfo(filepath);
    Node *node = filetable->head;
    
    // loop up this node in filetable
    while (node != NULL){
        if (strcmp(node->name, fileInfo->filepath) == 0) {
            node->size = fileInfo->size;
            node->timestamp = fileInfo->lastModifyTime;
            print_filetable();
            peer_sendpkt(conn, filetable, FILE_UPDATE);
            return 0;
        }
        node = node->pNext;
    }
    print_filetable();
    printf("cannot find %s in filetable\n", fileInfo->filepath);
    return -1;
}


void print_filetable(){
    Node *node = filetable->head;
    int cnt = 0;
    printf("printing filetable...\n");
    while (node != NULL){
        printf("%d node filename = %s\n", cnt, node->name);
        printf("%d node size = %lu\n", cnt, node->size);
        printf("%d node lastModifyTime = %lu\n", cnt, node->timestamp);
        printf("%d node peer ip = %s\n", cnt, node->newpeerip[0]);
        node = node->pNext;
        cnt++;
    }
}

int notify(DirTree *dirTree) {
    int length = 0;
    int fd;
    int wd[MAX_SUBDIR_NUM]; // max num of directories being watched
    char buffer[BUF_LEN];
    
    char *folderArray[MAX_SUBDIR_NUM];
    for (int i = 0; i < MAX_SUBDIR_NUM; i++){
        folderArray[i] = NULL;
    }
    fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
    }
    
    // add all directories in dir tree to *inotify_add_watch*
    int count = 0;
    DirNode *dirNode = dirTree->head;
    char *path;
    while (dirNode != NULL) {
        path = dirNode->dirpath;
        printf("adding dir %s to inotify_add_watch()\n", path);
        folderArray[count] = path;
        wd[count++] = inotify_add_watch(fd, path,
                                        IN_OPEN | IN_MODIFY | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVED_TO |
                                        IN_MOVED_FROM);
        dirNode = dirNode->next;
    }
    
    // keep watching
    while (1) {
        struct inotify_event *event;
        
        length = read(fd, buffer, BUF_LEN);
        
        if (length < 0) {
            perror("read");
        }
        
        char *event_pnt;
        for (event_pnt = buffer; event_pnt < buffer + length;){
            event = (struct inotify_event *) event_pnt;
            
            if (event->len) {
                
                char abspath[FILE_NAME_LEN];
                sprintf(abspath, "%s%s", folderArray[(event->wd) - 1], event->name);
                //printf("absolute path of file %s is %s\n", event->name, abspath);
                struct stat statbuf;
                
                if (event->mask & IN_CREATE) {
                    char *eventName = event->name;
                    //     if (!(localNode->type == FILE_OPEN && strcmp(eventName, localNode->name) == 0)) {
                    if (strcmp(eventName, "4913") != 0 &&
                        !(eventName[0] == '.' )) { // filter out all hiden files
                        
                        if (event->mask & IN_ISDIR) {
                            printf("The directory %s was created.\n", event->name);
                            // add this new directory to dir tree
                            addDirNode(dirTree, abspath);
                            printf("adding dir %s to inotify_add_watch()\n", abspath);
                            folderArray[count] = abspath;
                            wd[count++] = inotify_add_watch(fd, abspath,
                                                            IN_OPEN | IN_MODIFY | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVED_TO |
                                                            IN_MOVED_FROM);
                            
                        }
                        else {
                            if (stat(abspath, &statbuf) != -1) {
                                printf("The file %s was created.\n", event->name);
                                fileAdded(abspath); // when add a file, should firstly check whether it exists
                                if (strcmp(eventName, "ccc") == 0) {
                                    //Node *nd = lookupNode(eventName);
                                    printf("eventName = %s\n", eventName);
                                    //printf("name %s\n", nd->name);
                                }
                            }
                        }
                    }
                    //    }
                }
                
                else if (event->mask & IN_OPEN) {
                    char *eventName = event->name;
                    if (strcmp(eventName, "4913") != 0 &&
                        !(eventName[0] == '.')) { // filter out all hiden files
                        
                        if (event->mask & IN_ISDIR) {
                            //printf("The directory %s was opened.\n", event->name);
                        }
                        else {
                            if (stat(abspath, &statbuf) != -1) {
                                printf("The file %s was opened.\n", event->name);
                            }
                        }
                    }
                }
                
                else if (event->mask & IN_DELETE) {
                    char *eventName = event->name;
                    size_t l = strlen(eventName);
                    if (eventName[l - 1] != '~' &&
                        strcmp(eventName, "4913") != 0 &&
                        !(eventName[0] == '.')) {
                        
                        if (event->mask & IN_ISDIR) {
                            printf("The directory %s was deleted.\n", event->name);
                            
                            // remove this directory from dir tree
                            deleteDirNode(dirTree, abspath);
                            
                            // update folderArray and remove this directory from watching
                            for (int i = 0; i < MAX_SUBDIR_NUM; i++){
                                if (folderArray[i] == NULL){
                                    continue;
                                }
                                if (strstr(folderArray[i], abspath) != NULL){
                                    printf("wd index = %d\n", i);
                                    (void) inotify_rm_watch(fd, wd[i]);
                                    folderArray[i] = NULL;
                                    break;
                                }
                            }
                        }
                        else {
                            if (stat(abspath, &statbuf) != -1) {
                                fileDeleted(abspath);
                            }
                            printf("The file %s was deleted.\n", event->name);
                        }
                    }
                    else if (eventName[l - 1] == '~') {
                        //printf("delete deteted! %s is deleted\n", eventName);
                        
                        size_t sz = strlen(event->name);
                        char ptr[sz];
                        char sub[sz - 1];
                        strcpy(ptr, event->name);
                        
                        int j;
                        for (j = 0; j < (int) (sz - 1); j++)
                            sub[j] = ptr[j];
                        sub[sz - 1] = '\0';
                        
                        if (event->mask & IN_ISDIR) {
                            printf("The directory %s was modified.\n", sub);
                        }
                        else {
                            if (stat(abspath, &statbuf) != -1) {
                                //fileModified(abspath);
                            }
                            //printf("The file %s was modified.~2\n", sub);
                        }
                    }
                }
                
                else if (event->mask & IN_MODIFY) {
                    
                    char *eventName = event->name;
                    size_t l = strlen(eventName);
                    
                    if (eventName[l - 1] != '~' &&
                        !(eventName[0] == '.')) {
                        
                        if (event->mask & IN_ISDIR) {
                            printf("The directory %s modified.\n", event->name);
                        }
                        else {
                            if (stat(abspath, &statbuf) != -1) {
                                fileModified(abspath);
                                printf("The file %s was modified.~1\n", event->name);
                            }
                        }
                    }
                }
                
            }
            event_pnt += EVENT_SIZE + event->len;
        }
    }
    
    for (int j = 0; j < MAX_SUBDIR_NUM; j++) {
        if (folderArray[j] != NULL){
            (void) inotify_rm_watch(fd, wd[j]);
        }
    }
    
    (void) close(fd);
    exit(0);
}

int watchDirectory(file_t *file_table, int tracker_conn){
    print_directory_tree(sync_dir);
    printf("total number of directories = %d\n", dirNum);
    filetable = file_table;
    conn = tracker_conn;
    
    notify(dir_tree);
    
    return 0;
}
