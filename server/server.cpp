#include <iostream>
#include <unistd.h>
#include <string>

#include "servant.h"

using namespace std;
using namespace cxm::p2p;

void server_main(bool isBackground)
{
    initNetwork();

    ServantServer ss;
    ss.Start();

    if (isBackground)
        while (true)
            sleep(10);
    else
        getchar();
}

int main(int argc, char **argv)
{
    if (argc == 2 && string(argv[1]) == "-b") {
        int pid = fork();
        if (-1 == pid) {
            cout << "call function fork() error!" << endl;  
            return -1;
        }

        if (pid > 0) {
            cout << "the child process id: " << pid << endl;
            return 0;
        }

        cout << "----------in child process.----------" << endl;  
        //将该进程的进程组ID设置为该进程的进程ID。  
        setpgrp();  
        //创建一个新的Session，断开与控制终端的关联。也就是说Ctrl+c的触发的SIGINT信号，该进程接收不到。  
        setsid();  

        server_main(true);
    } else {
        server_main(false);
    }

    return 0;
}

