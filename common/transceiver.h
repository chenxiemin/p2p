#ifndef _CXM_NET_TRANSPORT_H_
#define _CXM_NET_TRANSPORT_H_

#include <memory>
#include "thread.h"
#include "candidate.h"
#include "udp.h"

namespace cxm {
namespace p2p {

class ReceiveData
{
	private: uint8_t *mpbuffer;
	private: int mlen;
	private: std::shared_ptr<Candidate> mremoteCandidate;

	public: uint8_t *GetBuffer() { return mpbuffer; }
	public: int GetLength() { return mlen; }
	public: std::shared_ptr<Candidate> GetRemoteCandidate() { return mremoteCandidate; }

	public: ReceiveData(int bufferLength)
	{
		this->mpbuffer = new uint8_t[bufferLength];
		this->mlen = bufferLength;
	}

	public: ReceiveData(uint8_t * buf, int bufferLength)
	{
		this->mpbuffer = new uint8_t[bufferLength];
		this->mlen = bufferLength;
		memcpy(mpbuffer, buf, bufferLength);
	}

	public: ReceiveData(uint8_t * buf, int bufferLength, std::shared_ptr<Candidate> remoteCandidate)
	{
		this->mpbuffer = new uint8_t[bufferLength];
		this->mlen = bufferLength;
		memcpy(mpbuffer, buf, bufferLength);
		mremoteCandidate = remoteCandidate;
	}

	public: virtual ~ReceiveData()
	{
		assert(NULL != mpbuffer);
		delete []mpbuffer;
		mpbuffer = NULL;
		mlen = 0;
	}
};

class IReveiverSinkU
{
	public: virtual ~IReveiverSinkU() { }
	public: virtual void OnData(std::shared_ptr<ReceiveData> data) = 0;
};

class TransceiverU : public cxm::util::IRunnable
{
	public: const static int MAX_RECEIVE_BUFFER_SIZE = 1024 * 10;

	private: std::shared_ptr<cxm::util::Thread> mthread;
	private: bool misRun;
	private: IReveiverSinkU *mpsink;
	private: uint8_t mreceBuffer[MAX_RECEIVE_BUFFER_SIZE];

	private: std::shared_ptr<Candidate> mlocalCandidate;
#if 0
	private: std::shared_ptr<Candidate> mremoteCandidate;
#endif
	private: Socket msocket;

	public: Socket GetSocket() { return msocket; }
	public: std::shared_ptr<Candidate> GetLocalCandidate() { return mlocalCandidate; }
#if 0
	public: std::shared_ptr<Candidate> GetRemoteCandidate() { return mremoteCandidate; }
#endif

	public: void SetLocalCandidate(std::shared_ptr<Candidate> candidate)
		{ mlocalCandidate = candidate; }
#if 0
	public: void SetRemoteCandidate(std::shared_ptr<Candidate> candidate)
		{ mremoteCandidate = candidate; }
#endif

	public: TransceiverU() : misRun(false), mpsink(NULL),
		msocket(INVALID_SOCKET) {}

	public: int Open();
	public: void Close();

	public: void SetSink(IReveiverSinkU *psink) { mpsink = psink; }
	public: int SendTo(std::shared_ptr<Candidate> remote, const uint8_t *buf, int len);

	private: virtual void Run();
#if 0
	public: static std::shared_ptr<TransceiverU> CreateTransceiverU(
		std::shared_ptr<Candidate> remoteCandidate,
		std::shared_ptr<Candidate> localCandidate = std::shared_ptr<Candidate>());
#endif
};

}
}
#endif
