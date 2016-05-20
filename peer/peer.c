//
//  main.c
//  peer
//
//  Created by Lexin Tang on 5/18/16.
//  Copyright © 2016 Lexin Tang. All rights reserved.
//

#include "peer.h"

int ptp_listen_fd, tracker_conn;
file_t *file_table;

// this function is used to initialize file table
file_t *filetable_create(){
    file_t *ft = (file_t *)malloc(sizeof(file_t));
    return ft;
}

// ptp_listening thread keeps receiving data requests from other peers. It handles data requests by creating a P2P upload thread.
// ptp_listening thread is started after the peer is registered.
void* ptp_listening(void* arg){
    int conn;
    socklen_t sin_size;
    sin_size = sizeof(struct sockaddr_in);
    
    while (1){
        struct sockaddr_in *client_addr;
        client_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        if ((conn = accept(ptp_listen_fd, (struct sockaddr *)(client_addr), &sin_size)) == -1) {
            perror("accept error\n");
            exit(1);
        }
        printf("ptp_listening: received a peer request!\n");
        
        int *sockfd = (int *)malloc(sizeof(int));
        *sockfd = conn;
        // create a ptp_upload thread
        pthread_t ptp_upload_thread;
        pthread_create(&ptp_upload_thread, NULL, ptp_upload, (void*)(sockfd));
        printf("ptp_listening: created a ptp_upload thread with sockfd = %d!\n", conn);
    }
}

// ptp_upload thread is started by listen_to_peer thread and responsible for uploading data to remote peer.
void* ptp_upload(void* arg){
    //struct sockaddr_in * peer_addr = (struct sockaddr_in *) arg;
    int sockfd = * ((int *)arg);
    free(arg);

    printf("Peer: start sending data to ptp_download thread of remote peer!\n");
    
    // send data to remote peer through *sockfd*
    
    
    
    close(sockfd);
    
    // terminate this thread
    pthread_detach(pthread_self());
    pthread_exit(0);
}

// ptp_download thread is responsible for downloading data from remote peer.
// ptp_download thread is started after peer received a BROADCAST pkt and found difference between its local file table with the broadcasted file table.
void* ptp_download(void* arg){
    struct sockaddr_in * peer_addr = (struct sockaddr_in *) arg;
    
    // start connection with ptp_listening thread of remote peer
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 6)) == -1) {
        perror("socket creation error\n");
        exit(1);
    }
    
    peer_addr->sin_port = htons(P2P_PORT);    // ptp_listening port
    if ((connect(sockfd, (struct sockaddr *)&(peer_addr), sizeof(struct sockaddr))) == -1) {
        perror("connect error\n");
        exit(1);
    }
    printf("Peer: successfully connect to ptp_listening thread of remote peer!\n");
    
    free(peer_addr);

    // receive data from remote peer through *sockfd*
    while (1) {
        
    }
    
    close(sockfd);
    
    // terminate this thread
    pthread_detach(pthread_self());
    pthread_exit(0);
}

// file_monitor thread monitors a local file directory
// it sends out updated file table to tracker if any file changes in the local file directory.
void* file_monitor(void* arg){
    
}

// keep_alive thread sends out heartbeat messages to tracker periodically.
void* keep_alive(void* arg){
    
    while (1){
        sleep(HEARTBEAT_INTERVAL);
        
        // send HEARTBEAT pkt to tracker
        
        
    }
}

void ptp_listen_thread_init(){
    // create a new socket for listening
    if ((ptp_listen_fd = socket(AF_INET, SOCK_STREAM, 6)) == -1) {
        perror("socket creation error\n");
        exit(1);
    }
    
    struct sockaddr_in server_addr;
    
    /* Prepare the socket address structure of the server */
    server_addr.sin_family = AF_INET;       // host byte order
    server_addr.sin_port = htons(P2P_PORT);   // short, network byte order
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // automatically fill with my IP address "localhost"
    memset(&(server_addr.sin_zero), '\0', 8); // zero the rest of the struct
    
    if (bind(ptp_listen_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind error\n");
        exit(1);
    }
    
    if (listen(ptp_listen_fd, MAX_PEER_NUM) == -1) {
        perror("listen error\n");
        exit(1);
    }
    
    pthread_t ptp_listening_thread;
    pthread_create(&ptp_listening_thread, NULL, ptp_listening, (void*)0);
}

void ft_destroy(){
    
}

void peer_stop(){
    ft_destroy();
    close(ptp_listen_fd);
    close(tracker_conn);
}

// this function is used to start connection to tracker, send REGISTER pkt and wait for ACCEPT pkt.
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
    
    // send a REGISTER pkt
    
    
    // wait for ACCEPT pkt from tracker
    
    
    return sockfd;
}

int main(int argc, const char * argv[]) {
    printf("Peer: initializing...\n");
    
    // register a signal handler which is sued to terminate the process
    signal(SIGINT, peer_stop);
    
    // initialize file table
    file_table = filetable_create();
    
    // connect to tracker, register and accept
    tracker_conn = connect_to_tracker();
    printf("Peer: successfully connect to tracker!\n");
    
    // initialize listen_to_peer thread
    ptp_listen_thread_init();
    
    // initialize file_monitor thread
    pthread_t file_monitor_thread;
    pthread_create(&file_monitor_thread, NULL, file_monitor, (void*)0);
    
    // initialize keep_alive thread
    pthread_t keep_alive_thread;
    pthread_create(&keep_alive_thread, NULL, keep_alive, (void*)0);
    
    // (file_monitor thread) send a HANDSHAKE pkt filled with file table
    
    // (main thread) 1.keep receiving BROADCAST pkt from tracker
    // 2. compare the received file table with local file table
    // 3. create a ptp_download thread for each different file and update peer table
    // 4. (ptp_download thread) send HANDSHAKE pkt to tracker
    while (1){
        
    }
    
    return 0;
}
