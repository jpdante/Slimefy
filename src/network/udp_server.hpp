#pragma once

#include <esp_err.h>
#include <sys/param.h>
#include <sys/socket.h>

#include "net_buffer.hpp"

class UdpServer
{
private:
    bool running;
    bool connected;
    sockaddr_in clientAddress;
    sockaddr_in serverAddress;
    int sock;

public:
    UdpServer();
    ~UdpServer();

    esp_err_t start(int port);
    esp_err_t stop();
    ssize_t receive(char *buffer, size_t bufferLength, sockaddr_in *sourceAddress, socklen_t *sourceAddressLength);

    esp_err_t connect(const char *host, int port);
    esp_err_t disconnect();
    esp_err_t send(unsigned char *message, size_t size);
    esp_err_t send(NetBuffer &buffer);

    bool isRunning();
    bool isConnected();
};