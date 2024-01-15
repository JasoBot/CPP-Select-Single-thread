#include <iostream>
#include <winsock2.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <algorithm>

class Clientfd // 存储客户端的信息
{

public:
    int clientSockfd;
    int num; // 这是第几个连接的客户端
    sockaddr_in client_addr;
    Clientfd(int clientfd, int num, sockaddr_in caddr) : clientSockfd(clientfd), num(num),
                                                         client_addr(caddr) {}
    ~Clientfd() = default;
};

class Server
{
public:
    int connectNum;
    WSADATA wsdata;
    int listenfd;
    std::vector<Clientfd> Client;
    sockaddr_in server_addr;
    fd_set fds;

public:
    Server()
    {
        WSAStartup(MAKEWORD(2, 2), &wsdata);
        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY; // inet_addr("0.0.0.0");
        server_addr.sin_port = htons(8888);
        FD_ZERO(&fds);
        FD_SET(listenfd, &fds);
        if (bind(listenfd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            std::cout << " bind error! " << std::endl;
            return;
        }
        if (listen(listenfd, 128) < 0)
        {
            std::cout << " listen error! " << std::endl;
            return;
        }
    }

    void doClient(int clientfd);
    void acceptClient(fd_set *set);
    void serverLoop();

    ~Server()
    {
        closesocket(listenfd);
        WSACleanup();
    }
};

void Server::doClient(int clientSockfd)
{
    // 获取客户端信息
    Clientfd *client = nullptr;
    for (auto &c : Client)
    {
        if (c.clientSockfd == clientSockfd)
        {
            client = &c;
            break;
        }
    }

    if (!client)
    {
        std::cerr << "Client not found." << std::endl;
        return;
    }

    const char *clientIP = inet_ntoa(client->client_addr.sin_addr);
    int clientPort = ntohs(client->client_addr.sin_port);
    printf("The client's IP is %s ,and its port is %d \n", clientIP, clientPort);
    char rBuf[1024];
    while (true)
    {
        memset(rBuf, 0, sizeof(rBuf));
        int recvlen = recv(clientSockfd, rBuf, sizeof(rBuf), 0);

        if (recvlen > 0)
        {
            // 处理接收到的数据
            char sBuf[1024];
            memset(sBuf, 0, sizeof(sBuf));
            std::string respond = "Thank u for ur message: ";
            strcpy(sBuf, respond.c_str());
            strncpy(sBuf + respond.length(), rBuf, recvlen);
            // 要发送的长度为 strlen(sBuf)+1 因为 strlen() 只返回字符数量，+1将最后的 '\0' 也一并发送
            send(clientSockfd, sBuf, strlen(sBuf) + 1, 0);
        }
        else if (recvlen == 0)
        {
            // 客户端关闭了连接
            std::cout << "Client closed the connection." << std::endl;
            FD_CLR(clientSockfd, &fds); // 移除套接字
            closesocket(clientSockfd);  // 关闭套接字
            // 移除客户端对象
            Client.erase(std::remove_if(Client.begin(), Client.end(),
                                        [clientSockfd](const Clientfd &c)
                                        { return c.clientSockfd == clientSockfd; }),
                         Client.end());
            break;
        }
        else
        {
            // 发生错误
            std::cerr << "Recv error: " << WSAGetLastError() << std::endl;
            break;
        }
    }
    closesocket(clientSockfd);
}

void Server::acceptClient(fd_set *set)
{
    int clientSockfd;
    struct sockaddr_in caddr;
    memset(&caddr, 0, sizeof(caddr));
    int caddr_len;
    caddr_len = sizeof(sockaddr_in);

    std::cout << "Prepare to accept" << std::endl;
    clientSockfd = accept(listenfd, (sockaddr *)&caddr, &caddr_len);
    std::cout << "Accepted" << std::endl;
    Client.emplace_back(clientSockfd, Client.size() + 1, caddr);
    FD_SET(clientSockfd, set);
}

void Server::serverLoop()
{
    fd_set tmp;// select 会修改传入的 fd_set,因此需要保存原始的待监听的数据，往select中传入一份拷贝即可
    while (1)
    {
        tmp = fds;
        int ret = select(-1, &tmp, NULL, NULL, NULL);
        if (ret > 0)
        {
            if (FD_ISSET(listenfd, &tmp))
            {
                acceptClient(&fds);
            }

            // for (int i = 0; i < FD_SETSIZE; i++)-----> only on Linux
            // {
            //     if (FD_ISSET(i, &tmp) && i != listenfd)
            //     {
            //         doClient(i);
            //     }
            // }

            for (auto i : Client)
            {
                if (FD_ISSET(i.clientSockfd, &tmp))
                {
                    doClient(i.clientSockfd);
                }
            }
        }
        else if (ret == SOCKET_ERROR)
        {
            std::cerr << "Select error: " << WSAGetLastError() << std::endl;
            break; // 或其他错误处理
        }
    }
}

int main()
{
    Server *mServer = new Server();
    mServer->serverLoop();
    return 0;
}