//
//  tracker.c
//  tracker
//
//  Created by Debanjum Singh Solanky on 05/20/16.
//  Copyright © 2016 Debanjum Singh Solanky. All rights reserved.

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "tracker.h"

tracker_peer_t *peer_head = NULL;   //Peer List
file_t *ft                = NULL;   //File Table

// main thread for handling communication with individual peers
void* handshake(void* arg) {
    int connection = *(int *)arg;
    ptp_peer_t* recv_pkt = (ptp_peer_t *)calloc(1,sizeof(ptp_peer_t));
    
    while(tracker_recvpkt(connection, recv_pkt)) {
	if(recv_pkt->type==REGISTER)
	    tracker_sendpkt(connection, ft);
	else if(recv_pkt->type==FILE_UPDATE) {
	    update_filetable(&recv_pkt->file_table);
	}
    }
    return 0;
}

// continuously check (every HEARTBEAT_INTERVAL) and remove peers not heard from in a while (= HEARTBEAT_TIMEOUT)
void* heartbeat(void* arg) {
    tracker_peer_t* temp;
    
    while(1) {
	//traverse through peers linked_list for peer
	for( temp = peer_head; temp != NULL; temp = temp->next )
	    // when did we last hear from the peer ? if last heard from peer > HEARTBEAT_TIMEOUT
	    if ( ((unsigned long)time(NULL) - temp->last_time_stamp)  > HEARTBEAT_TIMEOUT )
		delete_peer(temp->sockfd);
	// sleep between checks
	sleep(HEARTBEAT_INTERVAL);   
    }
}

// update tracker filetable based on information from peer's filetable
int update_filetable(file_t *peer_ft) {
    Node *peer_ftemp, *tracker_ftemp;
    int found = 0;

    // traverse through each file in peer's file table
    for( peer_ftemp = peer_ft->head; peer_ftemp != NULL; peer_ftemp = peer_ftemp->pNext ) {
	found = 0;
	// traverse through each file till before last in tracker's file table
	for( tracker_ftemp = ft->head; tracker_ftemp != NULL; tracker_ftemp = tracker_ftemp->pNext ) {
	    // if found matchine file entry between tracker and peer
	    if (strcmp(tracker_ftemp->name, peer_ftemp->name)==0) {
		// UPDATE_FILEMETA: if matching file entry in tracker older than on peer, update tracker file_table entry
		if(tracker_ftemp->timestamp < peer_ftemp->timestamp) {
		    tracker_ftemp->size = peer_ftemp->size;
		    tracker_ftemp->timestamp = peer_ftemp->timestamp;
		    memcpy(tracker_ftemp->newpeerip, peer_ftemp->newpeerip, IP_LEN);
		    found = 1;
		    break;
		}
	    }
	    //break while pointer still pointing to last element in file_table linked list. Allows appending later if required
	    if (tracker_ftemp->pNext == NULL)
		break;
	}
	// APPEND FILEMETA: if no file with current file_name on peer found on tracker's file table, append the file to tracker's file table
	if(!found) {
	    strcpy(tracker_ftemp->name, peer_ftemp->name);
	    memcpy(tracker_ftemp->newpeerip, peer_ftemp->newpeerip, IP_LEN);
	    tracker_ftemp->size      = peer_ftemp->size;
	    tracker_ftemp->timestamp = peer_ftemp->timestamp;
	}
    }
    return 0;
}

// add peer to tracker_peer_t list
int add_peer(int peer_sockfd, char ip[IP_LEN]) {
    tracker_peer_t* temp;
    
    //if first peer has come online
    if(!peer_head) {
	peer_head = calloc(1, sizeof(tracker_peer_t));
	temp = peer_head;
    }
    else {
	//move through peers linked_list
	for( temp = peer_head; temp->next != NULL; temp = temp->next ) {
	    //if peer's sockfd already in peer_list, don't add duplicate!
	    if(temp->sockfd == peer_sockfd)
		return -1;
	};
	// append empty peer_tracker struct to end of list
	temp->next                  = calloc(1, sizeof(tracker_peer_t));
	temp                        = temp->next;
    }

    //append new peer data to the end of the list
    memcpy(temp->ip, ip, IP_LEN);                        // peer's ip address string
    temp->last_time_stamp = (unsigned long)time(NULL);   // peer's current timestamp
    temp->sockfd          = peer_sockfd;                 // peer's connection file descriptor
    temp->next            = NULL;
    return 1;
}


//delete peer with given peer socket file descriptor(peer_sockfd)
int delete_peer(int peer_sockfd)
{
    tracker_peer_t* temp;
    //search through peers linked_list for peer with peer_sockfd and delete it
    for( temp = peer_head; temp != NULL; temp = temp->next )
	if (temp->sockfd == peer_sockfd)
	    temp = temp->next;
    
    return 1;
}


int main() {
    int peer_sockfd;
    struct sockaddr_in tracker_addr;
    int connection;
    struct sockaddr_in peer_addr;
    socklen_t peer_addr_len;
    
    peer_sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if(peer_sockfd<0) 
	return -1;
    
    memset(&tracker_addr, 0, sizeof(tracker_addr));
    tracker_addr.sin_family = AF_INET;
    tracker_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    tracker_addr.sin_port = htons(HANDSHAKE_PORT);

    if(bind(peer_sockfd, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr))< 0)
	return -1; 
    if(listen(peer_sockfd, 1) < 0)
	return -1;
    
    printf("waiting for connection\n");

    pthread_t heartbeat_thread;
    pthread_create(&heartbeat_thread, NULL, heartbeat, NULL);

    while((connection = accept(peer_sockfd, (struct sockaddr*)&peer_addr, &peer_addr_len))>0) {
	
	// Register peer in peer-list, [MAYBE] Ideally should be in handshake_thread, if peer sends pkt of type pkt->type=REGISTER
	add_peer(peer_sockfd, inet_ntoa(peer_addr.sin_addr));
	
	// Create and hand-off communication with peer to a separate handshake thread
	pthread_t handshake_thread;
	pthread_create(&handshake_thread, NULL, handshake, (void*)&connection);
    }
}
