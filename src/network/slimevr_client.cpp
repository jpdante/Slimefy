#include "slimevr_client.hpp"

#include <esp_log.h>
#include "net_buffer.hpp"

#define PACKET_HEARTBEAT 0
#define PACKET_HANDSHAKE 3
#define PACKET_ACCEL 4
#define PACKET_RAW_CALIBRATION_DATA 6
#define PACKET_CALIBRATION_FINISHED 7
#define PACKET_CONFIG 8
#define PACKET_PING_PONG 10
#define PACKET_SERIAL 11
#define PACKET_BATTERY_LEVEL 12
#define PACKET_TAP 13
#define PACKET_ERROR 14
#define PACKET_SENSOR_INFO 15
#define PACKET_ROTATION_DATA 17
#define PACKET_MAGNETOMETER_ACCURACY 18
#define PACKET_SIGNAL_STRENGTH 19
#define PACKET_TEMPERATURE 20

#define PACKET_INSPECTION 105

#define PACKET_RECEIVE_HEARTBEAT 1
#define PACKET_RECEIVE_VIBRATE 2
#define PACKET_RECEIVE_HANDSHAKE 3
#define PACKET_RECEIVE_COMMAND 4

#define PACKET_INSPECTION_PACKETTYPE_RAW_IMU_DATA 1
#define PACKET_INSPECTION_PACKETTYPE_FUSED_IMU_DATA 2
#define PACKET_INSPECTION_PACKETTYPE_CORRECTION_DATA 3
#define PACKET_INSPECTION_DATATYPE_INT 1
#define PACKET_INSPECTION_DATATYPE_FLOAT 2

static const char *TAG = "SlimeVRClient";

SlimeVRClient::SlimeVRClient()
{
    udpClient = UdpClient();
    udpServer = UdpServer();
    packetNumber = 0;
    connected = false;
}

SlimeVRClient::~SlimeVRClient()
{
    this->disconnect();
}

esp_err_t SlimeVRClient::start()
{
    if (this->udpServer.isRunning())
        return ESP_OK;
    esp_err_t res = this->udpServer.start(6969);
    if (res == ESP_OK)
    {
        xTaskCreate(listen, "SocketReader", 4096, this, tskIDLE_PRIORITY, &taskHandle);
        this->running = true;
        ESP_LOGI(TAG, "SlimeServer is now running");
    }
    return res;
}

esp_err_t SlimeVRClient::stop()
{
    if (!this->udpServer.isRunning())
        return ESP_OK;
    vTaskDelete(taskHandle);
    esp_err_t res = this->udpServer.stop();
    if (res == ESP_OK)
    {
        this->running = false;
        ESP_LOGI(TAG, "SlimeServer stopped");
    }
    return res;
}

esp_err_t SlimeVRClient::connect(const char *host, int port)
{
    if (this->udpClient.isConnected())
    {
        return ESP_OK;
    }
    ESP_LOGI(TAG, "Connecting to %s:%d", host, port);
    return this->udpClient.connect(host, port);
}

esp_err_t SlimeVRClient::disconnect()
{
    if (!this->udpClient.isConnected())
    {
        return ESP_OK;
    }
    ESP_LOGI(TAG, "Disconnecting");
    return this->udpClient.disconnect();
}

bool SlimeVRClient::isConnected()
{
    return this->connected;
}

bool SlimeVRClient::isRunning()
{
    return this->running;
}

void SlimeVRClient::writePacketHeader(NetBuffer &buffer, uint8_t packetType)
{
    buffer.writeByte(0);
    buffer.writeByte(0);
    buffer.writeByte(0);
    buffer.writeByte(packetType);
    buffer.writeULong(packetNumber++);
}

esp_err_t SlimeVRClient::sendHeartbeat()
{
    NetBuffer buffer(12);
    writePacketHeader(buffer, PACKET_HEARTBEAT);
    ESP_LOGI(TAG, "Sending heartbeat");
    return this->udpClient.send(buffer);
}

esp_err_t SlimeVRClient::sendHandshake()
{
    NetBuffer buffer(256);
    buffer.writeByte(0);
    buffer.writeByte(0);
    buffer.writeByte(0);
    buffer.writeByte(PACKET_HANDSHAKE);
    buffer.writeULong(0);

    buffer.writeInt(5); // Board
    buffer.writeInt(8); // IMU
    buffer.writeInt(2); // CPU Count

    buffer.writeInt(0);
    buffer.writeInt(0);
    buffer.writeInt(0);

    buffer.writeInt(16); // Build Version
    buffer.writeShortString("0.3.3"); // Version String
    uint8_t mac[6] = {0xC4, 0xDE, 0xE2, 0x13, 0x95, 0xEC};
    buffer.writeByteArray(mac, sizeof(mac)); // Mac Address

    ESP_LOGI(TAG, "Sending handshake");
    return this->udpClient.send(buffer);
}

void SlimeVRClient::internalPacketReceived(char buffer[], size_t size, struct sockaddr_in client_addr, socklen_t client_addr_len)
{
    char hex_buffer[size * 3 + 1];
    memset(hex_buffer, 0, sizeof(hex_buffer));
    for (int i = 0; i < size; i++)
    {
        snprintf(hex_buffer + i * 3, 4, "%02X ", (unsigned char)buffer[i]);
    }
    ESP_LOGI(TAG, "%s:%d: %s", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), hex_buffer);

    switch (buffer[0])
    {
    case PACKET_HANDSHAKE:
        ESP_LOGI(TAG, "Handshake successful");
        this->connected = true;
        return;
    case PACKET_HEARTBEAT:
        //ESP_LOGI(TAG, "Heartbeat received from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        //this->sendHeartbeat();
        break;
    }
    if (!this->connected)
    {
        this->connect(inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        this->sendHandshake();
    }
}

void SlimeVRClient::listen(void *arg)
{
    SlimeVRClient *client = (SlimeVRClient *)arg;
    size_t size = 1500;
    char recv_buffer[size];
    memset(recv_buffer, 0, sizeof(recv_buffer));
    for (;;)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        ssize_t len = client->udpServer.receive(recv_buffer, size, &client_addr, &client_addr_len);
        if (len > 0)
        {
            client->internalPacketReceived(recv_buffer, len, client_addr, client_addr_len);
            /*ESP_LOGI(TAG, "Received data from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            char hex_buffer[len * 3 + 1];
            memset(hex_buffer, 0, sizeof(hex_buffer));
            for (int i = 0; i < len; i++)
            {
                snprintf(hex_buffer + i * 3, 4, "%02X ", (unsigned char)recv_buffer[i]);
            }
            ESP_LOGI(TAG, "Data (hex): %s", hex_buffer);*/
        }
    }
    vTaskDelete(NULL);
}