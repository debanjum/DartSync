//
//  common.h
//  peer
//
//  Created by Lexin Tang on 5/18/16.
//  Copyright © 2016 Lexin Tang. All rights reserved.
//

#ifndef common_h
#define common_h

#include "stdio.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define PROTOCOL_LEN 100      // protocol name
#define RESERVED_LEN 512
#define IP_LEN 16
#define FILE_NAME_LEN 256

#define REGISTER 0
#define KEEP_ALIVE 1
#define FILE_UPDATE 2
#define ACCEPT 3

#define MAX_PEER_NUM 5

#define HANDSHAKE_PORT 3078
#define P2P_PORT 3578

#define HEARTBEAT_INTERVAL 60   // Heartbeat Message Interval
#define HEARTBEAT_TIMEOUT 240   // Peer dead if Tracker doesn't hear its heartbeat not heard for this long

#define TRACKER_IP "127.0.0.1"  // Tracker IP
#define WATCHING "./"

// each file can be represented as a node in file table
typedef struct node{
    //the size of the file
    long size;
    //the name of the file
    char *name;
    //the timestamp when the file is modified or created
    unsigned long int timestamp;
    //pointer to build the linked list
    struct node *pNext;
    //for the file table on peers, it is the ip address of the peer
    //for the file table on tracker, it records the ip of all peers which has the
    //newest edition of the file
    char *newpeerip;
} Node;

// file table is a linked list of Node
typedef struct file_table {
    Node * head;
} file_t;


#endif /* common_h */
