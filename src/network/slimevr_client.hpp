#pragma once

#include <esp_err.h>
#include "udp_client.hpp"
#include "udp_server.hpp"

class SlimeVRClient
{
public:
    UdpServer udpServer;

private:
    bool connected;
    bool running;
    UdpClient udpClient;
    uint64_t packetNumber;
    TaskHandle_t taskHandle;

public:
    SlimeVRClient();
    ~SlimeVRClient();

    esp_err_t start();
    esp_err_t stop();

    void writePacketHeader(NetBuffer &buffer, uint8_t packetType);
    bool isConnected();
    bool isRunning();

    esp_err_t sendHeartbeat();
    esp_err_t sendHandshake();

    void internalPacketReceived(char buffer[], size_t size, struct sockaddr_in client_addr, socklen_t client_addr_len);

private:
    esp_err_t connect(const char *host, int port);
    esp_err_t disconnect();

    static void listen(void *arg);
};