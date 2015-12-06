#ifndef _CXM_P2P_MESSAGE_H_
#define _CXM_P2P_MESSAGE_H_

#include <stdint.h>
#include <memory>

#include "udp.h"
#include "candidate.h"
#include "transceiver.h"

#define CLIENT_NAME_LENGTH 31

typedef enum {
	CXM_P2P_MESSAGE_UNKNOWN = 0,
	CXM_P2P_MESSAGE_LOGIN,
	CXM_P2P_MESSAGE_LOGIN_REPLY,
	CXM_P2P_MESSAGE_LOGOUT,
	CXM_P2P_MESSAGE_LOGOUT_REPLY,
	CXM_P2P_MESSAGE_REQUEST,
	CXM_P2P_MESSAGE_REPLY_REQUEST,
	CXM_P2P_MESSAGE_CONNECT,
	CXM_P2P_MESSAGE_REPLY_CONNECT,
	CXM_P2P_MESSAGE_DO_P2P_CONNECT,
	CXM_P2P_MESSAGE_REPLY_P2P_CONNECT,
	CXM_P2P_MESSAGE_USER_DATA,
	CXM_P2P_MESSAGE_COUNT // remain last
} CXM_P2P_MESSAGE_TYPE;

typedef enum {
	CXM_P2P_ROLE_SERVER, // message send by p2p server
	CXM_P2P_ROLE_CLIENT, // message send from peer to p2p server
	CXM_P2P_ROLE_P2P // message send from peer to peer
} CXM_P2P_ROLE_T;

typedef enum {
	CXM_P2P_REPLY_REQUEST_RESULT_OK = 1,
	CXM_P2P_REPLY_REQUEST_RESULT_UNKNOWN_PEER
} CXM_P2P_REPLY_REQUEST_RESULT_T;

struct Message
{
	uint16_t type;
	uint8_t role;

	Message()
	{
		memset(this, 0, sizeof(Message));
	}

	union {
		struct {

		} server;
		struct {
			char clientName[CLIENT_NAME_LENGTH + 1];
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
					uint8_t result;
					uint32_t remoteIp;
					uint16_t remotePort;
				} replyRequest;
				struct {
					char remoteName[CLIENT_NAME_LENGTH + 1];
				} connect;
			} uc;
		} client;
		struct {
			union {
				uint16_t userDataLength;
				struct {
					char key[CLIENT_NAME_LENGTH + 1];
				} p2p;
				struct {
					char key[CLIENT_NAME_LENGTH + 1];
				} p2pReply;
			} up;
		} p2p;
	} u;
};

#if 0
int MessageSend(std::shared_ptr<cxm::p2p::TransceiverU> transport, std::shared_ptr<Candidate> remote, Message *pmsg);
std::shared_ptr<Candidate> MessageReceive(std::shared_ptr<cxm::p2p::TransceiverU> transport, Message *pmsg);
#endif
int MessageValidate(const Message *pmsg);
#if 0
std::shared_ptr<Candidate> MessageCandidate(const Message *pmsg);
#endif
#endif
