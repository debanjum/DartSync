//
// Created by Amir on 5/19/2016.
//

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common/constant.h"
#include "common/list.h"
#include "monitor.h"


int watchDirectory(char *directory) {
//    TODO
    return -1;
}

char *readConfigFile(char *filename) {
    char content[999];
    FILE * file;
    file = fopen(file , "r");
    if (file) {
        while (fscanf(file, "%s", content)!=EOF)
            printf("%s",content);
        fclose(file);
        return content;
    }
    return NULL;
}

int fileAdded(char *filename) {
//    TODO
    return -1;
}

int fileModified(char *filename) {
//    TODO
    return -1;
}

int fileDeleted(char *filename) {
    if (remove(filename) != 0) {
        perror("Error deleting file");
        return 0;
    }

    puts("%s successfully deleted", filename);
    return 1;
}

FileList *getAllFilesInfo() {
    FileList *fileList = malloc(sizeof(FileList));
    DIR *dir;

    dir = opendir(DIR_PATH);
    if (dir == NULL)
        return NULL;

    int nodeId = 1;
    long fileSize;
    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL) {
        FileInfo fileInfo = malloc(sizeof(FileInfo));

        fseek(ent, 0L, SEEK_END);
        fileSize = ftell(ent);

        fileInfo.size = fileSize;
//        TODO
//        fileInfo.lastModifyTime = ;

        AppendList(fileInfo, fileList);
    }

    return fileList;
}