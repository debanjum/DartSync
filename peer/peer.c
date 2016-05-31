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
    ft->head = NULL;
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
        
        // receive a data request pkt from remote peer and parse the request info
        char filename[FILE_NAME_LEN];
        upload_arg_t *arg = (upload_arg_t *)malloc(sizeof(upload_arg_t));
        arg->sockfd = conn;
        recv_data_request(conn, filename, &arg->pieceNum, &arg->offset, &arg->size);
        
        // change filename into file path
        char file_path[FILE_NAME_LEN];
        strcpy(file_path, readConfigFile("config.data"));
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
    unsigned long offset = upload_arg->offset;
    unsigned long size = upload_arg->size;
    // send data to remote peer through *sockfd*
    upload_send(sockfd, file_path, pieceNum, offset, size);
    printf("Peer_upload: successfully sent a piece of file to remote peer!\n");
    
    close(sockfd);
    free(arg);
    
    // terminate this thread
    pthread_detach(pthread_self());
    pthread_exit(0);
}

// piece_download thread is for downloading one piece of file from a remote peer.
void* piece_download(void* arg){
    piece_download_arg_t *piece_arg = (piece_download_arg_t *)arg;
    char filename[FILE_NAME_LEN];
    strcpy(filename, piece_arg->filename);
    
    // start connection with ptp_listening thread of remote peer
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 6)) == -1) {
        perror("socket creation error\n");
        exit(1);
    }
    
    struct sockaddr_in peer_addr;
    memmove(&peer_addr, &piece_arg->peer_addr, sizeof(struct sockaddr_in));
    peer_addr.sin_port = htons(P2P_PORT);  // ptp_listening port
    
    if ((connect(sockfd, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr))) == -1) {
        perror("connect error\n");
        exit(1);
    }
    
    printf("Peer: successfully connect to ptp_listening thread of remote peer!\n");

    // send data request through *sockfd*, specify filename and pieceNum
    ptp_request_t *request_pkt = (ptp_request_t *)malloc(sizeof(ptp_request_t));
    strcpy(request_pkt->filename, filename);
    request_pkt->pieceNum = piece_arg->pieceNum;
    request_pkt->offset = piece_arg->offset;
    request_pkt->size = piece_arg->size;
    
    download_send(sockfd, request_pkt);
    
    int size = (int) piece_arg->size;
    int pkt_count = size / MAX_DATA_LEN;
    int remainder_len = size % MAX_DATA_LEN;
    if (remainder_len > 0){
        pkt_count++;
    }
    printf("piece_arg->size = %d\n", size);
    char temp_filename[FILE_NAME_LEN];
    memset(temp_filename, 0, FILE_NAME_LEN);
    sprintf(temp_filename, "%s.%s_%d", readConfigFile("config.data"), filename, request_pkt->pieceNum);
    
    FILE* fp = fopen(temp_filename, "a");
    if (fp == NULL){
        printf("cannot open %s\n", temp_filename);
    }
    nanosleep((const struct timespec[]){{0, RECV_INTERVAL}}, NULL);
    
    printf("successfully open %s\n", temp_filename);
    // receive data pkts from remote peer through *sockfd*
    printf("total num of data pkts = %d\n", pkt_count);
    
    char *buffer = (char *)malloc(size);
    memset(buffer, 0, size);
    
    for (int i = 0; i < pkt_count; i++){
        //ptp_data_pkt_t *pkt = (ptp_data_pkt_t *)malloc(sizeof(ptp_data_pkt_t));
        ptp_data_pkt_t pkt;
        memset(&pkt, 0, sizeof(ptp_data_pkt_t));
        printf("ready to receive data pkt id = %d!\n", i);
        ptp_recvpkt(sockfd, &pkt);
        // extract data from pkt and save it into a temporary file
        printf("successfully received data pkt id = %d!\n", i);
        //printf("pkt->size = %lu\n", pkt.size);
        //printf("pkt->pieceNum = %d\n", pkt.pieceNum);
        nanosleep((const struct timespec[]){{0, RECV_INTERVAL}}, NULL);
        memmove(&buffer[i * MAX_DATA_LEN], pkt.data, pkt.size);
        //free(pkt);
    }
    
    fwrite(buffer, size, 1, fp);
    free(buffer);
    fclose(fp);
    close(sockfd);
    free(arg);
    
    // terminate this thread
    pthread_detach(pthread_self());
    pthread_exit(0);
}

