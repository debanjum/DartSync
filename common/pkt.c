//
//  pkt.c
//  peer
//
//  Created by Lexin Tang on 5/18/16.
//  Copyright © 2016 Lexin Tang. All rights reserved.
//

#include "pkt.h"

/*in_addr_t *getmyip() {
    
    char myhostname[100];
    gethostname(myhostname, 100);
    struct hostent *node;
    in_addr_t *addr_list = NULL;

    // get host by name
    if((node=gethostbyname(myhostname))==NULL)
        perror("gethostbyname");

    //ip address from hostname
    addr_list = *(in_addr_t **)node->h_addr_list;

    return (in_addr_t *)addr_list;
    }*/

// peer sends packet to tracker
int peer_sendpkt(int conn, file_t *ft, int type){

    // pack file table into ptp_peer_t
    ptp_peer_t *send_pkt = (ptp_peer_t *)calloc(1, sizeof(ptp_peer_t));
    send_pkt->type = type;
    send_pkt->file_table_size = sizeof(ft);
    send_pkt->file_table = *ft;
    send_pkt->port = P2P_PORT;
    //[TODO]add peer ip to pkt struct
    
    char bufstart[2];
    char bufend[2];
    bufstart[0] = '!';
    bufstart[1] = '&';
    bufend[0] = '!';
    bufend[1] = '#';
    
    if (send(conn, bufstart, 2, 0) < 0) {
        free(send_pkt);
        return -1;
    }
    if(send(conn, send_pkt, sizeof(ptp_peer_t), 0) < 0) {
        free(send_pkt);
        return -1;
    }
    if(send(conn, bufend, 2, 0) < 0) {
        free(send_pkt);
        return -1;
    }
    printf("~> sent file table to tracker\n");
    free(send_pkt);
    return 1;
}

// peer recieves packet from tracker
int peer_recvpkt(int conn, file_t *ft){

    char buf[sizeof(ptp_tracker_t)+2], c; 
    int idx = 0;
    int state = 0;
    ptp_tracker_t* pkt = calloc(1, sizeof(ptp_tracker_t));
    
    // state can be 0,1,2,3;
    // 0 starting point
    // 1 '!' received
    // 2 '&' received, start receiving segment
    // 3 '!' received,
    // 4 '#' received, finish receiving segment
    
    while(recv(conn, &c, 1, 0)>0) {
	if (state == 0) {
	    if(c=='!')
		state = 1;
	}
	else if(state == 1) {
	    if(c=='&') 
		state = 2;
	    else
		state = 0;
	}
	else if(state == 2) {
	    if(c=='!') {
		buf[idx]=c;
		idx++;
		state = 3;
	    }
	    else {
		buf[idx]=c;
		idx++;
	    }
	}
	else if(state == 3) {
	    if(c=='#') {
		buf[idx]=c;
		idx++;
		state = 0;
		idx = 0;
		memcpy(pkt, buf, idx-2);
		*ft = pkt->file_table;
		return 0;
	    }
	    else if(c=='!') {
		buf[idx]=c;
		idx++;
	    }
	    else {
		buf[idx]=c;
		idx++;
		state = 2;
	    }
	}
    }
    return -1;
}

// tracker sends packet to peer
int tracker_sendpkt(int conn, file_t *ft)
{
    ptp_tracker_t *send_pkt = calloc(1, sizeof(ptp_tracker_t));
    send_pkt->file_table_size = sizeof(ft);
    send_pkt->file_table = *ft;
	
    char bufstart[2];
    char bufend[2];
    bufstart[0] = '!';
    bufstart[1] = '&';
    bufend[0] = '!';
    bufend[1] = '#';
    
    if (send(conn, bufstart, 2, 0) < 0) {
        free(send_pkt);
        return -1;
    }
    if(send(conn, send_pkt, sizeof(ptp_tracker_t), 0) < 0) {
        free(send_pkt);
        return -1;
    }
    if(send(conn, bufend, 2, 0) < 0) {
        free(send_pkt);
        return -1;
    }
    printf("~> sent file table to peer\n");
    free(send_pkt);
    return 1;
}

// tracker recieves packet from peer
int tracker_recvpkt(int conn, ptp_peer_t *pkt)
{
    char buf[sizeof(ptp_peer_t)+2], c; 
    int idx = 0;
    int state = 0;
    
    // state can be 0,1,2,3;
    // 0 starting point
    // 1 '!' received
    // 2 '&' received, start receiving segment
    // 3 '!' received,
    // 4 '#' received, finish receiving segment
    
    while(recv(conn, &c, 1, 0)>0) {
	if (state == 0) {
	    if(c=='!')
		state = 1;
	}
	else if(state == 1) {
	    if(c=='&') 
		state = 2;
	    else
		state = 0;
	}
	else if(state == 2) {
	    if(c=='!') {
		buf[idx]=c;
		idx++;
		state = 3;
	    }
	    else {
		buf[idx]=c;
		idx++;
	    }
	}
	else if(state == 3) {
	    if(c=='#') {
		buf[idx]=c;
		idx++;
		state = 0;
		idx = 0;
		memcpy(pkt, buf, idx-2);
		return 0;
	    }
	    else if(c=='!') {
		buf[idx]=c;
		idx++;
	    }
	    else {
		buf[idx]=c;
		idx++;
		state = 2;
	    }
	}
    }
    return -1;
}


// Peer sends data packet to another peer
int ptp_sendpkt(int conn, ptp_data_pkt_t *pkt){
    char bufstart[2];
    char bufend[2];
    bufstart[0] = '!';
    bufstart[1] = '&';
    bufend[0] = '!';
    bufend[1] = '#';
    
    if (send(conn, bufstart, 2, 0) < 0) {
        free(pkt);
        return -1;
    }
    if(send(conn, pkt, sizeof(ptp_data_pkt_t), 0) < 0) {
        free(pkt);
        return -1;
    }
    if(send(conn, bufend, 2, 0) < 0) {
        free(pkt);
        return -1;
    }
    printf("~> sent data pkt to peer ip = %s\n", pkt->dest_ip);
    free(pkt);
    return 1;
}

// Peer receives data packet from another peer
int ptp_recvpkt(int conn, ptp_data_pkt_t *pkt){
    char buf[sizeof(ptp_data_pkt_t)+2];
    char c;
    int idx = 0;
    // state can be 0,1,2,3;
    // 0 starting point
    // 1 '!' received
    // 2 '&' received, start receiving segment
    // 3 '!' received,
    // 4 '#' received, finish receiving segment
    int state = 0;
    while(recv(conn,&c,1,0)>0) {
        if (state == 0) {
            if(c=='!')
                state = 1;
        }
        else if(state == 1) {
            if(c=='&')
                state = 2;
            else
                state = 0;
        }
        else if(state == 2) {
            if(c=='!') {
                buf[idx]=c;
                idx++;
                state = 3;
            }
            else {
                buf[idx]=c;
                idx++;
            }
        }
        else if(state == 3) {
            if(c=='#') {
                buf[idx]=c;
                idx++;
                state = 0;
                idx = 0;
                memmove(pkt, buf, sizeof(ptp_data_pkt_t));
                return 1;
            }
            else if(c=='!') {
                buf[idx]=c;
                idx++;
            }
            else {
                buf[idx]=c;
                idx++;
                state = 2;
            }
        }
    }
    return -1;
}
