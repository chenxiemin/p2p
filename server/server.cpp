#include <iostream>

#include "servant.h"

using namespace std;
using namespace cxm::p2p;

int main()
{
	initNetwork();

	ServantServer ss;
	ss.Start();

	getchar();
	return 0;
}
