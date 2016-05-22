//
// Created by Amir on 5/19/2016.
//
#include <sys/time.h>
#include <time.h>

#ifndef DARTSYNC_MONITOR2_H
#define DARTSYNC_MONITOR2_H

typedef struct {
    char *filepath; // Path of the file
    long size; // Size of the file
    time_t  lastModifyTime; // time stamp of last modification
} FileInfo;

int watchDirectory( char* directory);

char *readConfigFile(char *filename);

int fileAdded(char* filename);

int fileModified(char* filename);

int fileDeleted(char* filename);

FileList *getAllFilesInfo();

FileInfo *getFileInfo(char *filename);

#endif //DARTSYNC_MONITOR2_H