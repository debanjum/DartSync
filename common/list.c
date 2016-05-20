/* ========================================================================== */
/* File: list.c
 *
 * Project name: CS60 DartSync
 *
 * Author: Amir H. Sharifzadeh
 * Date: May 19, 2016
 *
 */
/* ========================================================================== */

// ---------------- Open Issues

// ---------------- System includes e.g., <stdio.h>
#include <string.h>                          // strlen
#include <stdlib.h>

// ---------------- Local includes  e.g., "file.h"
#include "list.h"

// ---------------- Constant definitions

// ---------------- Macro definitions

// ---------------- Structures/Types

// ---------------- Private variables

// ---------------- Private prototypes


// append a URL to the end of the list
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

// pop from the list
FileInfo *PopList(FileList *fileList) {
    FileInfo *fileInfo = NULL;
    if (fileList->head != NULL) {
        FileInfoNode *node = fileList->head;
        if (fileList->head->next != NULL) {
            fileList->head = fileList->head->next;
            fileList->head->prev = NULL;
        }
        else {
            fileList->head = NULL;
            fileList->tail = NULL;
        }
        fileInfo = node->info;
    }
    return fileInfo;
}