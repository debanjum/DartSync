//
//  pkt.c
//  peer
//
//  Created by Lexin Tang on 5/18/16.
//  Copyright © 2016 Lexin Tang. All rights reserved.
//

#include "pkt.h"

int peer_sendpkt(int conn, file_t *ft, int type){
    // pack file table into ptp_peer_t
    ptp_peer_t *pkt = (ptp_peer_t *)malloc(sizeof(ptp_peer_t));
    pkt->type = type;
    
    return 1;
}

int peer_recvpkt(int conn, file_t *ft){
    
    return -1;
}


