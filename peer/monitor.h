//
// Created by Amir on 5/19/2016.
//

#ifndef DARTSYNC_MONITOR2_H
#define DARTSYNC_MONITOR2_H

#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <errno.h>
#include <sys/inotify.h>
#include "../common/constant.h"
#include "../common/pkt.h"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
#define USE_FDS 15  // depth of directory tree
#define MAX_SUBDIR_NUM 50

// data structure of file info
typedef struct file_info_t{
    char filepath[FILE_NAME_LEN]; // Path of the file, excluding the root sync folder
    unsigned long size; // size of the file
    unsigned long lastModifyTime; // time stamp of last modification
} FileInfo;

// data structure of directory node
typedef struct dir_tree_node_t{
    char dirpath[FILE_NAME_LEN];
    struct dir_tree_node_t *next;
}DirNode;

// data structure of directory tree. It is a linekd list in fact.
typedef struct dir_tree_t{
    DirNode *head;
    DirNode *tail;
}DirTree;

int watchDirectory(file_t *filetable, int conn);

int readConfigFile(char *filename);

int fileAdded(char* filename);

int fileModified(char* filename);

int fileDeleted(char* filename);

void print_filetable();

//FileList *getAllFilesInfo();

FileInfo *getFileInfo(char *filename);

/* these functions are used for constructing directory tree */
void addDirNode(DirTree *dirTree, const char *dirpath);

#endif //DARTSYNC_MONITOR2_H