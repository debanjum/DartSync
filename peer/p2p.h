//
//  p2p.h
//  DartSync_Xcode
//
//  Created by Lexin Tang on 5/23/16.
//  Copyright © 2016 Lexin Tang. All rights reserved.
//

#ifndef p2p_h
#define p2p_h

#include "../common/constant.h"

// The packet data structure sending from peer to peer
typedef struct segment_ptp{
    char filename[FILE_NAME_LEN];
    int pieceNum;
    // size of file Data
    unsigned long size;
    // fiel data contained in this pkt
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


// Peer sends data packet to another peer
int ptp_sendpkt(int conn, ptp_data_pkt_t *pkt);

// Peer receives data packet from another peer
int ptp_recvpkt(int conn, ptp_data_pkt_t *pkt);

// ptp_listening thread: parse data request from remote peer
int recv_data_request(int conn, char* filename, int* pieceNum, unsigned long* offset, unsigned long* size);

// ptp_upload thread: peer wraps file data and send them to remote peer
int upload_send(int conn, char* file_path, int pieceNum, unsigned long offset, unsigned long size);

// ptp_download thread: send data request to remote peer
int download_send(int conn, ptp_request_t *request_pkt);


#endif /* p2p_h */
