//
//  tracker.h
//  tracker
//
//  Created by Debanjum Singh Solanky on 05/20/16.
//  Copyright © 2016 Debanjum Singh Solanky. All rights reserved.

#include "../common/pkt.h"

// tracker-side peer table
typedef struct tracker_side_peer_t {
    //Remote peer IP address, 16 bytes.
    char ip[IP_LEN];
    //Last alive timestamp of this peer.
    unsigned long last_time_stamp;
    //TCP connection to this remote peer.
    int sockfd;
    //Pointer to the next peer, linked list.
    struct tracker_side_peer_t *next;
} tracker_peer_t;

//delete peer with given peer socket file descriptor(peer_sockfd)
int delete_peer(int peer_sockfd);

// add peer to tracker_peer_t list
int add_peer(int peer_sockfd, char ip[IP_LEN]);

// update tracker filetable based on information from peer's filetable
int update_filetable(file_t *peer_ft);

// send tracker filetable to all peer's in peers_list
int broadcast_filetable();

// heartbeat thread continuously checks (every HEARTBEAT_INTERVAL) and remove peers not heard from in a while(= HEARTBEAT_TIMEOUT)
void* heartbeat(void* arg);

// main thread for handling communication with individual peers
void* handshake(void* arg);
