//
//  main.c
//  peer
//
//  Created by Lexin Tang on 5/18/16.
//  Copyright © 2016 Lexin Tang. All rights reserved.
//

#include "peer.h"

int listen_fd, conn;
Node *file_table;

// this function is used to initialize file table
Node* filetable_create(){
    Node *ft = (Node *)malloc(sizeof(Node));
    return ft;
}

int peer_sendpkt(int conn, Node *ft, int type){
    // pack file table into ptp_peer_t
    ptp_peer_t *pkt = (ptp_peer_t *)malloc(sizeof(ptp_peer_t));
    pkt->type = type;
    pkt->port = listen_fd;
    
    return 1;
}

int peer_recvpkt(int conn, Node *ft){
    
    return -1;
}

// listen_to_peer thread keeps receiving data requests from other peers. It handles data requests by creating a P2P upload thread.
// listen_to_peer thread are started after the peer is registered.
void* listen_to_peer(void* arg){
    int conn;
    socklen_t sin_size;
    sin_size = sizeof(struct sockaddr_in);
    struct sockaddr_in client_addr;
    
    while (1){
        if ((conn = accept(listen_fd, (struct sockaddr *)&(client_addr), &sin_size)) == -1) {
            perror("accept error\n");
            exit(1);
        }
        printf("Listen_to_peer: received a peer request!\n");
        
        // create a ptp_upload thread
        pthread_t ptp_upload_thread;
        pthread_create(&ptp_upload_thread, NULL, ptp_upload, (void*)0);
        printf("Listen_to_peer: created ptp_upload thread!\n");
    }
}

// ptp_upload thread is started by listen_to_peer thread and responsible for uploading data to remote peer.
void* ptp_upload(void* arg){
    
    // terminate this thread
    pthread_detach(pthread_self());
    pthread_exit(0);
}

// ptp_download thread is responsible for downloading data from remote peer.
void* ptp_download(void* arg){
    // terminate this thread
    pthread_detach(pthread_self());
    pthread_exit(0);
}

// file_monitor thread monitors a local file directory
// it sends out updated file table to the tracker if any file changes in the local file directory.
void* file_monitor(void* arg){
    
}

// keep_alive thread sends out heartbeat messages to tracker periodically.
void* keep_alive(void* arg){
    // terminate this thread
    while (1){
        sleep(HEARTBEAT_INTERVAL);
        
    }
}

void listen_thread_init(){
    // create a new socket for listening
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 6)) == -1) {
        perror("socket creation error\n");
        exit(1);
    }
    
    struct sockaddr_in server_addr;
    
    /* Prepare the socket address structure of the server */
    server_addr.sin_family = AF_INET;       // host byte order
    server_addr.sin_port = htons(P2P_PORT);   // short, network byte order
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // automatically fill with my IP address "localhost"
    memset(&(server_addr.sin_zero), '\0', 8); // zero the rest of the struct
    
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind error\n");
        exit(1);
    }
    
    if (listen(listen_fd, MAX_PEER_NUM) == -1) {
        perror("listen error\n");
        exit(1);
    }
    
    pthread_t listen_to_peer_thread;
    pthread_create(&listen_to_peer_thread, NULL, listen_to_peer, (void*)0);
}

void ft_destroy(){
    
}

void peer_stop(){
    
}

int connect_to_tracker(){
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 6)) == -1) {
        perror("socket creation error\n");
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(HANDSHAKE_PORT);
    server_addr.sin_addr.s_addr = inet_addr(TRACKER_IP);  // IP address of tracker
    
    if ((connect(sockfd, (struct sockaddr *)&(server_addr), sizeof(struct sockaddr))) == -1) {
        perror("connect error\n");
        return -1;
    }
    
    return sockfd;
}


int main(int argc, const char * argv[]) {
    printf("Peer: initializing...\n");
    
    //register a signal handler which is sued to terminate the process
    signal(SIGINT, peer_stop);
    
    // connect to tracker
    conn = connect_to_tracker();
    
    // send a REGISTER packet
    file_table = filetable_create();
    
    listen_thread_init();
    
    // initialize file_monitor thread
    pthread_t file_monitor_thread;
    pthread_create(&file_monitor_thread, NULL, file_monitor, (void*)0);
    
    // initialize keep_alive thread
    pthread_t keep_alive_thread;
    pthread_create(&keep_alive_thread, NULL, keep_alive, (void*)0);

    
    return 0;
}
