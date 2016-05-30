//
//  common.h
//  peer
//
//  Created by Lexin Tang on 5/18/16.
//  Copyright © 2016 Lexin Tang. All rights reserved.
//

#ifndef common_h
#define common_h

#define _DEFAULT_SOURCE              // refer: https://stackoverflow.com/questions/3355298/unistd-h-and-c99-on-linux + warning message

#include "stdio.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PROTOCOL_LEN 100      // protocol name
#define RESERVED_LEN 512
#define IP_LEN 16
#define FILE_NAME_LEN 256
#define MAX_DATA_LEN 1464

// Packet Type
#define REGISTER 0
#define KEEP_ALIVE 1
#define FILE_UPDATE 2

// Node Type: File Created, Updated, or Deleted
#define FILE_CREATE 3
#define FILE_MODIFY 4
#define FILE_DELETE 5
#define FILE_OPEN 6
#define FILE_OPEN_MODIFY 7 // opened in edit mode

#define IS_FILE 1
#define IS_DIR 2

#define MAX_PEER_NUM 5

#define HANDSHAKE_PORT 3078
#define P2P_PORT 3578

#define HEARTBEAT_INTERVAL 60   // Heartbeat Message Interval
#define HEARTBEAT_TIMEOUT 240   // Peer dead if Tracker doesn't hear its heartbeat not heard for this long
#define RECV_INTERVAL 1000000

#define TRACKER_IP "129.170.214.115"   // Tracker IP[pi: 73.16.78.254], [flume: 129.170.214.115]

// each file can be represented as a node in file table
typedef struct node{
    // is file or directory
    short status;
    //the size of the file
    long size;
    //the name of the file
    char name[FILE_NAME_LEN];
    //the timestamp when the file is modified or created
    unsigned long int timestamp;
    //the type of file update
    int type;
    //pointer to build the linked list
    struct node *pNext;
    //for the file table on peers, it is the ip address of the peer
    //for the file table on tracker, it records the ip of all peers which has the
    //newest edition of the file
    char newpeerip[MAX_PEER_NUM][IP_LEN+1];
} Node;

// file table is a linked list of Node
typedef struct file_table {
    Node * head;
} file_t;

#endif /* common_h */
