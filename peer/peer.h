//
//  peer.h
//  peer
//
//  Created by Lexin Tang on 5/18/16.
//  Copyright © 2016 Lexin Tang. All rights reserved.
//

#ifndef peer_h
#define peer_h

#include "../common/pkt.h"
#include "p2p.h"
#include "monitor.h"

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

// ptp_listening thread keeps receiving data requests from other peers. It handles data requests by creating a P2P upload thread.
// ptp_listening thread is started after the peer is registered.
void* ptp_listening(void* arg);

// p2p_upload thread is started by listen_to_peer thread and responsible for uploading data to remote peer.
void* ptp_upload(void* arg);

// piece_download thread is for downloading one piece of file from a remote peer.
void* piece_download(void* arg);
    
// p2p_download thread is responsible for downloading one file from multiple peers.
void* ptp_download(void* arg);

// file_monitor thread monitors a local file directory
// it sends out updated file table to the tracker if any file changes in the local file directory.
void* file_monitor(void* arg);

// keep_alive thread sends out heartbeat messages to tracker periodically.
void* keep_alive(void* arg);

// compares tracker filetable with peer filetable and syncs to latest global version
void compareNode(Node *seekNode);

// this function is used to start connection to tracker, send REGISTER pkt and wait to ACCEPT 1st trackerFileTable.
int connect_to_tracker(file_t* trackerFileTable);

#endif /* peer_h */
