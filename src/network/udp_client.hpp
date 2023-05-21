#pragma once

#include <esp_err.h>
#include <sys/param.h>
#include <sys/socket.h>

#include "net_buffer.hpp"

class UdpClient
{
private:
    bool connected;
    sockaddr_in serverAddress;
    int sock;

public:
    UdpClient();
    ~UdpClient();

    esp_err_t connect(const char *host, int port);
    esp_err_t send(unsigned char *message, size_t size);
    esp_err_t send(NetBuffer &buffer);
    esp_err_t disconnect();
    bool isConnected();
};