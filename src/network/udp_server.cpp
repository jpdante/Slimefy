#include "udp_server.hpp"

#include <string.h>
#include <esp_log.h>

static const char *TAG = "UdpServer";

UdpServer::UdpServer()
{
    this->running = false;
}

UdpServer::~UdpServer()
{
    this->stop();
}

esp_err_t UdpServer::start(int port)
{
    memset(&this->serverAddress, 0, sizeof(this->serverAddress));
    this->serverAddress.sin_family = AF_INET;
    this->serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    this->serverAddress.sin_port = htons(port);

    this->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (this->sock < 0)
    {
        ESP_LOGE(TAG, "Failed to create socket: %d", errno);
        return ESP_FAIL;
    }

    this->running = true;

    int ret = bind(this->sock, (struct sockaddr *)&this->serverAddress, sizeof(this->serverAddress));
    if (ret != 0)
    {
        ESP_LOGE(TAG, "Failed to bind socket: %d", errno);
        this->stop();
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t UdpServer::stop()
{
    if (!running)
    {
        return ESP_OK;
    }
    int res = close(this->sock);
    if (res != 0)
    {
        ESP_LOGE(TAG, "Failed to close socket: %d", errno);
        return ESP_FAIL;
    }
    return ESP_OK;
}

ssize_t UdpServer::receive(char* buffer, size_t bufferLength, sockaddr_in* sourceAddress, socklen_t* sourceAddressLength) {
    if (!this->running) {
        return ESP_FAIL;
    }
    return recvfrom(this->sock, buffer, bufferLength, 0, (struct sockaddr *)sourceAddress, sourceAddressLength);
}

bool UdpServer::isRunning() {
    return this->running;
}