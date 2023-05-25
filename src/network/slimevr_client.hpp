#pragma once

#include <esp_err.h>
#include "udp_server.hpp"

class SlimeVRClient
{
public:
    UdpServer udpServer;

private:
    bool connected;
    bool running;
    uint64_t packetNumber;
    TaskHandle_t taskHandle;
    uint64_t lastPacketTime;
    uint64_t timeout;
    NetBuffer sendBuffer;

public:
    SlimeVRClient();
    ~SlimeVRClient();

    esp_err_t start();
    esp_err_t stop();

    void writePacketHeader(uint8_t packetType);
    bool isConnected();
    bool isRunning();

    esp_err_t sendHeartbeat();
    esp_err_t sendHandshake();
    esp_err_t sendSensorInfo(uint8_t id);
    esp_err_t sendAcceleration(uint8_t id);
    inline void processSensorInfo(unsigned char buffer[], size_t size);

    void internalPacketReceived(unsigned char buffer[], size_t size, struct sockaddr_in client_addr, socklen_t client_addr_len);

private:
    esp_err_t connect(const char *host, int port);
    esp_err_t disconnect();

    static void listen(void *arg);
};