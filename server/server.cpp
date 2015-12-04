#include <iostream>

#include "servant.h"

using namespace std;

int main()
{
	initNetwork();

	ServantServer ss;
	ss.Start();

	getchar();
	return 0;
}
