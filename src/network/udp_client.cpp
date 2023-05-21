#include "udp_client.hpp"

#include <string.h>
#include <esp_log.h>

static const char *TAG = "UdpClient";

UdpClient::UdpClient()
{
    this->connected = false;
}

UdpClient::~UdpClient()
{
    this->disconnect();
}

esp_err_t UdpClient::connect(const char *host, int port)
{
    if (this->connected)
    {
        return ESP_OK;
    }
    memset(&this->serverAddress, 0, sizeof(this->serverAddress));
    this->serverAddress.sin_family = AF_INET;
    this->serverAddress.sin_addr.s_addr = inet_addr(host);
    this->serverAddress.sin_port = htons(port);

    this->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (this->sock < 0)
    {
        ESP_LOGE(TAG, "Failed to create socket: %d", errno);
        return ESP_FAIL;
    }
    this->connected = true;
    return ESP_OK;
}

esp_err_t UdpClient::send(unsigned char *message, size_t size)
{
    char hex_buffer[size * 3 + 1];
    memset(hex_buffer, 0, sizeof(hex_buffer));
    for (int i = 0; i < size; i++)
    {
        snprintf(hex_buffer + i * 3, 4, "%02X ", (unsigned char)message[i]);
    }
    ESP_LOGI(TAG, "Sending message (hex): %s", hex_buffer);

    int sent = sendto(this->sock, message, size, 0, (struct sockaddr *)&this->serverAddress, sizeof(this->serverAddress));
    if (sent < 0)
    {
        ESP_LOGE(TAG, "Failed to send message: %d", errno);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t UdpClient::send(NetBuffer &buffer)
{
    return this->send(buffer.getBuffer(), buffer.getCurrentSize());
}

esp_err_t UdpClient::disconnect()
{
    if (!connected)
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

bool UdpClient::isConnected()
{
    return this->connected;
}