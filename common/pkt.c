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

    Node *ftemp;
    ptp_peer_t *send_pkt = (ptp_peer_t *)calloc(1, sizeof(ptp_peer_t));
    int file_count = 0;
    
    send_pkt->type            = type;
    send_pkt->port            = P2P_PORT;
    send_pkt->file_table_size = -1;
    //[TODO]add peer ip to pkt struct

    if( type == FILE_UPDATE ) {
	//find no. of files in file_table
	for( ftemp = ft->head; ftemp != NULL; ftemp = ftemp->pNext, file_count++ ){};

	// send first packet with only table size (no. of nodes[files] in linked_list)
	send_pkt->file_table_size = file_count;
    }
    
    if(send(conn, send_pkt, sizeof(ptp_peer_t), 0) < 0) {
	free(send_pkt);
	return -1;
    }

    // if type of packet to send is FILE_UPDATE
    if( type == FILE_UPDATE ) {
	// pack and send each of the nodes in the file table one by one to tracker
	for( ftemp = ft->head; ftemp != NULL; ftemp = ftemp->pNext ){
	    ptp_peer_t *send_pkt   = (ptp_peer_t *)calloc(1, sizeof(ptp_peer_t));
	    send_pkt->file            = *ftemp;
	    send_pkt->file_table_size = -1;
	    send_pkt->type            = type;
	    send_pkt->port            = P2P_PORT;
	    
	    if(send(conn, send_pkt, sizeof(ptp_tracker_t), 0) < 0) {
		free(send_pkt);
		return -1;
	    }
	    free(send_pkt);
	}
    }

    free(send_pkt);
    return 1;
}


// peer recieves packet from tracker
int peer_recvpkt(int conn, file_t *ft){

    Node *temp;
    int file_table_size, counter;
    ptp_tracker_t* pkt = calloc(1, sizeof(ptp_tracker_t));

    //recieve first packet containing file_table_size 
    if (recv(conn, pkt, sizeof(ptp_tracker_t), 0)<0) {
	free(pkt);
	return -1;
    }
    file_table_size = pkt->file_table_size;
    free(pkt);
    
    for ( counter=0; counter < file_table_size; counter++ ) {
	pkt = calloc(1, sizeof(ptp_tracker_t));
	if ( recv(conn, pkt, sizeof(ptp_tracker_t), 0) < 0 ) {
	    free(pkt);
	    return -1;
	}
	if( counter == 0 ) {
	    temp        = calloc(1, sizeof(Node));
	    *temp        = pkt->file;
	    ft->head    = temp;	    
	}
	else {
	    temp->pNext = calloc(1, sizeof(Node));
	    *temp->pNext = pkt->file;
	    temp        = temp->pNext;
	}
	free(pkt);
    }
    
    return 0;
}


// tracker sends packet to peer
int tracker_sendpkt(int conn, file_t *ft)
{
    Node *ftemp;
    ptp_tracker_t *send_pkt = calloc(1, sizeof(ptp_tracker_t));
    int file_count = 0;
    
    //find no. of files in file_table
    for( ftemp = ft->head; ftemp != NULL; ftemp = ftemp->pNext, file_count++ ){};

    // send first packet with only table size (no. of nodes[files] in linked_list)
    send_pkt->file_table_size = file_count;

    if(send(conn, send_pkt, sizeof(ptp_tracker_t), 0) < 0) {
	free(send_pkt);
	return -1;
    }

    // pack and send each of the nodes in the file table one by one to peer
    for( ftemp = ft->head; ftemp != NULL; ftemp = ftemp->pNext ){
	ptp_tracker_t *send_pkt = calloc(1, sizeof(ptp_tracker_t));
	send_pkt->file            = *ftemp;
	send_pkt->file_table_size = -1;
	
	if(send(conn, send_pkt, sizeof(ptp_tracker_t), 0) < 0) {
	    free(send_pkt);
	    return -1;
	}
	free(send_pkt);
    }

    free(send_pkt);
    return 1;
}

// tracker recieves packet from peer
int tracker_recvpkt(int conn, ptp_peer_t *pkt, file_t *ft)
{
    Node *temp;
    int file_table_size, counter;

    //recieve first packet containing file_table_size 
    if (recv(conn, pkt, sizeof(ptp_peer_t), 0)<0) {
	free(pkt);
	return -1;
    }
    
    if(pkt->type == FILE_UPDATE) {
	file_table_size = pkt->file_table_size;
	free(pkt);
    
	for ( counter=0; counter < file_table_size; counter++ ) {
	    pkt = calloc(1, sizeof(ptp_peer_t));
	    if ( recv(conn, pkt, sizeof(ptp_peer_t), 0) < 0 ) {
		free(pkt);
		return -1;
	    }
	    if( counter == 0 ) {
		temp         = calloc(1, sizeof(Node));
		*temp        = pkt->file;
		ft->head     = temp;	    
	    }
	    else {
		temp->pNext  = calloc(1, sizeof(Node));
		*temp->pNext = pkt->file;
		temp         = temp->pNext;
	    }
	    free(pkt);
	}
    }    
    return 0;
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
    printf("~> sent data pkt to peer\n");
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
