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

int readConfigFile(char *filename) {
//TODO
    return -1;
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
//    TODO
    return -1;
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