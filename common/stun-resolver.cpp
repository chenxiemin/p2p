#include "stun-resolver.h"

#include "udp.h"
#include "stun.h"
#include "log.h"

using namespace std;

namespace cxm {
namespace p2p {

StunResolver::StunResolver()
{
	this->mstunServerList.push_back("stun.voipbuster.com");

	initNetwork();
}

STUN_NAT_TYPE_T StunResolver::Resolve()
{
	STUN_NAT_TYPE_T type = STUN_NAT_TYPE_UNKNOWN;
	for (list<string>::iterator iter = mstunServerList.begin();
		iter != mstunServerList.end(); iter++) {
		type = ResolveInternal(*iter);
		if (STUN_NAT_TYPE_UNKNOWN != type)
			break;
	}

	return type;
}

STUN_NAT_TYPE_T StunResolver::ResolveInternal(const std::string &server)
{
	STUN_NAT_TYPE_T type = STUN_NAT_TYPE_UNKNOWN;
	do {
		StunAddress4 stunServerAddr;
		stunServerAddr.addr = 0;

		// resolve server address
		bool ret = stunParseServerName(const_cast<char *>(server.c_str()), stunServerAddr);
		if (!ret || 0 == stunServerAddr.addr) {
			LOGE("Cannot parser stun server address: %s", server.c_str());
			break;
		}

		int srcPort = stunRandomPort();

		bool presPort = false;
		bool hairpin = false;
		StunAddress4 sAddr;
		sAddr.addr = 0;
		sAddr.port = 0;

		NatType stype = stunNatType(stunServerAddr, false,
			&presPort, &hairpin, srcPort, &sAddr);

		switch (stype)
		{
		case StunTypeIndependentFilter:
			LOGD("Independent Mapping, Independent Filter: presPort %d hairpin %d", presPort, hairpin);
			type = STUN_NAT_TYPE_CONE;
			break;
		case StunTypeDependentFilter:
			LOGD("Independent Mapping, Address Dependent Filter: presPort %d hairpin %d", presPort, hairpin);
			type = STUN_NAT_TYPE_RESTRICTED;
			break;
		case StunTypePortDependedFilter:
			LOGD("Independent Mapping, Port Dependent Filter: presPort %d hairpin %d", presPort, hairpin);
			type = STUN_NAT_TYPE_PORT_RESTRICTED;
			break;
		case StunTypeDependentMapping:
			LOGD("Dependent Mapping: presPort %d hairpin %d", presPort, hairpin);
			type = STUN_NAT_TYPE_SYM;
			break;
		case StunTypeFailure:
			LOGE("Some stun error detetecting NAT type");
			break;
		case StunTypeUnknown:
			LOGE("Some unknown type error detetecting NAT type");
			break;
		case StunTypeOpen:
			LOGE("Open stun type");
			break;
		case StunTypeFirewall:
			cout << "Firewall";
			break;
		case StunTypeBlocked:
			cout << "Blocked or could not reach STUN server";
			break;
		default:
			LOGD("Unknown NAT stype: ", stype);
			break;
		}
	} while (false);

	return type;
}

StunResolver *StunResolver::GetInstance()
{
	static StunResolver resolver;
	return &resolver;
}

}
}
