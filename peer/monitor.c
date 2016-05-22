//
// Created by Amir on 5/19/2016.
//

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <dirent.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../common/constant.h"
#include "../common/list.h"
#include "monitor.h"

char *DIR_PATH;

int watchDirectory(char *directory) {
//    TODO
    return -1;
}

char *readConfigFile(char *filename) {
    char *content = 0;
    long length;
    FILE *file = fopen(filename, "rb");

    if (file) {
        fseek(file, 0, SEEK_END);
        length = ftell(file);
        fseek(file, 0, SEEK_SET);
        content = malloc((size_t) length);
        if (content) {
            fread(content, 1, (size_t) length, file);
        }
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

FileInfo *getFileInfo(char *filename) {
    if (filename == NULL || strlen(filename) == 0 || DIR_PATH == NULL || strlen(DIR_PATH) == 0)
        return NULL;

    char *absolutePath = malloc(sizeof(filename) + 50);
    strcpy(absolutePath, DIR_PATH);
    size_t folderLen = strlen(absolutePath);
    if (absolutePath[folderLen - 1] != '/')
        strcat(absolutePath, "/");

    sprintf(absolutePath, "%s%s", absolutePath, filename);

    FileInfo *fileInfo = NULL;

    struct stat statbuf;
    if (stat(absolutePath, &statbuf) != -1) {
        fileInfo = malloc(sizeof(FileInfo));
        long fileSize = statbuf.st_size;

        fileInfo->size = fileSize;
        fileInfo->lastModifyTime = statbuf.st_mtime;

        strcpy(fileInfo->filepath, absolutePath);
    }

    free(absolutePath);
    return fileInfo;
}

FileInfo *getFileInfo(char *filename) {
    if (filename == NULL || strlen(filename) == 0 || DIR_PATH == NULL || strlen(DIR_PATH) == 0)
        return NULL;

    char *absolutePath = malloc(sizeof(filename) + 50);
    strcpy(absolutePath, DIR_PATH);
    size_t folderLen = strlen(absolutePath);
    if (absolutePath[folderLen - 1] != '/')
        strcat(absolutePath, "/");

    sprintf(absolutePath, "%s%s", absolutePath, filename);

    FileInfo *fileInfo = NULL;

    struct stat statbuf;
    if (stat(absolutePath, &statbuf) != -1) {
        fileInfo = malloc(sizeof(FileInfo));
        long fileSize = statbuf.st_size;

        fileInfo->size = fileSize;
        fileInfo->lastModifyTime = statbuf.st_mtime;

        strcpy(fileInfo->filepath, absolutePath);
    }

    free(absolutePath);
    return fileInfo;
}