// p2p_download thread is responsible for downloading one file from multiple peers.
// ptp_download thread is started after peer received a BROADCAST pkt and found difference between its local file table with the broadcasted file table.
void* ptp_download(void* arg){
    download_arg_t* download_arg = (download_arg_t *)arg;
    char *filename = download_arg->filename;
    struct sockaddr_in* peer_addr_list = download_arg->addr_list;
    unsigned long fileSize = download_arg->size;
    unsigned long pieceSize;
    int remainder_len;
    // divide one file into *peerNum* pieces
    int peerNum;    // total num of pieces
    if (download_arg->peerNum == 1){
        peerNum = 1;
        pieceSize = fileSize;
        remainder_len = 0;
    }
    else if (download_arg->peerNum == 2) {
        peerNum = 2;
        pieceSize = (fileSize + 1) / 2;
        remainder_len = fileSize - pieceSize;
    }
    else {
        peerNum = download_arg->peerNum - 1;
        pieceSize = fileSize / peerNum;
        remainder_len = fileSize % peerNum;
        if (remainder_len > 0){
            peerNum++;
        }
    }
    printf("peerNum = %d\n", peerNum);
    printf("pieceSize = %lu\n", pieceSize);
    printf("remainder_len = %d\n", remainder_len);
    
    // download one file from multiple peers using multi-threading
    pthread_t tid[peerNum];
    for (int i = 0; i < peerNum; i++){
        piece_download_arg_t *piece_arg = (piece_download_arg_t *)malloc(sizeof(piece_download_arg_t));
        strcpy(piece_arg->filename, filename);
        memmove(&piece_arg->peer_addr, &peer_addr_list[i], sizeof(struct sockaddr_in));
        piece_arg->pieceNum = i;
        piece_arg->offset = i * pieceSize;
        piece_arg->size = (remainder_len > 0 && i == peerNum - 1)? remainder_len:pieceSize;
        printf("pieceNum = %d\n", piece_arg->pieceNum);
        printf("piece size = %lu\n", piece_arg->size);
        printf("creating piece_download thread!\n");
        // create a piece_download thread for downloading the i_th piece of the file
        pthread_create(&tid[i], NULL, piece_download, (void*)(piece_arg));
    }
    
    // wait for all piece_download threads to terminate
    for (int i = 0; i < peerNum; i++){
        pthread_join(tid[i], NULL);
    }
    printf("successfully received all pieces!\n");
    // put all recieved pieces together
    // ? use temp file: will be noticed by inotify!! use buffer instead
    char filepath[FILE_NAME_LEN];
    strcpy(filepath, readConfigFile("config.data"));
    strcat(filepath, filename);
    FILE* fp = fopen(filepath, "w");
    for (int i = 0; i < peerNum; i++){
        char temp_filename[FILE_NAME_LEN];
        memset(temp_filename, 0, FILE_NAME_LEN);
        
        sprintf(temp_filename, "%s.%s_%d", readConfigFile("config.data"), filename, i);
        FILE* tmp = fopen(temp_filename, "r");
        if (tmp == NULL){
            printf("cannot open %s!\n", temp_filename);
        }
        fseek(tmp, 0, SEEK_END);
        int fileLen = ftell(tmp);
        fseek(tmp, 0, SEEK_SET);
        printf("fileLen = %d\n", fileLen);
        char *buffer = (char*)malloc(fileLen);
        //char buffer[fileLen];
        fread(buffer, fileLen, 1, tmp);
        fclose(tmp);
        
        // delete temp files
        int ret = remove(temp_filename);
        if (ret != 0) {
            perror("Unable to delete the file");
        }
        
        fwrite(buffer, fileLen, 1, fp);
        printf("going to free buffer!\n");
        free(buffer);
    }
    
    fclose(fp);
    printf("ptp_download: successfully received file %s!\n", filename);

    free(arg);
    
    // update file_table and send HANDSHAKE pkt to tracker
    // no need to do so.. because add-file will be noticed by file monitor
    
    // terminate this thread
    pthread_detach(pthread_self());
    pthread_exit(0);
}

