//
//  p2p.c
//  DartSync_Xcode
//
//  Created by Lexin Tang on 5/23/16.
//  Copyright © 2016 Lexin Tang. All rights reserved.
//

#include "p2p.h"

// Peer sends data packet to another peer
int ptp_sendpkt(int conn, ptp_data_pkt_t *pkt){
    if(send(conn, pkt, sizeof(ptp_data_pkt_t), 0) < 0) {
        free(pkt);
        return -1;
    }
    printf("~> sent data pkt to peer\n");
    printf("ptp_sendpkt: pkt size = %lu\n", pkt->size);
    printf("ptp_sendpkt: pkt pieceNum = %d\n", pkt->pieceNum);
    free(pkt);
    return 1;
}

// Peer receives data packet from another peer
int ptp_recvpkt(int conn, ptp_data_pkt_t *pkt){
    if (recv(conn, pkt, sizeof(ptp_data_pkt_t), 0) == -1){
        perror("socket receive error!\n");
        return -1;
    }
    printf("ptp_recvpkt: pkt size = %lu\n", pkt->size);
    return 1;
}

// ptp_listening thread: parse data request from remote peer
int recv_data_request(int conn, char* filename, int* pieceNum, unsigned long* offset, unsigned long* size){
    ptp_request_t request_pkt;
    if (recv(conn, &request_pkt, sizeof(ptp_request_t), 0) == -1){
        perror("socket receive error!\n");
        return -1;
    }
    
    strcpy(filename, request_pkt.filename);
    *pieceNum = request_pkt.pieceNum;
    *offset = request_pkt.offset;
    *size = request_pkt.size;
    return 1;
}

// ptp_upload thread: peer wraps file data and send them to remote peer
int upload_send(int conn, char* file_path, int pieceNum, unsigned long offset, unsigned long size){
    FILE *fp;
    if ((fp = fopen(file_path, "r")) == NULL){
        printf("Cannot find %s!\n", file_path);
        return -1;
    }
    
    int fileLen = (int)size;
    printf("ptp_upload: sending a piece of file [%s], fileLen = %d\n", file_path, fileLen);
    
    char *buffer = (char *)malloc(fileLen);
    fseek(fp, offset, SEEK_SET);
    fread(buffer, fileLen, 1, fp);
    fclose(fp);
    
    // cut data into several packets if data is too large
    int pkt_count = fileLen / MAX_DATA_LEN;
    int remainder_len = fileLen % MAX_DATA_LEN;
    if (remainder_len > 0){
        pkt_count++;
    }
    
    // send data packet one by one
    for (int i = 0; i < pkt_count; i++){
        ptp_data_pkt_t *pkt = (ptp_data_pkt_t *)malloc(sizeof(ptp_data_pkt_t));
        memset(pkt, 0, sizeof(ptp_data_pkt_t));
        char *dataPtr = buffer;
        int length = (remainder_len > 0 && i == pkt_count - 1)? remainder_len : MAX_DATA_LEN;
        memmove(pkt->data, &dataPtr[i * MAX_DATA_LEN], length);
        
        // filename? timestamp?
        pkt->pieceNum = pieceNum;
        pkt->size = length;
        
        if (ptp_sendpkt(conn, pkt) < 0){
            return -1;
        }
    }
    
    free(buffer);
    return 1;
}

// ptp_download thread: send data request to remote peer
int download_send(int conn, ptp_request_t *request_pkt){
    if(send(conn, request_pkt, sizeof(ptp_request_t), 0) < 0) {
        free(request_pkt);
        return -1;
    }
    printf("~> sent data request to peer\n");
    free(request_pkt);
    return 1;
}

