//
//  pkt.c
//  peer
//
//  Created by Lexin Tang on 5/18/16.
//  Copyright © 2016 Lexin Tang. All rights reserved.
//

#include "pkt.h"

char *getmyip() {
    
    char myhostname[100];
    gethostname(myhostname, 100);
    struct hostent *node;
    in_addr_t *addr_list = NULL;

    // get host by name
    if((node=gethostbyname(myhostname))==NULL)
        perror("gethostbyname");

    //ip address from hostname
    addr_list = *(in_addr_t **)node->h_addr_list;

    return inet_ntoa(*(struct in_addr *)addr_list);
}


// peer sends packet to tracker
int peer_sendpkt(int conn, file_t *ft, int type){

    Node *ftemp;
    ptp_peer_t *send_pkt = (ptp_peer_t *)calloc(1, sizeof(ptp_peer_t));
    int file_count = 0;
    
    send_pkt->type            = type;
    send_pkt->port            = P2P_PORT;
    send_pkt->file_table_size = -1;
    strcpy(send_pkt->peer_ip, getmyip()); // set my(peer) IP
    
    printf("send a pkt of type %d from %s\n", type, send_pkt->peer_ip);
    
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
	    ptp_peer_t *send_pkt      = (ptp_peer_t *)calloc(1, sizeof(ptp_peer_t));
	    send_pkt->file            = *ftemp;
	    send_pkt->file_table_size = -1;
	    send_pkt->type            = type;
	    send_pkt->port            = P2P_PORT;
	    strcpy(send_pkt->peer_ip, getmyip()); // set my(peer) IP
	    
	    if(send(conn, send_pkt, sizeof(ptp_peer_t), 0) < 0) {
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

    //receive first packet containing file_table_size
    if (recv(conn, pkt, sizeof(ptp_tracker_t), 0)<0) {
	free(pkt);
	return -1;
    }
    file_table_size = pkt->file_table_size;
    //free(pkt);
    printf(" received file_table of size: %d\n", file_table_size);
    for ( counter=0; counter < file_table_size; counter++ ) {
	pkt = calloc(1, sizeof(ptp_tracker_t));
	if ( recv(conn, pkt, sizeof(ptp_tracker_t), 0) < 0 ) {
	    free(pkt);
	    return -1;
	}
	if( counter == 0 ) {
	    temp         = calloc(1, sizeof(Node));
	    *temp        = pkt->file;
	    ft->head     = temp;
	    printf(" received file_table head\n");
	}
	else {
	    temp->pNext  = calloc(1, sizeof(Node));
	    *temp->pNext = pkt->file;
	    temp         = temp->pNext;
	    printf("received file_table node: %d\n", counter);
	}
	free(pkt);
    }
    free(pkt);
    return 1;
}


// tracker sends packet to peer
int tracker_sendpkt(int conn, file_t *ft)
{
    Node *ftemp;
    ptp_tracker_t *send_pkt = calloc(1, sizeof(ptp_tracker_t));
    int file_count = 0;
    
    //find no. of files in file_table
    printf("~>tracker_sendpkt: sending file table ");
    if(ft)
	for( ftemp = ft->head; ftemp != NULL; ftemp = ftemp->pNext, file_count++ ){};

    // send first packet with only table size (no. of nodes[files] in linked_list)
    send_pkt->file_table_size = file_count;
    
    printf("of size %d to peer\n", send_pkt->file_table_size);
    if(send(conn, send_pkt, sizeof(ptp_tracker_t), 0) < 0) {
	free(send_pkt);
	return -1;
    }

    if (!ft)
	return 1;
    
    // pack and send each of the nodes in the file table one by one to peer
    for( ftemp = ft->head; ftemp != NULL; ftemp = ftemp->pNext ){
	ptp_tracker_t *send_pkt   = calloc(1, sizeof(ptp_tracker_t));
	send_pkt->file            = *ftemp;
	send_pkt->file_table_size = -1;
	
	if(send(conn, send_pkt, sizeof(ptp_tracker_t), 0) < 0) {
	    free(send_pkt);
	    return -1;
	}
	printf("~>tracker_sendpkt: sent file %s to peer\n", send_pkt->file.name);
	free(send_pkt);
    }
    printf("~>tracker_sendpkt: sent file %s to peer\n", send_pkt->file.name);
    free(send_pkt);
    return 1;
}

// tracker recieves packet from peer and updates ptp_peer_t and file_table
int tracker_recvpkt(int conn, ptp_peer_t *pkt)
{
    //receive first packet containing file_table_size 
    if ( recv(conn, pkt, sizeof(ptp_peer_t), 0) < 0 ) {
	printf("error in receiving packet from peer\n"); 
	free(pkt);
	return -1;
    }
    printf("successfully received packet from %s\n", pkt->peer_ip); 
    return 1;
}
