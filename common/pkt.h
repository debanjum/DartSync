//
//  pkt.h
//  peer
//
//  Created by Lexin Tang on 5/18/16.
//  Copyright © 2016 Lexin Tang. All rights reserved.
//

#ifndef pkt_h
#define pkt_h
#include "../common/constant.h"

// The packet data structure sending from peer to tracker
typedef struct segment_peer {
    // packet type : register, keep alive, update file table
    int type;
    // reserved space, you could use this space for your convenience, 8 bytes by default
    char reserved[RESERVED_LEN];
    // the peer ip address sending this packet
    char peer_ip[IP_LEN];
    // listening port number in p2p
    int port;
    // the number of files in the local file table -- optional
    int file_table_size;
    // file table of the client -- your own design
    file_t file_table;
} ptp_peer_t;


// The packet data structure sending from tracker to peer
typedef struct segment_tracker{
    // file number in the file table -- optional
    int file_table_size;
    // file table of the tracker -- your own design
    file_t  file_table;
} ptp_tracker_t;

// Tracker receives packet from peer
int tracker_recvpkt(int connection, ptp_peer_t *pkt);

// Tracker sends packet to peer with its current file table wrapped
int tracker_sendpkt(int conn, file_t *ft);

// Peer wraps its file table and request type into a packet and sends it to the tracker    
int peer_sendpkt(int conn, file_t *ft, int type);

// Peer receives packet from tracker and unpacks trackers file table 
int peer_recvpkt(int conn, file_t *ft);

#endif // pkt_h


