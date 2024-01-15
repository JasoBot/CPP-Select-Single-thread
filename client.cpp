#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <windows.h>

int main()
{
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in connect_addr;
    connect_addr.sin_family = AF_INET;
    connect_addr.sin_port = htons(8888); // 设置服务器端口号
    /*
    “0.0.0.0” 通常用于服务器监听，而不是用于客户端连接。应该使用服务器的实际IP地址，
    或者如果客户端和服务器运行在同一台机器上，可以使用 127.0.0.1（本地回环地址）
    */
    inet_pton(AF_INET, "127.0.0.1", &connect_addr.sin_addr); // 设置服务器地址，
    //"192.168.31.217" 地址来自 ipconfig 查看，若为了具有普适性，则可以使用 "127.0.0.1"

    if (connect(fd, (sockaddr *)&connect_addr, sizeof(struct sockaddr)) < 0)
    {
        std::cerr << "Connection failed." << std::endl;
        return -1;
    }

    char sBuf[1024];
    memset(sBuf, 0, sizeof(sBuf));
    sprintf(sBuf, "Hello server,I am %d.", rand());
    send(fd, sBuf, strlen(sBuf) + 1, 0);

    char rBuf[1024];
    memset(rBuf, 0, sizeof(rBuf));
    recv(fd, rBuf, sizeof(rBuf), 0);
    printf("Server says : %s", rBuf);

    Sleep(10000);
    closesocket(fd);
    WSACleanup();
    return 0;
}
