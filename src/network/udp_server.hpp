#pragma once

#include <esp_err.h>
#include <sys/param.h>
#include <sys/socket.h>

class UdpServer
{
private:
    bool running;
    sockaddr_in serverAddress;
    int sock;

public:
    UdpServer();
    ~UdpServer();

    esp_err_t start(int port);
    esp_err_t stop();
    ssize_t receive(char* buffer, size_t bufferLength, sockaddr_in* sourceAddress, socklen_t* sourceAddressLength);
    bool isRunning();
};