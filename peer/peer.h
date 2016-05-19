//
//  peer.h
//  peer
//
//  Created by Lexin Tang on 5/18/16.
//  Copyright © 2016 Lexin Tang. All rights reserved.
//

#ifndef peer_h
#define peer_h

#include "stdio.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include "constant.h"

//each file can be represented as a node in file table
typedef struct node{
    //the size of the file
    int size;
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

/* The packet data structure sending from peer to tracker */
typedef struct segment_peer {
    // protocol length
    int protocol_len;
    // protocol name
    char protocol_name[PROTOCOL_LEN + 1];
    // packet type : register, keep alive, update file table
    int type;
    // reserved space, you could use this space for your convenient, 8 bytes by default
    char reserved[RESERVED_LEN];
    // the peer ip address sending this packet
    char peer_ip[IP_LEN];
    // listening port number in p2p
    int port;
    // the number of files in the local file table -- optional
    int file_table_size;
    // file table of the client -- your own design
    Node *file_table;
} ptp_peer_t;

/* Peer-side peer table */
typedef struct _peer_side_peer_t {
    // Remote peer IP address, 16 bytes.
    char ip[IP_LEN];
    //Current downloading file name.
    char file_name[FILE_NAME_LEN];
    //Timestamp of current downloading file.
    unsigned long file_time_stamp;
    //TCP connection to this remote peer.
    int sockfd;
    //Pointer to the next peer, linked list.
    struct _peer_side_peer_t *next;
} peer_peer_t;

// listen_to_peer thread keeps receiving data requests from other peers. It handles data requests by creating a P2P upload thread.
// listen_to_peer thread are started after the peer is registered.
void* listen_to_peer(void* arg);

// p2p_upload thread is started by listen_to_peer thread and responsible for uploading data to remote peer.
void* ptp_upload(void* arg);

// p2p_download thread is responsible for downloading data from remote peer.
void* ptp_download(void* arg);

// file_monitor thread monitors a local file directory
// it sends out updated file table to the tracker if any file changes in the local file directory.
void* file_monitor(void* arg);

// keep_alive thread sends out heartbeat messages to tracker periodically.
void* keep_alive(void* arg);

#endif /* peer_h */
