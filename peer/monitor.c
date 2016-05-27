//
// Created by Amir on 5/19/2016.
//

#include <stdlib.h>
#include <unistd.h>
#include <ftw.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/inotify.h>
#include <strings.h>
#include <sys/dirent.h>

#include "../common/constant.h"
#include "monitor.h"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

#ifndef USE_FDS
#define USE_FDS 15
#endif

char *DIR_PATH;

PathList *pathList;

FileList *fileList;

//Node *localNode;

file_t *tableNode;

char current_path[200];

void substring(char s[], char sub[], size_t p, size_t l) {
    size_t c = 0;

    while (c < l) {
        sub[c] = s[p + c - 1];
        c++;
    }
    sub[c] = '\0';

    printf("%s\n", sub);
}

int endsWith(const char *str, const char *suffix) {
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix > lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

int readConfigFile(char *filename) {
    char cfg[200];
    FILE *file = fopen(filename, "rb");

    if (file) {
        if (fgets(cfg, 200, file) == NULL) {
            fclose(file);
            return 0;
        }
        fclose(file);
    }
    else {
        return 0;
    }

    if (strlen(cfg) == 0)
        return 0;

    struct stat st;
    int err = stat(cfg, &st);

    if (err == -1) {

        if (ENOENT == errno) {
            printf("%s does not exist.\n", cfg);
        } else {
            perror("stat");
            return 0;
        }
    }
    else {
        if (S_ISDIR(st.st_mode)) {
            size_t urlLen = strlen(cfg);
            if (cfg[urlLen - 1] != '/')
                strcat(cfg, "/");

            strcpy(current_path, cfg);
            return 1;
        }
    }
    return 0;
}

int print_entry(const char *filepath, const struct stat *info,
                const int typeflag, struct FTW *pathinfo) {
    if (typeflag == FTW_SL) {
        char *target;
        size_t maxlen = 1023;
        ssize_t len;

        while (1) {

            target = malloc(maxlen + 1);
            if (target == NULL)
                return ENOMEM;

            len = readlink(filepath, target, maxlen);
            if (len == (ssize_t) -1) {
                const int saved_errno = errno;
                free(target);
                return saved_errno;
            }
            if (len >= (ssize_t) maxlen) {
                free(target);
                maxlen += 1024;
                continue;
            }

            target[len] = '\0';
            break;
        }

        free(target);

    }
    if (typeflag == FTW_D || typeflag == FTW_DP)
        appendPath(pathList, strdup(filepath));

    return 0;
}


int print_directory_tree(const char *const dirpath) {
    int result;

    if (dirpath == NULL || *dirpath == '\0')
        return errno = EINVAL;

    result = nftw(dirpath, print_entry, USE_FDS, FTW_PHYS);
    if (result >= 0)
        errno = result;

    return errno;
}

Node *lookupNode(char *filename) {

    char abspath[100];
    strcpy(abspath, current_path);
    strcat(abspath, filename);

    Node *myNode = NULL;
    if (tableNode->head != NULL) {
        PointerNode *pointerNode = tableNode->head;
        while (pointerNode) {

            myNode = pointerNode->node;
            char *name = myNode->name;
            char *fullpath = myNode->absolutePath;

            if (strcmp(name, filename) == 0 && strcmp(abspath, fullpath) == 0)
                return myNode;

            pointerNode = pointerNode->next;
        }
    }

    return myNode;
}

void appendPath(PathList *pathList, char *path) {
    struct DirPathNode *node = (struct DirPathNode *) malloc(sizeof *node);
    node->path = path;
    node->next = NULL;

    if (pathList->tail == NULL) {
        pathList->tail = node;
        pathList->head = node;
        node->next = NULL;
        node->prev = NULL;
        dirNum++;
        return;
    }
    node->prev = pathList->tail;
    node->next = NULL;
    pathList->tail->next = node;
    pathList->tail = node;
    dirNum++;
}

char *PopPathList(PathList *pathList) {
    char *path = NULL;
    if (pathList->head != NULL) {
        DirPathNode *node = pathList->head;
        if (pathList->head->next != NULL) {
            pathList->head = pathList->head->next;
            pathList->head->prev = NULL;
        }
        else {
            pathList->head = NULL;
            pathList->tail = NULL;
        }
        path = node->path;
        dirNum--;
    }
    return path;
}

int dirNumbers() {
    return dirNum;
}

int fileAdded(char *filename) {

    char abspath[100];
    strcpy(abspath, current_path);
    strcat(abspath, filename);

    struct stat statbuf;
    Node *node = malloc(sizeof(Node));
    node->name = filename;
    node->absolutePath = abspath;
    node->type = FILE_CREATE;

    if (stat(abspath, &statbuf) != -1) {
        long fileSize = statbuf.st_size;
        node->size = fileSize;
        node->timestamp = (unsigned long) (statbuf.st_mtime);
        node->status = IS_FILE;
    }

    struct PointerNode *pointerNode = (struct PointerNode *) malloc(sizeof *pointerNode);
    pointerNode->node = node;
    pointerNode->next = NULL;

    if (tableNode->tail == NULL) {
        tableNode->tail = pointerNode;
        tableNode->head = pointerNode;
        pointerNode->next = NULL;
        pointerNode->prev = NULL;
        return 0;
    }
    pointerNode->prev = tableNode->tail;
    pointerNode->next = NULL;
    tableNode->tail->next = pointerNode;
    tableNode->tail = pointerNode;

    return 1;
}

FileInfo *getFileInfo(char *filename) {
    if (filename == NULL || strlen(filename) == 0 || current_path == NULL || strlen(current_path) == 0)
        return NULL;

    char abspath[100];
    strcpy(abspath, current_path);
    strcat(abspath, filename);

    FileInfo *fileInfo = NULL;

    struct stat statbuf;
    if (stat(abspath, &statbuf) != -1) {
        fileInfo = malloc(sizeof(FileInfo));
        long fileSize = statbuf.st_size;

        fileInfo->size = fileSize;
        fileInfo->timestamp = (unsigned long int) (statbuf.st_mtime);

        fileInfo->filepath, abspath;
    }

    return fileInfo;
}

void addFileToList(FileList *fileList, FileInfo *fileInfo) {
    struct FileInfoNode *node = (struct FileInfoNode *) malloc(sizeof *node);
    node->info = fileInfo;
    node->next = NULL;

    if (fileList->tail == NULL) {
        fileList->tail = node;
        fileList->head = node;
        node->next = NULL;
        node->prev = NULL;
        return;
    }
    node->prev = fileList->tail;
    node->next = NULL;
    fileList->tail->next = node;
    fileList->tail = node;

}

FileList *getAllFilesInfo() {
    fileList = malloc(sizeof(FileList));
    DIR *dir;

    dir = opendir(current_path);
    if (dir == NULL)
        return NULL;

    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL) {
        char *fileName = ent->d_name;

        FileInfo *fileInfo = getFileInfo(fileName);
        if (fileInfo == NULL)
            continue;

        addFileToList(fileList, fileInfo);
    }

    return fileList;
}

