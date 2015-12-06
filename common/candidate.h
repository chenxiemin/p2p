#ifndef _CXM_P2P_CANDIDATE_H_
#define _CXM_P2P_CANDIDATE_H_

#include <string>
#include <sstream>

#include "network.h"

class Candidate
{
	private: int mip;
	private: unsigned short mport;

	public: Candidate(int ip, unsigned short port) : mip(ip), mport(port) { }

	public:  public: Candidate(const char *ip, unsigned short port)
	{
		if (NULL != ip)
			mip = ntohl(inet_addr(ip));
		else
			mip = 0;
		mport = port;
	}

	public: std::string ToString()
	{
		unsigned char bytes[4];
		bytes[0] = mip & 0xFF;
		bytes[1] = (mip >> 8) & 0xFF;
		bytes[2] = (mip >> 16) & 0xFF;
		bytes[3] = (mip >> 24) & 0xFF;

		std::ostringstream ss;
		ss << std::to_string(bytes[3]) << "." << 
			std::to_string(bytes[2]) << "." << 
			std::to_string(bytes[1]) << "." << 
			std::to_string(bytes[0]) << ":" << mport;
		// ss << ":" << mport;
		return ss.str();

		// printf("%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]);
	}

	public: bool Equal(const Candidate &rhs)
		{ return mip == rhs.mip && mport == rhs.mport; }

	public: int Ip() { return mip; }
	public: uint16_t Port() { return mport; }

	public: void SetIP(int ip) { mip = ip; }
	public: void SetPort(uint16_t port) { mport = port; }
};

#endif
