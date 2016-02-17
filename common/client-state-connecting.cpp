
#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif


#include <stdlib.h>
#include <cstdlib>
#include <time.h>
#include <string.h>
#include <sstream>

#include "servant.h"
#include "log.h"
#include "network.h"

using namespace std;
using namespace std::chrono;
using namespace cxm::util;

#define CONNECTING_RETRY_MILS 20000

namespace cxm {
namespace p2p {

ClientStateConnecting::ClientStateConnecting(ServantClient *client) : ClientState(SERVANT_CLIENT_CONNECTING, client)
{
    mlastReplyRequestTime = system_clock::from_time_t(0);
    mlastReplyConnectTime = system_clock::from_time_t(0);
    mstartTime = system_clock::now();

    // first start right now
    // this->OnTimer();
	mtimer = shared_ptr<Timer>(new Timer(this, milliseconds(CONNECTING_RETRY_MILS)));
}

ClientStateConnecting::~ClientStateConnecting()
{
    if (PClient->GetState() != SERVANT_CLIENT_CONNECTED) {
        LOGD("Destroy ClientStateConnecting to disconnect at state: %d",
                PClient->GetState());
        this->Disconnect();
    }
}

int ClientStateConnecting::Connect()
{
    LOGD("Connecting state start connect with the role: %d", PClient->mpeerRole);
    if (CXM_P2P_PEER_ROLE_SLAVE == PClient->mpeerRole) {
        // slave do not connect initiative
        // at this moment, the login state has already saved remote peer info
        this->OnReplyConnect();
    }

    // master start a time to connect initative
    // time also used to check connecting timeout
    this->OnTimer();
    mtimer->Start(true);

    return 0;
}

void ClientStateConnecting::Disconnect()
{
    if (NULL == mtimer.get())
        return;

    unique_lock<mutex> lock(mmutex);

    mtimer->Stop();
    mtimer.reset();

    shared_ptr<ClientState> oldState =
        PClient->SetStateInternal(SERVANT_CLIENT_LOGIN);
}

void ClientStateConnecting::OnTimer()
{
    // check timeout
    milliseconds delta = duration_cast<milliseconds>(
            system_clock::now() - mstartTime);
    LOGD("The connnect time delta %d", (int)delta.count());
    if (delta.count() > PClient->mconnectingTimeoutMils) {
        LOGE("Connecting timeout at mils %d, give up without lock", (int)delta.count());
        PClient->Disconnect();
        LOGD("Connecting timeout disconnect complete");
        return;
    }

    // do not send request to p2p server for the slave role
    if (PClient->mpeerRole == CXM_P2P_PEER_ROLE_SLAVE) {
        LOGD("Do not send connect request because of the slave role");
        return;
    }

    unique_lock<mutex> lock(mmutex);

    // send connect message to server
    // so that server can send the message back to the two peers
    Message msg;
    msg.type = CXM_P2P_MESSAGE_CONNECT;
    strncpy(msg.u.client.clientName, PClient->mname.c_str(), CLIENT_NAME_LENGTH);
    strncpy(msg.u.client.uc.connect.remoteName, PClient->mremotePeer.c_str(), CLIENT_NAME_LENGTH);

    int res = PClient->mtransport->SendTo(PClient->mserverCandidate,
            (uint8_t *)&msg, sizeof(Message));
    if (0 == res)
        LOGD("Sending connecting command to server");
    else
        LOGE("Send connecing command to server failed: %d", res);
}

int ClientStateConnecting::OnMessage(shared_ptr<ReceiveMessage> message)
{
	switch (message->GetMessage()->type) {
    case CXM_P2P_MESSAGE_REPLY_CONNECT: {
		milliseconds delta = duration_cast<milliseconds>(
                system_clock::now() - mlastReplyConnectTime);
        if (delta.count() < CONNECTING_RETRY_MILS) {
            LOGW("Ignore mulit REPLY_REQUEST at delta mils: %d", (int)delta.count());
            return -1;
        }
        mlastReplyConnectTime = system_clock::now();

        PClient->mpeerRole = (CXM_P2P_PEER_ROLE_T)
            message->GetMessage()->u.client.uc.replyConnect.peerRole;
        PClient->PeerCandidate = shared_ptr<Candidate>(new Candidate(
                    message->GetMessage()->u.client.uc.replyConnect.remoteIp,
                    message->GetMessage()->u.client.uc.replyConnect.remotePort));

        LOGD("Receive REPLY_CONNECT with role %d at peer candidate %s",
                PClient->mpeerRole, PClient->PeerCandidate->ToString().c_str());

        this->OnReplyConnect();

		return 0;
	} case CXM_P2P_MESSAGE_DO_P2P_CONNECT: {
        // update remote peer candidate if in SYM NAT
		if (!PClient->PeerCandidate->Equal(message->GetRemoteCandidate())) {
            LOGI("Receive DO_P2P from different candidate at local %s, update %s to %s",
				message->GetLocalCandidate()->ToString().c_str(),
				PClient->PeerCandidate->ToString().c_str(),
				message->GetRemoteCandidate()->ToString().c_str());
			PClient->PeerCandidate = message->GetRemoteCandidate();
		}

        int ttl = 64;
        int error = setsockopt(PClient->mtransport->GetSocket(),
                IPPROTO_IP, IP_TTL, (const char *)&ttl, sizeof(int));
        LOGD("Set socket ttl to %d with error %d", ttl, error);

        for (int i = 0; i < 3; i++) {
            // send p2p connect
            Message msg;
            memset(&msg, 0, sizeof(msg));
            msg.type = CXM_P2P_MESSAGE_REPLY_P2P_CONNECT;
            msg.u.p2p.up.p2pReply.yourPrivatePort = message->GetMessage()->u.p2p.up.p2p.myPrivatePort;
            strncpy(msg.u.p2p.up.p2pReply.key, SERVANT_P2P_REPLY_MESSAGE, CLIENT_NAME_LENGTH);

            int res = PClient->mtransport->SendTo(PClient->PeerCandidate,
                    (uint8_t *)&msg, sizeof(Message));
            if (0 != res)
                LOGE("Cannot send reply p2p connect to %s: %d",
                        PClient->PeerCandidate->ToString().c_str(), res);
            else
                LOGI("Receiving DO_P2P_CONNECT command from peer %s private port %d, "
                        "send back REPLY_P2P_CONNECT with key: %s via local candidate %s",
                        PClient->PeerCandidate->ToString().c_str(),
                        msg.u.p2p.up.p2pReply.yourPrivatePort,
                        SERVANT_P2P_REPLY_MESSAGE,
                        PClient->mtransport->GetLocalCandidate()->ToString().c_str());
        }

		return 0;
    } case CXM_P2P_MESSAGE_REPLY_P2P_CONNECT: {
        int privatePort = message->GetMessage()->u.p2p.up.p2pReply.yourPrivatePort;
        LOGI("Receive REPLY_P2P from different candidate via private port %d, update %s to %s",
                privatePort,
                PClient->PeerCandidate->ToString().c_str(),
                message->GetRemoteCandidate()->ToString().c_str());
        if (!PClient->PeerCandidate->Equal(message->GetRemoteCandidate())) {
            PClient->PeerCandidate = message->GetRemoteCandidate();
        }
        int res = PClient->mtransport->UpdateLocalCandidateByPort(privatePort);
        if (0 != res) {
            LOGE("Cannot update transport candidate %s: %d",
                    message->GetLocalCandidate()->ToString().c_str(), res);
        }

        for (int i = 0; i < 3; i++) {
            // send p2p connect to remote candidate again
            Message msg;
            memset(&msg, 0, sizeof(msg));
            msg.type = CXM_P2P_MESSAGE_REPLY_P2P_CONNECT;
            strncpy(msg.u.p2p.up.p2pReply.key, SERVANT_P2P_REPLY_MESSAGE, CLIENT_NAME_LENGTH);

            int res = PClient->mtransport->SendTo(PClient->PeerCandidate,
                    (uint8_t *)&msg, sizeof(Message));
            if (0 != res)
                LOGE("Cannot send REPLY_P2P_CONNECT to %s: %d",
                        PClient->PeerCandidate->ToString().c_str(), res);
            else
                LOGI("Receiving REPLY_P2P_CONNECT command from peer %s, "
                        " send again REPLY_P2P_CONNECT with key: %s via local candidate %s",
                        PClient->PeerCandidate->ToString().c_str(),
                        SERVANT_P2P_REPLY_MESSAGE,
                        PClient->mtransport->GetLocalCandidate()->ToString().c_str());
        }

        LOGI("P2P connection establis successfully between local %s and peer %s",
                PClient->mtransport->GetLocalCandidate()->ToString().c_str(),
                PClient->PeerCandidate->ToString().c_str());

        // stop timer
        if (NULL != this->mtimer.get())
            this->mtimer->Stop();

        // hold on this reference to prevent self deleted
        shared_ptr<ClientState> oldState =
            PClient->SetStateInternal(SERVANT_CLIENT_CONNECTED);

        // start peer keep alive
        PClient->StartPeerKeepAlive();

        // fire notify
        PClient->FireOnConnectNofity();

        return 0;
    } default: {
		LOGE("Unwant message at connecting state: %d", message->GetMessage()->type);
		return -1;
	}
	}
}

void ClientStateConnecting::Logout()
{
	// stop timer
	if (NULL != this->mtimer.get())
		this->mtimer->Stop();

	// change to logouting state
	shared_ptr<ClientState> oldState =
		PClient->SetStateInternal(SERVANT_CLIENT_LOGOUTING);
	// resend logout event
	PClient->meventThread->PutEvent(ServantClient::SERVANT_CLIENT_EVENT_LOGOUT);
}

void ClientStateConnecting::OnReplyConnect()
{
    assert(NULL != PClient->PeerCandidate.get() &&
            0 != PClient->PeerCandidate->Ip() &&
            0 != PClient->PeerCandidate->Port());

    // after server send this message, connect to the real peer
    // send p2p connect to remote client
    Message msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = CXM_P2P_MESSAGE_DO_P2P_CONNECT;
    strncpy(msg.u.p2p.up.p2p.key, SERVANT_P2P_MESSAGE, CLIENT_NAME_LENGTH);

    if (CXM_P2P_PEER_ROLE_SLAVE == PClient->mpeerRole) {
        // slave peer use short TTL udp packet to open the port
        int ttl = 4;
        int error = setsockopt(PClient->mtransport->GetSocket(),
                IPPROTO_IP, IP_TTL, (const char *)&ttl, sizeof(int));
        LOGD("Set socket ttl to %d with error %d", ttl, error);

        GenerateGuessList(shared_ptr<Candidate>(
                    new Candidate(PClient->PeerCandidate->Ip(),
                        PClient->PeerCandidate->Port())), 320, 500);

        for (auto iter = mcandidateGuessList.begin();
                iter != mcandidateGuessList.end(); iter++) {
            for (int j = 0; j < 2; j++) {
                shared_ptr<Candidate> sendCandidate = *iter;
                int res = PClient->mtransport->SendTo(sendCandidate,
                        (uint8_t *)&msg, sizeof(Message));
                if (0 != res)
                    LOGE("Cannot send p2p connect to remote %s: %d at times %d",
                            sendCandidate->ToString().c_str(), res, j);
            }
        }
    } else {
        PClient->mtransport->CloseCandidateListWithoutMaster();
        GenerateGuessList(shared_ptr<Candidate>(new Candidate(0,
                        PClient->PeerCandidate->Port())), 220, 500);

        for (auto iter = mcandidateGuessList.begin();
                iter != mcandidateGuessList.end(); iter++) {
            int res = PClient->mtransport->AddLocalCandidate((*iter));
            if (0 != res)
                LOGE("Cannot add candidate: %s",
                        (*iter)->ToString().c_str());
        }

        LOGI("Open multi candidates to punch remote peer %s, size %d asleep to punch",
                PClient->PeerCandidate->ToString().c_str(), 220);
        Thread::Sleep(500);

        // save the origin master port for restore
        int masterPort = PClient->mtransport->GetLocalCandidate()->Port();

        for (int j = 0; j < 2; j++) {
            // master peer connect directly
            for (auto iter = PClient->mtransport->GetCandiateList().begin();
                    iter != PClient->mtransport->GetCandiateList().end();
                    iter++) {
                PClient->mtransport->UpdateLocalCandidateByPort(
                        (*iter)->MCandidate->Port());

                msg.u.p2p.up.p2p.myPrivatePort = (*iter)->MCandidate->Port();
                int res = PClient->mtransport->SendTo(PClient->PeerCandidate,
                        (uint8_t *)&msg, sizeof(Message));
                if (0 != res)
                    LOGE("Cannot send p2p connect to remote %s: %d at times %d",
                            (*iter)->MCandidate->ToString().c_str(), res, j);
            }
        }

        PClient->mtransport->UpdateLocalCandidateByPort(masterPort);
    }
}

void ClientStateConnecting::GenerateGuessList(
        shared_ptr<Candidate> candidate, int generateSize, int minPort)
{
    srand(time(NULL));
    mcandidateGuessList.clear();

    // port - 10 ~ port + 10, but port at the first one
    mcandidateGuessList.push_back(shared_ptr<Candidate>(
                new Candidate(candidate->Ip(), candidate->Port())));
    for (int i = -5; i < 0; i++)
        mcandidateGuessList.push_back(shared_ptr<Candidate>(
                    new Candidate(candidate->Ip(), candidate->Port() + i)));
    for (int i = 1; i <= 5; i++)
        mcandidateGuessList.push_back(shared_ptr<Candidate>(
                    new Candidate(candidate->Ip(), candidate->Port() + i)));

    while (mcandidateGuessList.size() < generateSize) {
        int randPort = (rand() + rand()) % 65536;
        if (randPort < minPort)
            continue;

        bool find = false;
        for (auto iter = mcandidateGuessList.begin();
                iter != mcandidateGuessList.end(); iter++)
            if ((*iter)->Port() == randPort) {
                find = true;
                break;
            }
        if (find)
            continue;

        mcandidateGuessList.push_back(shared_ptr<Candidate>(
                    new Candidate(candidate->Ip(), randPort)));
    }

    ostringstream ss;
    ss << "The generated port list: ";
    for (auto iter = mcandidateGuessList.begin();
            iter != mcandidateGuessList.end(); iter++)
        ss << (*iter)->Port() << " ";

#if 0
    LOGD("%s", ss.str().c_str());
#endif
}

}
}
