#ifndef _CXM_P2P_STUN_RESOLVER_H_
#define _CXM_P2P_STUN_RESOLVER_H_

#include <list>
#include <string>

namespace cxm {
namespace p2p {

typedef enum {
	STUN_NAT_TYPE_UNKNOWN,
	STUN_NAT_TYPE_CONE,
	STUN_NAT_TYPE_RESTRICTED,
	STUN_NAT_TYPE_PORT_RESTRICTED,
	STUN_NAT_TYPE_SYM
} STUN_NAT_TYPE_T;

class StunResolver
{
	private: std::list<std::string> mstunServerList;

	private: StunResolver();

	public: STUN_NAT_TYPE_T Resolve();
	private: STUN_NAT_TYPE_T ResolveInternal(const std::string &server);

	public: static StunResolver *GetInstance();
};

}
}
#endif
