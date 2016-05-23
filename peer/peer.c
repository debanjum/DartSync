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

// ptp_listening thread: parse data request from remote peer
int recv_data_request(int conn, char* filename, int* pieceNum){
    ptp_request_t request_pkt;
    char buf[sizeof(ptp_request_t)+2];
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
                memmove(&request_pkt, buf, sizeof(ptp_request_t));
                strcpy(filename, request_pkt.filename);
                *pieceNum = request_pkt.pieceNum;
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

// ptp_upload thread: peer wraps file data and send it to remote peer
int upload_send(int conn, char * file_path, int pieceNum){
    FILE *fp;
    if ((fp = fopen(file_path, "r")) == NULL){
        printf("Cannot find %s!\n", file_path);
        return -1;
    }
    
    ptp_data_pkt_t *pkt = (ptp_data_pkt_t *)malloc(sizeof(ptp_data_pkt_t));
    memset(pkt, 0, sizeof(ptp_data_pkt_t));
    fseek(fp, pieceNum * sizeof(char) * MAX_DATA_LEN, SEEK_SET); // pieceNum starts from 0, 1, 2...
    fread(pkt->data, sizeof(char), MAX_DATA_LEN, fp);
    
    pkt->pieceNum = pieceNum;
    pkt->size = strlen(pkt->data);
    
    if (ptp_sendpkt(conn, pkt) < 0){
        return -1;
    }
    
    fclose(fp);
    return 1;
}

// ptp_download thread: send data request to remote peer
int download_send(int conn, ptp_request_t *request_pkt){
    char bufstart[2];
    char bufend[2];
    bufstart[0] = '!';
    bufstart[1] = '&';
    bufend[0] = '!';
    bufend[1] = '#';
    
    if (send(conn, bufstart, 2, 0) < 0) {
        free(request_pkt);
        return -1;
    }
    if(send(conn, request_pkt, sizeof(ptp_request_t), 0) < 0) {
        free(request_pkt);
        return -1;
    }
    if(send(conn, bufend, 2, 0) < 0) {
        free(request_pkt);
        return -1;
    }
    printf("~> sent data request to peer\n");
    free(request_pkt);
    return 1;
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
        
        // receive a data request pkt from remote peer
        char filename[FILE_NAME_LEN];
        int pieceNum;
        recv_data_request(conn, filename, &pieceNum);
        
        // parse the request info
        upload_arg_t *arg = (upload_arg_t *)malloc(sizeof(upload_arg_t));
        arg->sockfd = conn;
        arg->pieceNum = pieceNum;
        
        // change filename into file path
        char file_path[FILE_NAME_LEN];
        strcpy(file_path, WATCHING);
        strcat(file_path, filename);
        printf("data request: file_path = %s\n", file_path);
        strcpy(arg->file_path, file_path);
        
        // create a ptp_upload thread
        pthread_t ptp_upload_thread;
        pthread_create(&ptp_upload_thread, NULL, ptp_upload, (void*)(arg));
        printf("ptp_listening: created a ptp_upload thread with sockfd = %d!\n", conn);
    }
}

// ptp_upload thread is started by listen_to_peer thread and responsible for uploading data to remote peer.
void* ptp_upload(void* arg){
    upload_arg_t * upload_arg = (upload_arg_t *)arg;
    int sockfd =  upload_arg -> sockfd;
    char *file_path = upload_arg -> file_path;
    int pieceNum = upload_arg -> pieceNum;

    // send data to remote peer through *sockfd*
    upload_send(sockfd, file_path, pieceNum);
    printf("Peer_upload: sent data pkt to remote peer!\n");
    
    close(sockfd);
    free(arg);
    
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

    // send data request through *sockfd*, specify filename and pieceNum
    ptp_request_t *request_pkt = (ptp_request_t *)malloc(sizeof(ptp_request_t));
    request_pkt->filename =
    request_pkt->pieceNum =
    download_send(sockfd, request_pkt);
    
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
