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

void* handshake(void* arg) {
    int connection = *(int *)arg;
    ptp_peer_t* recv_pkt = (ptp_peer_t *)calloc(1,sizeof(ptp_peer_t));
    
    while(tracker_recvpkt(connection, recv_pkt)) {
	if(recv_pkt->type==REGISTER)
	    tracker_sendpkt(connection, ft);
    }
    return 0;
}
void* heartbeat(void* arg) {
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
	//move to end of peers linked_list
	for( temp = peer_head; temp->next != NULL; temp = temp->next ) {};
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
    pthread_create(&heartbeat_thread, NULL, heartbeat, (void*)&connection);

    while((connection = accept(peer_sockfd, (struct sockaddr*)&peer_addr, &peer_addr_len))>0) {
	//start the waitNbrs thread to wait for incoming connections from neighbors with larger node IDs
	//if(pkt->type == ACCEPT)

	add_peer(peer_sockfd, inet_ntoa(peer_addr.sin_addr));
	pthread_t handshake_thread;
	pthread_create(&handshake_thread, NULL, handshake, (void*)&connection);
	return 1;
    }
}
