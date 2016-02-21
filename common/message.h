#ifndef _CXM_P2P_MESSAGE_H_
#define _CXM_P2P_MESSAGE_H_

#include <stdint.h>
#include <memory>

#include "udp.h"
#include "candidate.h"
#include "transceiver.h"

#define CLIENT_NAME_LENGTH 15

typedef enum {
	CXM_P2P_MESSAGE_UNKNOWN = 0,
	CXM_P2P_MESSAGE_LOGIN,
	CXM_P2P_MESSAGE_LOGIN_SUB,
	CXM_P2P_MESSAGE_LOGIN_REPLY,
	CXM_P2P_MESSAGE_LOGOUT,
	CXM_P2P_MESSAGE_LOGOUT_REPLY,
	CXM_P2P_MESSAGE_CONNECT,
	CXM_P2P_MESSAGE_REPLY_CONNECT,
	CXM_P2P_MESSAGE_DO_P2P_CONNECT,
	CXM_P2P_MESSAGE_REPLY_P2P_CONNECT,
	CXM_P2P_MESSAGE_USER_DATA,
	CXM_P2P_MESSAGE_SERVER_KEEP_ALIVE,
	CXM_P2P_MESSAGE_PEER_KEEP_ALIVE,
	CXM_P2P_MESSAGE_PEER_DISCONNECT,
	CXM_P2P_MESSAGE_COUNT // remain last
} CXM_P2P_MESSAGE_TYPE;

#if 0
typedef enum {
	CXM_P2P_ROLE_SERVER, // message send by p2p server
	CXM_P2P_ROLE_CLIENT, // message send from peer to p2p server
	CXM_P2P_ROLE_P2P // message send from peer to peer
} CXM_P2P_ROLE_T;
#endif

typedef enum {
    CXM_P2P_PEER_ROLE_MASTER, // caller
    CXM_P2P_PEER_ROLE_SLAVE, // callee
	CXM_P2P_PEER_ROLE_COUNT
} CXM_P2P_PEER_ROLE_T;

typedef enum {
	CXM_P2P_REPLY_RESULT_OK = 1,
	CXM_P2P_REPLY_RESULT_UNKNOWN_PEER
} CXM_P2P_REPLY_RESULT_T;

struct Message
{
	uint16_t type;
	// uint8_t role;

	Message()
	{
		memset(this, 0, sizeof(Message));
	}

	union {
		struct {
			char clientName[CLIENT_NAME_LENGTH + 1];
			uint32_t clientPrivateIp;
			uint16_t clientPrivatePort;
		} login;

		struct {
			char clientName[CLIENT_NAME_LENGTH + 1];
		} logout;

		struct {
			char clientName[CLIENT_NAME_LENGTH + 1];
			char remoteName[CLIENT_NAME_LENGTH + 1];
		} connect;

		struct {
			uint32_t remoteIp;
			uint16_t remotePort;
			uint16_t peerRole;
			char remoteName[CLIENT_NAME_LENGTH + 1];
		} replyConnect;

		struct {
			char key[CLIENT_NAME_LENGTH + 1];
			// the private port which the master send the p2p packet
			// at the sym nat, every packet the master send to the slave
			// may be mapped to the different public port
			// the slave can only see the public port of the udp packet
			// this port will be send back from slave to the master
			// in the p2pReply.yourPrivatePort to let the master know
			// which private port the master used can send packet to the slave
			uint16_t masterPrivatePort;
		} p2pConnect;

		struct {
			char key[CLIENT_NAME_LENGTH + 1];
			// the public port which slave see from the master
			uint16_t masterPublicPort;
			// the private port the master send
			// pass through p2pConnect.myPrivatePort
			uint16_t masterPrivatePort;
		} p2pReply;

		struct {
			char clientName[CLIENT_NAME_LENGTH + 1];
		} keepAlive;

		struct {
			uint16_t userDataLength;
		} p2pData;
	} u;
};

int MessageValidate(const Message *pmsg);
#endif

