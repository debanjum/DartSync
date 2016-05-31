//
//  tracker.c
//  tracker
//
//  Created by Debanjum Singh Solanky on 05/20/16.
//  Copyright � 2016 Debanjum Singh Solanky. All rights reserved.

#include "tracker.h"

tracker_peer_t *peer_head = NULL;   // Peer List
file_t *ft                = NULL;   // File Table

// main thread for handling communication with individual peers
void* handshake(void* arg) {
    int connection = *(int *)arg;
    ptp_peer_t* recv_pkt = (ptp_peer_t *)calloc(1,sizeof(ptp_peer_t));
    
    int counter = -1, file_table_size = -1;
    file_t *peer_ft;
    Node *temp;
    printf("->handshake: handshake thread started\n");    
    while( tracker_recvpkt(connection, recv_pkt) > 0 ) {
	printf("~>handshake: received packet from %s of type %d\n", recv_pkt->peer_ip, recv_pkt->type);
	if( recv_pkt->type == REGISTER )
	    tracker_sendpkt(connection, ft);                  // send tracker file_table to peer as response
	
	else if( recv_pkt->type == FILE_UPDATE ) {

	    // if tracker expects first FILE_UPDATE and peer sent first FILE_UPDATE packet 
	    if( counter == -1 && recv_pkt->file_table_size != -1)  {
		printf("~>handshake: received 1ST file node from peer\n");
		file_table_size = recv_pkt->file_table_size;        // first FILE_UPDATE packet contains only no. of nodes in linked list
		peer_ft = calloc(1, sizeof(file_t));                // initialize peer_file table for receiving
	    }

	    // if tracker expects file Node and peer sent packet with file Node
	    else if( ++counter < file_table_size && recv_pkt->file_table_size == -1 ) {
		// if second FILE_UPDATE packet used to set linked list HEAD
		if( counter == 0 ) {                          
		    temp           = calloc(1, sizeof(Node));
		    *temp          = recv_pkt->file;
		    peer_ft->head  = temp;
		    printf("~>handshake: received 2ND file node from peer\n");
		}
		else {
		    temp->pNext    = calloc(1, sizeof(Node));
		    *temp->pNext   = recv_pkt->file;
		    temp           = temp->pNext;
		    printf("~>handshake: received >=3RD file node from peer\n");
		}
	    }
	    
	    // if received last file Node of peer's file_table
	    if ( counter == (file_table_size-1) ) {
		printf("~>handshake: received last file node from peer\n");
		update_filetable(peer_ft);            // update tracker file_table based on information from peer's file_table
		
		counter = -1;                         // reset counter
		file_table_size = -1;                 // reset file_table_size, to detect if tracker expects file Node in previous else if
		free(peer_ft);                        // free peer file table
		
		if (broadcast_filetable()<0)          // send updated tracker file_table to all peers
		    printf("~>handshake: error in broadcasting filetable\n");	
	    }
	}
	
	else if( recv_pkt->type == KEEP_ALIVE )       // if receive heartbeat message(KEEP_ALIVE)
	    heard_peer(connection);                   // update last_heard timestamp of peer with 'connection'
    }
    printf("~>handshake: exiting handshake thread\n");
    return 0;
}


// continuously check (every HEARTBEAT_INTERVAL) and remove peers not heard from in a while (= HEARTBEAT_TIMEOUT)
void* heartbeat(void* arg) {
    tracker_peer_t* temp;
    printf("heartbeat thread started\n");
    
    while(1) {
	//traverse through peers linked_list for peer
	for( temp = peer_head; temp != NULL; temp = temp->next ) 
	    // when did we last hear from the peer ? if last heard from peer > HEARTBEAT_TIMEOUT
	    if ( ((unsigned long)time(NULL) - temp->last_time_stamp)  > HEARTBEAT_TIMEOUT )
		delete_peer(temp->sockfd, temp->ip);
	// sleep between checks
	sleep(HEARTBEAT_INTERVAL);   
    }
    return 0;
}