// file_monitor thread monitors a local file directory
// it sends out updated file table to tracker if any file changes in the local file directory.
void* file_monitor(void* arg){
    readConfigFile("config.data");
    watchDirectory(file_table, tracker_conn);
    
    return 0;
}

// add a node to our local file table before downloading this new file from others
void add(download_arg_t *down) {
    // get the head of the local file table
    Node *current = file_table->head;
    Node *prev = NULL;
    // iterate to the end of the linked list to add at the end
    while (current != NULL) {
        prev = current;
        current = current->pNext;
    }

    // set up the node
    current = (Node *)malloc(sizeof(Node));
    current->type = FILE_DOWNLOAD;
    current->size = down->size;
    strcpy(current->name,down->filename);
    current->timestamp = down->timestamp;
    current->pNext = NULL;
    
    if (prev == NULL){
        file_table->head = current;
    }
    else {
        prev->pNext = current;
    }
    
    // fill in the peer ip addresses, only need to store myip in peer side
    strcpy(current->newpeerip[0], getmyip());
}

// modify a node in our local file table before downloading the latest version from others
void modify(download_arg_t *down) {
    // get the head of the local file table
    Node *current = file_table->head;

    // find the node that you are searching for
    while (strcmp(current->name, down->filename) != 0) {
        current = current->pNext;
    }

    // modify the node's information
    current->size = down->size;
    current->timestamp = down->timestamp;
    current->type = FILE_DOWNLOAD;

}

// delete a node from our local file table to reflect changes in the global directory
void delete(Node *find) {
    // modify the node's type to FILE_DELETE
    find->type = FILE_DELETE;
}

// keep_alive thread sends out heartbeat messages to tracker periodically.
void* keep_alive(void* arg){
    while (1){
        sleep(HEARTBEAT_INTERVAL);
        
        // send HEARTBEAT pkt to tracker
        peer_sendpkt(tracker_conn, file_table, KEEP_ALIVE);
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
    Node *ptr = file_table->head;

    // if the pointer does not point to NULL
    if (ptr != NULL) {
        //free(ptr->name);    // free the name of the file
        Node *current = ptr;
        ptr = ptr->pNext;   // get the next node
        free(current);  // free the current node
    }

    // free the entire file table
    free(file_table);
}

void peer_stop(){
    ft_destroy();
    close(ptp_listen_fd);
    close(tracker_conn);
    exit(0);
}

// this function is used to start connection to tracker, send REGISTER pkt and wait for ACCEPT pkt.
int connect_to_tracker(){
    int sockfd;
    // create socket for peer-tracker communications
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 6)) == -1) {
        perror("socket creation error\n");
        return -1;
    }

    // connect to tracker
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(HANDSHAKE_PORT);
    server_addr.sin_addr.s_addr = inet_addr(TRACKER_IP);  // IP address of tracker

    printf("connect to tracker");
    if ((connect(sockfd, (struct sockaddr *)&(server_addr), sizeof(struct sockaddr))) == -1) {
        printf("...fail\n");
	return -1;
    }
    printf("...pass\n");

    // send a REGISTER pkt
    printf("send REGISTER packet to tracker");
    if( peer_sendpkt(sockfd, file_table, REGISTER) < 0 ) {
	printf("...fail\n");
	return -1;
    }
    printf("...pass\n");
    
    // wait for ACCEPT pkt from tracker
    printf("accept FILE_TABLE in tracker response");
    if( peer_recvpkt(sockfd, file_table) < 0 ) {
	printf("...fail\n");
	return -1;
    }
    printf("...pass\n");
    
    return sockfd;
}

