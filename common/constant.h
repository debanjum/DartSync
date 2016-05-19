//
//  common.h
//  peer
//
//  Created by Lexin Tang on 5/18/16.
//  Copyright © 2016 Lexin Tang. All rights reserved.
//

#ifndef common_h
#define common_h

#define PROTOCOL_LEN 100    // protocol name
#define RESERVED_LEN 512
#define IP_LEN 16
#define FILE_NAME_LEN 256

#define REGISTER 0
#define KEEP_ALIVE 1
#define FILE_UPDATE 2

#define MAX_PEER_NUM 5

#define HANDSHAKE_PORT 3078
#define P2P_PORT 3578

#define HEARTBEAT_INTERVAL 60

#define TRACKER_IP "127.0.0.1"  // Tracker IP

#endif /* common_h */