// send tracker filetable to all peer's in peers_list
int broadcast_filetable() {
    tracker_peer_t* temp;
    printf("~>broadcast_filetable: broadcasting updated tracker filetable to all peers\n");
    //traverse through peer_list and transmit file_table to each peer
    for( temp = peer_head; temp != NULL; temp = temp->next )
	if (tracker_sendpkt(temp->sockfd, ft) < 0)
	    return -1;
    
    return 1;
}


// update tracker filetable based on information from peer's filetable
int update_filetable(file_t *peer_ft) {
    Node *peer_ftemp, *tracker_ftemp;
    int file_found = 0, index, peer_found=0;
    
    printf("~>update_filetable: updating tracker file table\n");
    // traverse through each file in peer's file table
    for( peer_ftemp = peer_ft->head; peer_ftemp != NULL; peer_ftemp = peer_ftemp->pNext ) {
	file_found = 0;
	
	// traverse through each file till before last in tracker's file table
	if(ft) {
	    for( tracker_ftemp = ft->head; tracker_ftemp != NULL; tracker_ftemp = tracker_ftemp->pNext ) {
		
		if ( !tracker_ftemp->name || !peer_ftemp->name) {
		    printf(" ftemp not set\n");
		}
		printf("tracker_ftemp name: %s\t peer_ftemp name: %s\n", tracker_ftemp->name, peer_ftemp->name);
		// if found matching file entry between tracker and peer
		if ( strcmp(tracker_ftemp->name, peer_ftemp->name) == 0 ) {
		    printf("ASD\n");
		    // UPDATE_FILE_PEERS: if matching file entry in tracker same as on peer, update tracker file_table entry
		    if(tracker_ftemp->timestamp == peer_ftemp->timestamp) {
			int peers  = sizeof(tracker_ftemp->newpeerip)/IP_LEN;                  // find no. of peers in 'newpeerip' string array
			peer_found = 0;
			
			// check if peer exists in newpeerip list of peers with latest file version
			for( index=0; index<peers; index++)
			    if(strcmp(tracker_ftemp->newpeerip[index], peer_ftemp->newpeerip[0])==0) {
				peer_found=1;
				break;
			    }
			
			// if peer doesn't exist in newpeerip list of peers with latest file version
			if(!peer_found)
			    strcpy(tracker_ftemp->newpeerip[peers], peer_ftemp->newpeerip[0]);  // append new peer to end of 'newpeerip' string array
			
			file_found = 1;
			break;
		    }
		    
		    // UPDATE_FILE_VERSION : if matching file entry in tracker older than on peer, update tracker file_table entry
		    else if(tracker_ftemp->timestamp < peer_ftemp->timestamp) {
			tracker_ftemp->size      = peer_ftemp->size;
			tracker_ftemp->timestamp = peer_ftemp->timestamp;
			tracker_ftemp->type      = peer_ftemp->type;
			tracker_ftemp->status    = peer_ftemp->status;
			strcpy(tracker_ftemp->newpeerip[0], peer_ftemp->newpeerip[0]);
			file_found               = 1;
			break;
		    }
		}
		
		// break while pointer still pointing to last element in file_table linked list. Allows appending later if required
		if (tracker_ftemp->pNext == NULL)
		    break;
	    }
	}
	// ADD_FILE: if no file with current file_name on peer found on tracker's file table, append the file to tracker's file table
	if(file_found==0) {
	    
	    //if file_list not empty, head!=NULL case
	    if(tracker_ftemp) {
		tracker_ftemp->pNext  = calloc(1, sizeof(Node));
		tracker_ftemp         = tracker_ftemp->pNext;
	    }
	    
	    //catches case of empty list, head==NULL case
	    else {
		printf("~>update_filetable: HEAD was null\n");
		tracker_ftemp        = calloc(1, sizeof(Node));
		ft                   = calloc(1, sizeof(file_t));
		ft->head             = tracker_ftemp;
	    }
	    
	    // add file meta data
	    strcpy(tracker_ftemp->name, peer_ftemp->name);
	    strcpy(tracker_ftemp->newpeerip[0], peer_ftemp->newpeerip[0]);
	    tracker_ftemp->size      = peer_ftemp->size;
	    tracker_ftemp->timestamp = peer_ftemp->timestamp;
	    tracker_ftemp->type      = peer_ftemp->type;
	    tracker_ftemp->status    = peer_ftemp->status;
	    //printf("~>update_filetable: added %s from %s to file table\n", ft->head->name);
	}
    }
    printf("~>update_filetable: updated file table successfully\n");
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
	}
	// append empty peer_tracker struct to end of list
	temp->next        = calloc(1, sizeof(tracker_peer_t));
	temp              = temp->next;
    }

    //append new peer data to the end of the list
    memcpy(temp->ip, ip, IP_LEN);                        // peer's ip address string
    temp->last_time_stamp = (unsigned long)time(NULL);   // peer's current timestamp
    temp->sockfd          = peer_sockfd;                 // peer's connection file descriptor
    temp->next            = NULL;
    printf("added peer %s to peer_list\n", temp->ip);
    return 1;
}