// this function is used to find a certain node in our file table and modify it (or add it) if
// necessary
void compareNode(Node *seekNode) {

    Node *current = file_table->head;   // get the head of the local file table

    // attempt to find the file
    while (current != NULL && strcmp(current->name, seekNode->name) != 0) {
        current = current->pNext;
    }

    // check if we need to update the file we found
    if (current != NULL) {

        // compare the timestamps and see if the file needs to be updated to a newer version
        if (current->timestamp <= seekNode->timestamp) {

            // if we need to modify the file
             if (seekNode->type != FILE_DELETE && current->timestamp < seekNode->timestamp) {

                download_arg_t* download_arg = (download_arg_t *)malloc(sizeof(download_arg_t));

                // fill in the data for download_arg
                strcpy(download_arg->filename, current->name);

                //int peers = (sizeof(current->newpeerip))/IP_LEN;
                //download_arg->peerNum = peers;  // ? wrong?
                int i;
                for (i = 0; strlen(seekNode->newpeerip[i]) != 0 && i < MAX_PEER_NUM; i++) {
                    //struct sockaddr_in address;
                    //inet_aton(seekNode->newpeerip[i], &address.sin_addr);
                    //download_arg->addr_list[i] = address;
                    download_arg->addr_list[i].sin_family = AF_INET;
                    download_arg->addr_list[i].sin_addr.s_addr = inet_addr(seekNode->newpeerip[i]);
                    download_arg->addr_list[i].sin_port = htons(P2P_PORT);
                }
                download_arg->peerNum = i;
                download_arg->size = seekNode->size;
                download_arg->timestamp = seekNode->timestamp;
                
                modify(download_arg);
                
                // create a ptp_download thread
                pthread_t ptp_download_thread;
                pthread_create(&ptp_download_thread, NULL, ptp_download, (void*)download_arg);
             }

            // otherwise, we need to delete the file
            else if (seekNode->type == FILE_DELETE && current->type != FILE_DELETE){

                delete(current);
                
                // delete the file from the local directory first
                // current->name should be appended to the current sync folder path
                char filepath[FILE_NAME_LEN];
                strcpy(filepath, readConfigFile("config.data"));
                strcat(filepath, current->name);
                int ret = remove(filepath);
                if (ret != 0) {
                    perror("Unable to delete the file");
                }

                
            }
        }
    }

    // we have a new file that we need to download, add a new node in file table and set node type to FILE_DOWNLOADED
    else {

        download_arg_t* download_arg = (download_arg_t *)malloc(sizeof(download_arg_t));

        // fill in the data for download_arg
        strcpy(download_arg->filename, seekNode->name);

        //int peers = (sizeof(current->newpeerip))/IP_LEN;
        //download_arg->peerNum = peers;  // ? wrong?
        int i;
        for (i = 0; strlen(seekNode->newpeerip[i]) != 0 && i < MAX_PEER_NUM; i++) {
            //struct sockaddr_in address;
            //inet_aton(seekNode->newpeerip[i], &address.sin_addr);
            //download_arg->addr_list[i] = address;
            download_arg->addr_list[i].sin_family = AF_INET;
            download_arg->addr_list[i].sin_addr.s_addr = inet_addr(seekNode->newpeerip[i]);
            download_arg->addr_list[i].sin_port = htons(P2P_PORT);
        }
        download_arg->peerNum = i;
        download_arg->size = seekNode->size;
        download_arg->timestamp = seekNode->timestamp;

        add(download_arg);
        
        // create a ptp_download thread
        pthread_t ptp_download_thread;
        pthread_create(&ptp_download_thread, NULL, ptp_download, (void*)download_arg);

    }

}

int main(int argc, const char * argv[]) {
    printf("Peer: initializing...\n");
    
    // register a signal handler which is sued to terminate the process
    signal(SIGINT, peer_stop);
    
    // initialize file table
    file_table = filetable_create();
    
    // connect to tracker, register and accept
    tracker_conn = connect_to_tracker();
    if(tracker_conn < 0) {
	perror("connect_to_tracker");
	return -1;
    }
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
        // receive file table from tracker
        file_t *trackerFileTable = (file_t *)malloc(sizeof(file_t));
        peer_recvpkt(tracker_conn, trackerFileTable);

        // iterate through nodes in tracker's file table and find the files to be updated
        Node *current = trackerFileTable->head;
        while (current != NULL) {
            compareNode(current);
            current = current->pNext;
        }

        // free the table

        Node *ptr = trackerFileTable->head;
        // if the pointer does not point to NULL
        if (ptr != NULL) {
            //free(ptr->name);    // free the name of the file
            Node *current = ptr;
            ptr = ptr->pNext;   // get the next node
            free(current);  // free the current node
        }

        free(trackerFileTable);
    }
    return 0;
}
