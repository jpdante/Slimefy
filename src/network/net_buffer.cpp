#include "net_buffer.hpp"

#include <cstring>

template <typename T>
unsigned char * NetBuffer::toChars(T src)
{
    union data
    {
        unsigned char buffer[sizeof(T)];
        T value;
    } un;
    un.value = src;
    for (size_t i = 0; i < sizeof(T); i++)
    {
        convBuf[i] = un.buffer[sizeof(T) - i - 1];
    }
    return convBuf;
}

template <typename T>
T NetBuffer::fromChars(unsigned char * const src)
{
    union data
    {
        unsigned char buffer[sizeof(T)];
        T value;
    } un;
    for (size_t i = 0; i < sizeof(T); i++)
    {
        un.buffer[i] = src[sizeof(T) - i - 1];
    }
    return un.value;
}

NetBuffer::NetBuffer(size_t size)
{
    buffer = new unsigned char[size];
    bufferSize = size;
    currentPosition = 0;
}

NetBuffer::~NetBuffer()
{
    delete[] buffer;
}

NetBuffer::NetBuffer(const NetBuffer &other)
    : bufferSize(other.bufferSize),
      currentPosition(other.currentPosition)
{
    buffer = new unsigned char[bufferSize];
    std::memcpy(buffer, other.buffer, bufferSize);
}

NetBuffer &NetBuffer::operator=(const NetBuffer &other)
{
    if (this != &other)
    {
        delete[] buffer;
        bufferSize = other.bufferSize;
        currentPosition = other.currentPosition;
        buffer = new unsigned char[bufferSize];
        std::memcpy(buffer, other.buffer, bufferSize);
    }
    return *this;
}

esp_err_t NetBuffer::seek(size_t position)
{
    if (position > bufferSize)
    {
        return ESP_ERR_INVALID_ARG;
    }
    currentPosition = position;
    return ESP_OK;
}

esp_err_t NetBuffer::writeByte(int8_t data)
{
    if (this->currentPosition + sizeof(int8_t) > this->bufferSize)
    {
        return ESP_FAIL;
    }
    std::memcpy(this->buffer + this->currentPosition, toChars(data), sizeof(int8_t));
    this->currentPosition += sizeof(int8_t);
    return ESP_OK;
}

esp_err_t NetBuffer::writeUByte(uint8_t data)
{
    if (this->currentPosition + sizeof(uint8_t) > this->bufferSize)
    {
        return ESP_FAIL;
    }
    std::memcpy(this->buffer + this->currentPosition, toChars(data), sizeof(uint8_t));
    this->currentPosition += sizeof(uint8_t);
    return ESP_OK;
}

esp_err_t NetBuffer::writeInt(int32_t data)
{
    if (this->currentPosition + sizeof(int32_t) > this->bufferSize)
    {
        return ESP_FAIL;
    }
    std::memcpy(this->buffer + this->currentPosition, toChars(data), sizeof(int32_t));
    this->currentPosition += sizeof(int32_t);
    return ESP_OK;
}

esp_err_t NetBuffer::writeUInt(uint32_t data)
{
    if (this->currentPosition + sizeof(uint32_t) > this->bufferSize)
    {
        return ESP_FAIL;
    }
    std::memcpy(this->buffer + this->currentPosition, toChars(data), sizeof(uint32_t));
    this->currentPosition += sizeof(uint32_t);
    return ESP_OK;
}

esp_err_t NetBuffer::writeLong(int64_t data)
{
    if (this->currentPosition + sizeof(int64_t) > this->bufferSize)
    {
        return ESP_FAIL;
    }

    std::memcpy(this->buffer + this->currentPosition, toChars(data), sizeof(int64_t));
    this->currentPosition += sizeof(int64_t);
    return ESP_OK;
}

esp_err_t NetBuffer::writeULong(uint64_t data)
{
    if (this->currentPosition + sizeof(uint64_t) > this->bufferSize)
    {
        return ESP_FAIL;
    }

    std::memcpy(this->buffer + this->currentPosition, toChars(data), sizeof(uint64_t));
    this->currentPosition += sizeof(uint64_t);
    return ESP_OK;
}

esp_err_t NetBuffer::writeFloat(float data)
{
    if (this->currentPosition + sizeof(float) > this->bufferSize)
    {
        return ESP_FAIL;
    }
    std::memcpy(this->buffer + this->currentPosition, toChars(data), sizeof(float));
    this->currentPosition += sizeof(float);
    return ESP_OK;
}

esp_err_t NetBuffer::writeBool(bool data)
{
    if (this->currentPosition + sizeof(bool) > this->bufferSize)
    {
        return ESP_FAIL;
    }
    std::memcpy(this->buffer + this->currentPosition, toChars(data), sizeof(bool));
    this->currentPosition += sizeof(bool);
    return ESP_OK;
}

esp_err_t NetBuffer::writeByteArray(uint8_t *data, size_t size)
{
    if (this->currentPosition + size > this->bufferSize)
    {
        return ESP_FAIL;
    }
    std::memcpy(this->buffer + this->currentPosition, data, size);
    this->currentPosition += size;
    return ESP_OK;
}

esp_err_t NetBuffer::writeString(char *data, size_t size)
{
    if (this->currentPosition + size > this->bufferSize)
    {
        return ESP_FAIL;
    }
    this->writeUInt(static_cast<uint32_t>(size));
    std::memcpy(this->buffer + this->currentPosition, data, size);
    this->currentPosition += size;
    return ESP_OK;
}

esp_err_t NetBuffer::writeShortString(char *data, size_t size)
{
    if (this->currentPosition + size > this->bufferSize)
    {
        return ESP_FAIL;
    }
    this->writeUByte(static_cast<uint8_t>(size));
    std::memcpy(this->buffer + this->currentPosition, data, size);
    this->currentPosition += size;
    return ESP_OK;
}

esp_err_t NetBuffer::writeString(const char *data)
{
    size_t size = strlen(data);
    if (this->currentPosition + size > this->bufferSize)
    {
        return ESP_FAIL;
    }
    this->writeUByte(static_cast<uint8_t>(size));
    std::memcpy(this->buffer + this->currentPosition, data, size);
    this->currentPosition += size;
    return ESP_OK;
}

esp_err_t NetBuffer::writeShortString(const char *data)
{
    size_t size = strlen(data);
    if (this->currentPosition + size > this->bufferSize)
    {
        return ESP_FAIL;
    }
    this->writeUByte(static_cast<uint8_t>(size));
    std::memcpy(this->buffer + this->currentPosition, data, size);
    this->currentPosition += size;
    return ESP_OK;
}

unsigned char *NetBuffer::getBuffer()
{
    return this->buffer;
}

size_t NetBuffer::getBufferSize()
{
    return this->bufferSize;
}

size_t NetBuffer::getCurrentSize()
{
    return this->currentPosition;
}