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

// tracker recieves packet from peer and updates ptp_peer_t and file_table
int tracker_recvpkt(int conn, ptp_peer_t *pkt, file_t *ft)
{
    Node *temp;
    int file_table_size, counter;

    //receive first packet containing file_table_size 
    if ( recv(conn, pkt, sizeof(ptp_peer_t), 0) < 0 ) {
	free(pkt);
	return -1;
    }
    //if packet of type FILE_UPDATE
    if( pkt->type == FILE_UPDATE ) {
	file_table_size = pkt->file_table_size;  //first packet contains no. of nodes in linked list
	
	// wait for recieving all the nodes from the
	for ( counter=0; counter < file_table_size; counter++ ) {
	    pkt = calloc(1, sizeof(ptp_peer_t));
	    if ( recv(conn, pkt, sizeof(ptp_peer_t), 0) < 0 ) {
		free(pkt);
		return -1;
	    }
	    if(pkt->type == FILE_UPDATE) {
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
	    }
	    // if packet recieved not of type FILE_UPDATE
	    else {
		counter--;
	    }
	}
    }    
    return 0;
}
