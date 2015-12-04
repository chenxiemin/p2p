#ifndef _CXM_P2P_MESSAGE_H_
#define _CXM_P2P_MESSAGE_H_

#include <stdint.h>
#include <memory>

#include "udp.h"
#include "candidate.h"

#define CLIENT_NAME_LENGTH 31

typedef enum {
	CXM_P2P_MESSAGE_UNKNOWN = -1,
	CXM_P2P_MESSAGE_LOGIN = 0,
	CXM_P2P_MESSAGE_LOGOUT = 1,
	CXM_P2P_MESSAGE_REQUEST = 2,
	CXM_P2P_MESSAGE_REPLY_REQUEST = 3,
	CXM_P2P_MESSAGE_CONNECT = 4,
	CXM_P2P_MESSAGE_REPLY_CONNECT = 5,
	CXM_P2P_MESSAGE_DO_P2P_CONNECT = 6,
	CXM_P2P_MESSAGE_REPLY_P2P_CONNECT = 7,
	CXM_P2P_MESSAGE_COUNT // remain last
} CXM_P2P_MESSAGE_TYPE;

typedef struct
{
	uint16_t type;
	char clientName[CLIENT_NAME_LENGTH + 1];
	uint32_t clientIp; // client ip & port within public network
	uint16_t clientPort;

	union {
		struct {
			uint32_t clientPrivateIp;
			uint16_t clientPrivatePort;
		} login;
		struct {

		} logout;
		struct {
			char remoteName[CLIENT_NAME_LENGTH + 1];
		} request;
		struct {
			uint32_t remoteIp;
			uint16_t remotePort;
		} replyRequest;
		struct {
			char remoteName[CLIENT_NAME_LENGTH + 1];
		} connect;
		struct {
			char key[CLIENT_NAME_LENGTH + 1];
		} p2p;
		struct {
			char key[CLIENT_NAME_LENGTH + 1];
		} p2pReply;
	} umsg;
} Message;

int MessageSend(Socket socket, Message *pmsg, std::shared_ptr<Candidate> remote);
int MessageReceive(Socket socket, Message *pmsg);
int MessageValidate(const Message *pmsg);
std::shared_ptr<Candidate> MessageCandidate(const Message *pmsg);


#endif
