#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

// Special message types for network communication
// 9xx range reserved for system/protocol messages

#define PING_MSG_TYPE               999
#define PONG_MSG_TYPE               998
#define ROLE_MSG_TYPE               997
#define BRIDGE_ANNOUNCE_MSG_TYPE    996
#define NODE_COUNT_MSG_TYPE         995
#define TOTAL_LED_COUNT_MSG_TYPE    994
#define SWEEP_SPOT_MSG_TYPE         993

#endif // MESSAGE_TYPES_H