//delete peer with given peer socket file descriptor(peer_sockfd)
int delete_peer(int peer_sockfd, char peer_ip[IP_LEN]) {
    int index, counter;
    tracker_peer_t* temp;
    Node *tracker_ftemp;
    
    if(ft) {
	// for each file node in file table
	for( tracker_ftemp = ft->head; tracker_ftemp != NULL; tracker_ftemp = tracker_ftemp->pNext ) {
	    // check if peer exists in newpeerip list of peers with latest file version
	    for( index=0; index<MAX_PEER_NUM && tracker_ftemp->newpeerip[index]!=NULL; index++ ) {
		// if peer found
		if( strcmp( tracker_ftemp->newpeerip[index], peer_ip ) == 0 ) {
		    // traverse and find last peer in Node's peer list
		    for(counter=index; counter<MAX_PEER_NUM && tracker_ftemp->newpeerip[counter]!=NULL; counter++) {};
		    
		    // overwrite peer IP by last set peer IP in Node's peer list
		    strcpy(tracker_ftemp->newpeerip[index], tracker_ftemp->newpeerip[counter-1]);
		    // and set overwriteing IP index in peer list to NULL
		    bzero(tracker_ftemp->newpeerip[counter-1], IP_LEN);
		}
	    }
	}
    }
    //search through peers linked_list for peer with peer_sockfd and delete it
    for( temp = peer_head; temp != NULL; temp = temp->next)
	if (temp->sockfd == peer_sockfd) {
	    close(temp->sockfd);
	    temp = temp->next;
	}
    
    return 1;
}


//heard heartbeat of peer with given peer socket file descriptor(peer_sockfd)
int heard_peer(int peer_sockfd) {    
    tracker_peer_t* temp;
    
    //search through peers linked_list for peer with peer_sockfd and update its last_time_stamp
    for( temp = peer_head; temp != NULL; temp = temp->next )
	if (temp->sockfd == peer_sockfd) {
	    temp->last_time_stamp = (unsigned long)time(NULL);   // peer's last_time_stamp
	    return 1;
	}
    return -1;
}


int main() {
    int peer_sockfd;
    struct sockaddr_in tracker_addr;
    int connection;
    struct sockaddr_in peer_addr;
    socklen_t peer_addr_len = sizeof(struct sockaddr_in);
    
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
    
    printf("listening on %d for peer connections\n", HANDSHAKE_PORT);

    pthread_t heartbeat_thread;
    pthread_create(&heartbeat_thread, NULL, heartbeat, NULL);

    while((connection = accept(peer_sockfd, (struct sockaddr*)&peer_addr, &peer_addr_len))>0) {
	printf("accepted connection from %s and create a socket %d\n", inet_ntoa(peer_addr.sin_addr), connection);
	// Register peer in peer-list, [MAYBE] Ideally should be in handshake_thread, if peer sends pkt of type pkt->type=REGISTER
	add_peer(connection, inet_ntoa(peer_addr.sin_addr));
	
	// Create and hand-off communication with peer to a separate handshake thread
	pthread_t handshake_thread;
	pthread_create(&handshake_thread, NULL, handshake, (void*)&connection);
    }
    
    return 0;
}

