#include "slimevr_client.hpp"

#include <esp_log.h>
#include <esp_timer.h>
#include <esp_system.h>
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

SlimeVRClient::SlimeVRClient() : sendBuffer(128)
{
    udpServer = UdpServer();
    packetNumber = 0;
    connected = false;
    lastPacketTime = 0;
    timeout = 3000;
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
    if (this->udpServer.isConnected())
    {
        return ESP_OK;
    }
    ESP_LOGI(TAG, "Connecting to %s:%d", host, port);
    return this->udpServer.connect(host, port);
}

esp_err_t SlimeVRClient::disconnect()
{
    if (!this->udpServer.isConnected())
    {
        return ESP_OK;
    }
    ESP_LOGI(TAG, "Disconnecting");
    return this->udpServer.disconnect();
}

bool SlimeVRClient::isConnected()
{
    return this->connected;
}

bool SlimeVRClient::isRunning()
{
    return this->running;
}

void SlimeVRClient::writePacketHeader(uint8_t packetType)
{
    this->sendBuffer.writeByte(0);
    this->sendBuffer.writeByte(0);
    this->sendBuffer.writeByte(0);
    this->sendBuffer.writeByte(packetType);
    this->sendBuffer.writeULong(packetNumber++);
}

esp_err_t SlimeVRClient::sendHeartbeat()
{
    this->sendBuffer.reset();
    writePacketHeader(PACKET_HEARTBEAT);
    ESP_LOGI(TAG, "Sending heartbeat");
    return this->udpServer.send(this->sendBuffer);
}

esp_err_t SlimeVRClient::sendHandshake()
{
    this->sendBuffer.reset();
    this->sendBuffer.writeByte(0);
    this->sendBuffer.writeByte(0);
    this->sendBuffer.writeByte(0);
    this->sendBuffer.writeByte(PACKET_HANDSHAKE);
    this->sendBuffer.writeULong(0);

    this->sendBuffer.writeInt(5); // Board
    this->sendBuffer.writeInt(8); // IMU
    this->sendBuffer.writeInt(2); // CPU Count

    this->sendBuffer.writeInt(0);
    this->sendBuffer.writeInt(0);
    this->sendBuffer.writeInt(0);

    this->sendBuffer.writeInt(16);              // Build Version
    this->sendBuffer.writeShortString("0.3.3"); // Version String
    uint8_t mac[6] = {0xC4, 0xDE, 0xE2, 0x13, 0x95, 0xEC};
    this->sendBuffer.writeByteArray(mac, sizeof(mac)); // Mac Address

    ESP_LOGI(TAG, "Sending handshake");
    return this->udpServer.send(sendBuffer);
}

esp_err_t SlimeVRClient::sendSensorInfo(uint8_t id)
{
    this->sendBuffer.reset();
    this->writePacketHeader(PACKET_SENSOR_INFO);
    this->sendBuffer.writeByte(id);
    this->sendBuffer.writeByte(1);
    this->sendBuffer.writeByte(8);
    return this->udpServer.send(sendBuffer);
}

float generateRandomFloat()
{
    uint32_t randomValue = esp_random();
    float randomFloat = (float)randomValue / UINT32_MAX;
    return randomFloat;
}

esp_err_t SlimeVRClient::sendAcceleration(uint8_t id)
{
    this->sendBuffer.reset();
    this->writePacketHeader(PACKET_ACCEL);
    this->sendBuffer.writeFloat(generateRandomFloat());
    this->sendBuffer.writeFloat(generateRandomFloat());
    this->sendBuffer.writeFloat(generateRandomFloat());
    this->sendBuffer.writeByte(id);
    return this->udpServer.send(sendBuffer);
}

void SlimeVRClient::processSensorInfo(unsigned char buffer[], size_t size)
{
}

void SlimeVRClient::internalPacketReceived(unsigned char buffer[], size_t size, struct sockaddr_in client_addr, socklen_t client_addr_len)
{
    lastPacketTime = (uint64_t)(esp_timer_get_time() / 1000ULL);

    /*char hex_buffer[size * 3 + 1];
    memset(hex_buffer, 0, sizeof(hex_buffer));
    for (int i = 0; i < size; i++)
    {
        snprintf(hex_buffer + i * 3, 4, "%02X ", (unsigned char)buffer[i]);
    }
    ESP_LOGI(TAG, "%s:%d: %s", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), hex_buffer);*/

    if (this->connected)
    {
        switch (buffer[3])
        {
        case PACKET_HEARTBEAT:
            ESP_LOGI(TAG, "Heartbeat received");
            this->sendHeartbeat();
            break;
        case PACKET_RECEIVE_VIBRATE:
            ESP_LOGI(TAG, "Vibrate received");
            break;
        case PACKET_RECEIVE_HANDSHAKE:
            ESP_LOGI(TAG, "Handshake received");
            break;
        case PACKET_RECEIVE_COMMAND:
            ESP_LOGI(TAG, "Command received");
            break;
        case PACKET_CONFIG:
            ESP_LOGI(TAG, "Config received");
            break;
        case PACKET_PING_PONG:
            ESP_LOGI(TAG, "Ping received");
            this->udpServer.send((unsigned char *)buffer, size);
            break;
        case PACKET_SENSOR_INFO:
            ESP_LOGI(TAG, "Sensor Info received");
            if (size < 6)
            {
                ESP_LOGW(TAG, "Wrong sensor info packet");
                break;
            }
            this->processSensorInfo(buffer, size);
            break;
        }
        if (lastPacketTime + timeout < (uint64_t)(esp_timer_get_time() / 1000ULL))
        {
            this->connected = false;
            ESP_LOGW(TAG, "Connection to server timed out");
        }
    }
    else
    {
        switch (buffer[0])
        {
        case PACKET_HANDSHAKE:
            ESP_LOGI(TAG, "Handshake successful");
            this->connected = true;
            return;
        }
        this->connect(inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        this->sendHandshake();
    }
}

void SlimeVRClient::listen(void *arg)
{
    SlimeVRClient *client = (SlimeVRClient *)arg;
    const size_t size = 128;
    unsigned char recv_buffer[size];
    memset(recv_buffer, 0, sizeof(recv_buffer));
    for (;;)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        ssize_t len = client->udpServer.receive((char *)recv_buffer, size, &client_addr, &client_addr_len);
        if (len > 0)
        {
            client->internalPacketReceived(recv_buffer, len, client_addr, client_addr_len);
        }
    }
    vTaskDelete(NULL);
}