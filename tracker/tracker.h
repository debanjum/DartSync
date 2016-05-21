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



