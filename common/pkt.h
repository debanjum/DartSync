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
    Node file;
} ptp_peer_t;


// The packet data structure sending from tracker to peer
typedef struct segment_tracker{
    // file number in the file table -- optional
    int file_table_size;
    // file table of the tracker -- your own design
    Node file;
} ptp_tracker_t;

// The packet data structure sending from peer to peer
typedef struct segment_ptp{
    char filename[FILE_NAME_LEN];
    int pieceNum;
    // size of file Data
    unsigned long size;
    // file data contained in this pkt
    char data[MAX_DATA_LEN];
} ptp_data_pkt_t;

// structure of data request sent from remote peer
typedef struct data_request_ptp{
    char filename[FILE_NAME_LEN];
    unsigned long offset; // file offset
    unsigned long size; // piece size
    int pieceNum;
} ptp_request_t;

// data structure of the arguments parsed from ptp_listening thread to ptp_upload thread
typedef struct upload_arg_ptp{
    int sockfd;
    char file_path[FILE_NAME_LEN];
    int pieceNum;
    unsigned long offset; // file offset
    unsigned long size; // piece size
} upload_arg_t;

// data structure of the arguments parsed from main thread to ptp_download thread
typedef struct download_arg_ptp{
    char filename[FILE_NAME_LEN];
    unsigned long size;   // file size
    int peerNum; // num of available peers
    struct sockaddr_in addr_list[MAX_PEER_NUM];
} download_arg_t;

typedef struct piece_download_arg_ptp{
    char filename[FILE_NAME_LEN];
    unsigned long offset; // file offset
    unsigned long size; // piece size
    int pieceNum;
    struct sockaddr_in peer_addr;
} piece_download_arg_t;

// Tracker receives packet from peer
int tracker_recvpkt(int connection, ptp_peer_t *pkt, file_t *ft);

// Tracker sends packet to peer with its current file table wrapped
int tracker_sendpkt(int conn, file_t *ft);

// Peer wraps its file table and request type into a packet and sends it to the tracker    
int peer_sendpkt(int conn, file_t *ft, int type);

// Peer receives packet from tracker and unpacks trackers file table 
int peer_recvpkt(int conn, file_t *ft);

// Peer sends data packet to another peer
int ptp_sendpkt(int conn, ptp_data_pkt_t *pkt);

// Peer receives data packet from another peer
int ptp_recvpkt(int conn, ptp_data_pkt_t *pkt);

#endif // pkt_h


