//
//  test.c
//  DartSync_Xcode
//
//  Created by Lexin Tang on 5/23/16.
//  Copyright © 2016 Lexin Tang. All rights reserved.
//

#include "peer.h"
int ptp_listen_fd;

// each peer listening at P2P_PORT

int main(int argc, const char * argv[]){
    // ptp_listen_thread_init()
    
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
    
    // specify a file to be transfered, downloading a file from a remote peer
    
    //
    download_arg_t* download_arg = (download_arg_t *)malloc(sizeof(download_arg_t));
    
    strcpy(download_arg->filename, "send_this_text.txt");
    // address list of available peers
    /*
    FILE *f = fopen("send_this_text.txt", "r");
    if (f == NULL){
        printf("cannot open file!\n");
    }
    fseek(f, 0, SEEK_END);
    download_arg->size = (unsigned long)ftell(f);
    printf("file size = %lu", download_arg->size);
    fclose(f);
     */
    download_arg->size = 37170;
    download_arg->peerNum = 3;
    
    for (int i = 0; i < MAX_PEER_NUM; i++){
        memset(&download_arg->addr_list[i], 0, sizeof(struct sockaddr_in));
    }
    download_arg->addr_list[0].sin_family = AF_INET;
    download_arg->addr_list[0].sin_addr.s_addr = inet_addr("129.170.214.100");  // carter
    download_arg->addr_list[0].sin_port = htons(P2P_PORT);
    download_arg->addr_list[1].sin_family = AF_INET;
    download_arg->addr_list[1].sin_addr.s_addr = inet_addr("129.170.214.160");  // wildcat
    download_arg->addr_list[1].sin_port = htons(P2P_PORT);
    download_arg->addr_list[2].sin_family = AF_INET;
    download_arg->addr_list[2].sin_addr.s_addr = inet_addr("129.170.213.32");   // bear
    download_arg->addr_list[2].sin_port = htons(P2P_PORT);
    
    // create a ptp_download thread
    pthread_t ptp_download_thread;
    pthread_create(&ptp_download_thread, NULL, ptp_download, (void*)(download_arg));
    
    pthread_join(ptp_download_thread, NULL);
    return 0;
}