int fileModified(char *filename) {

    Node *myNode = lookupNode(filename);
    if (myNode == NULL)
        return 0;

    FileInfo *fileInfo = getFileInfo(filename);
    if (fileInfo == NULL)
        return 0;

    myNode->timestamp = fileInfo->timestamp;
    myNode->size = fileInfo->size;

    char abspath[100];
    strcpy(abspath, current_path);
    strcat(abspath, filename);

    Node *tempNode = NULL;
    if (tableNode->head != NULL) {
        PointerNode *pointerNode = tableNode->head;
        while (pointerNode) {

            tempNode = pointerNode->node;
            char *name = tempNode->name;
            char *fullpath = tempNode->absolutePath;

            if (strcmp(name, filename) == 0 && strcmp(abspath, fullpath) == 0) {
                pointerNode->node = myNode;
                return 1;
            }
            pointerNode = pointerNode->next;
        }
    }
    return 0;
}

int fileDeleted(char *filename) {

    char abspath[100];
    strcpy(abspath, current_path);
    strcat(abspath, filename);

    Node *myNode = NULL;
    if (tableNode->head != NULL) {
        PointerNode *pointerNode = tableNode->head;
        while (pointerNode->next) {

            myNode = pointerNode->node;
            char *name = myNode->name;
            char *fullpath = myNode->absolutePath;

            if (strcmp(name, filename) == 0 && strcmp(abspath, fullpath) == 0) {
                if (tableNode->head->next != NULL) {
                    tableNode->head = tableNode->head->next;
                    tableNode->head->prev = NULL;
                    return 1;
                }
                else {
                    tableNode->head = NULL;
                    tableNode->tail = NULL;
                    return 0;
                }
            }

            pointerNode = pointerNode->next;
        }
        if (pointerNode->node != NULL) {
            myNode = pointerNode->node;
            char *name = myNode->name;
            char *fullpath = myNode->absolutePath;

            if (strcmp(name, filename) == 0 && strcmp(abspath, fullpath) == 0) {
                if (tableNode->head->next != NULL) {
                    tableNode->head = tableNode->head->next;
                    tableNode->head->prev = NULL;
                    return 1;
                }
                else {
                    tableNode->head = NULL;
                    tableNode->tail = NULL;
                    return 0;
                }
            }
        }
    }
    return 0;
}

