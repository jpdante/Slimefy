#pragma once

#include <stdio.h>
#include <esp_err.h>

struct NetBuffer
{
private:
    unsigned char *buffer;
    size_t bufferSize;
    size_t currentPosition;

public:
    NetBuffer(size_t size);
    ~NetBuffer();
    NetBuffer(const NetBuffer &other);
    NetBuffer &operator=(const NetBuffer &other);

public:
    esp_err_t seek(size_t position);
    esp_err_t writeByte(int8_t data);
    esp_err_t writeUByte(uint8_t data);
    esp_err_t writeInt(int32_t data);
    esp_err_t writeUInt(uint32_t data);
    esp_err_t writeLong(int64_t data);
    esp_err_t writeULong(uint64_t data);
    esp_err_t writeFloat(float data);
    esp_err_t writeBool(bool data);
    esp_err_t writeByteArray(uint8_t *data, size_t size);
    esp_err_t writeString(char *data, size_t size);
    esp_err_t writeShortString(char *data, size_t size);
    esp_err_t writeString(const char *data);
    esp_err_t writeShortString(const char *data);
    unsigned char *getBuffer();
    size_t getBufferSize();
    size_t getCurrentSize();
};