int notify(PathList *pathList) {
    int length, i = 0;
    int fd;
    int wd[dirNum + 1];
    char buffer[BUF_LEN];

    char *folderArray[dirNum];

    fd = inotify_init();

    if (fd < 0) {
        perror("inotify_init");
    }

    char *path;

    int count = 0;
    while ((path = PopPathList(pathList)) != NULL) {
        printf("%s\n", path);
        folderArray[count] = path;
        wd[count++] = inotify_add_watch(fd, path,
                                        IN_OPEN | IN_MODIFY | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVED_TO |
                                        IN_MOVED_FROM);
    }


    while (1) {
        struct inotify_event *event;

        length = read(fd, buffer, BUF_LEN);

        if (length < 0) {
            perror("read");
        }

        event = (struct inotify_event *) &buffer[i];

        if (event->len) {

            char abspath[200];
            sprintf(abspath, "%s/%s", folderArray[(event->wd) - 1], event->name);
            struct stat statbuf;

            if (event->mask & IN_CREATE) {
                char *eventName = event->name;
                //     if (!(localNode->type == FILE_OPEN && strcmp(eventName, localNode->name) == 0)) {
                if (strcmp(eventName, "4913") != 0 &&
                    !(eventName[0] == '.' && (endsWith(eventName, ".swp") || endsWith(eventName, ".swpx")))
                        ) {

                    if (event->mask & IN_ISDIR) {
                        printf("The directory %s was created.\n", event->name);
                    }
                    else {
                        if (stat(abspath, &statbuf) != -1) {
                            printf("The file %s was created.\n", event->name);
                            fileAdded(eventName);
                            if (strcmp(eventName, "ccc") == 0) {
                                Node *nd = lookupNode(eventName);
                                printf("name %s\n", nd->name);
                            }
                        }

                    }
                }
                //    }
            }
            if (event->mask & IN_OPEN) {
                char *eventName = event->name;

                if (event->mask & IN_ISDIR) {
                    printf("The directory %s was opened.\n", event->name);
                }
                else {
                    if (stat(abspath, &statbuf) != -1) {

                    }
                    printf("The file %s was opened.\n", event->name);
                }
            }
            else if (event->mask & IN_DELETE) {
                char *eventName = event->name;
                size_t l = strlen(eventName);
                if (eventName[l - 1] != '~' &&
                    strcmp(eventName, "4913") != 0 &&
                    !(eventName[0] == '.' && (endsWith(eventName, ".swp") ||
                                              endsWith(eventName, ".swpx")))) {

                    if (event->mask & IN_ISDIR) {
                        printf("The directory %s was deleted.\n", event->name);
                    }
                    else {
                        if (stat(abspath, &statbuf) != -1) {
                            fileDeleted(eventName);
                        }
                        printf("The file %s was deleted.\n", event->name);
                    }
                }
                else if (eventName[l - 1] == '~') {

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
                            fileModified(eventName);
                        }
                        printf("The file %s was modified.\n", sub);
                    }
                }
            }

            else if (event->mask & IN_MODIFY) {

                char *eventName = event->name;
                size_t l = strlen(eventName);

                if (eventName[l - 1] != '~' &&
                    !(eventName[0] == '.' && (endsWith(eventName, ".swp") || endsWith(eventName, ".swpx")))) {

                    if (event->mask & IN_ISDIR) {
                        printf("The directory %s modified.\n", event->name);
                    }
                    else {
                        if (stat(abspath, &statbuf) != -1) {
                            fileModified(eventName);
                        }
                        printf("The file %s modified.\n", event->name);
                    }
                }
            }
//        i += EVENT_SIZE + event->len;
        }
    }

    int j;
    for (j = 0; j < dirNum; j++) {
        (void) inotify_rm_watch(fd, wd[j]);
    }

    (void) close(fd);
    exit(0);
}

int watchDirectory(char *directory, struct file_table *table) {
    tableNode = table;

    dirNum = 0;

    fileList = malloc(sizeof(FileList));
    
    readConfigFile(directory);

    pathList = malloc(sizeof(PathList));

    print_directory_tree(current_path);

    printf("\ncount = %d", dirNumbers());

    notify(pathList);
    return 1;
}
