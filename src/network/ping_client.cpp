#include "ping_client.hpp"

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>

static const char *TAG = "PingClient";

PingClient::PingClient(int count, int interval, int taskPriority)
{
    config = ESP_PING_DEFAULT_CONFIG();
    config.count = count;
    config.interval_ms = interval;
    config.task_prio = taskPriority;
    callbacks = {
        .cb_args = this,
        .on_ping_success = PingClient::on_ping_success,
        .on_ping_timeout = PingClient::on_ping_timeout,
        .on_ping_end = PingClient::on_ping_end,
    };
    handle = esp_ping_handle_t();
    running = false;
}

esp_err_t dnsLookup(const char *target_host, ip_addr_t &target_addr)
{
    memset(&target_addr, 0, sizeof(target_addr));
    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    struct addrinfo *res = NULL;

    int err = getaddrinfo(target_host, NULL, &hint, &res);
    if (err != 0 || res == NULL)
    {
        ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGI(TAG, "DNS lookup success");
    }

    if (res->ai_family == AF_INET)
    {
        struct in_addr addr4 = ((struct sockaddr_in *)(res->ai_addr))->sin_addr;
        inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
    }
    else
    {
        struct in6_addr addr6 = ((struct sockaddr_in6 *)(res->ai_addr))->sin6_addr;
        inet6_addr_to_ip6addr(ip_2_ip6(&target_addr), &addr6);
    }
    freeaddrinfo(res);
    return ESP_OK;
}

esp_err_t PingClient::start(uint32_t interval, uint32_t taskPriority, const char *target_host)
{
    config.interval_ms = interval;
    config.task_prio = taskPriority;
    return start(target_host);
}

esp_err_t PingClient::start(const char *target_host)
{
    if (this->running)
        return ESP_FAIL;
    ip_addr_t target_addr;
    auto err = dnsLookup(target_host, target_addr);
    if (err != ESP_OK)
    {
        return err;
    }
    ESP_LOGI(TAG, "target_addr.type=%d", target_addr.type);
    ESP_LOGI(TAG, "target_addr.u_addr.ip4=%s", ip4addr_ntoa(&(target_addr.u_addr.ip4)));
    config.target_addr = target_addr;

    /* set callback functions */
    handle = new esp_ping_handle_t();
    err = esp_ping_new_session(&config, &callbacks, &handle);
    if (err != ESP_OK)
    {
        return err;
    }
    esp_ping_start(handle);
    if (err != ESP_OK)
    {
        return err;
    }
    ESP_LOGI(TAG, "ping start");
    this->running = true;
    return ESP_OK;
}

esp_err_t PingClient::stop()
{
    if (!running)
        return ESP_OK;
    auto err = esp_ping_stop(handle);
    return err;
}

bool PingClient::isRunning()
{
    return this->running;
}

void PingClient::internal_ping_success(esp_ping_handle_t hdl)
{
    if (this->handle != hdl)
        return;
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));

    ESP_LOGI(TAG, "%" PRIu32 " bytes from %s icmp_seq=%d ttl=%d time=%" PRIu32 " ms",
             recv_len, inet_ntoa(target_addr.u_addr.ip4), seqno, ttl, elapsed_time);
}

void PingClient::internal_ping_timeout(esp_ping_handle_t hdl)
{
    if (this->handle != hdl)
        return;
    uint16_t seqno;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    ESP_LOGW(TAG, "From %s icmp_seq=%d timeout", inet_ntoa(target_addr.u_addr.ip4), seqno);
}

void PingClient::internal_ping_end(esp_ping_handle_t hdl)
{
    if (this->handle != hdl)
        return;
    ip_addr_t target_addr;
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;
    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
    uint32_t loss = (uint32_t)((1 - ((float)received) / transmitted) * 100);
    if (IP_IS_V4(&target_addr))
    {
        ESP_LOGI(TAG, "\n--- %s ping statistics ---", inet_ntoa(*ip_2_ip4(&target_addr)));
    }
    else
    {
        ESP_LOGI(TAG, "\n--- %s ping statistics ---", inet6_ntoa(*ip_2_ip6(&target_addr)));
    }
    ESP_LOGI(TAG, "%" PRIu32 " packets transmitted, %" PRIu32 " received, %" PRIu32 "%% packet loss, time %" PRIu32 "ms",
             transmitted, received, loss, total_time_ms);
    // delete the ping sessions, so that we clean up all resources and can create a new ping session
    // we don't have to call delete function in the callback, instead we can call delete function from other tasks
    esp_ping_delete_session(this->handle);
    this->running = false;
}

void PingClient::on_ping_success(esp_ping_handle_t hdl, void *args)
{
    auto pingClient = (PingClient *)args;
    if (pingClient != nullptr)
    {
        pingClient->internal_ping_success(hdl);
    }
}

void PingClient::on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
    auto pingClient = (PingClient *)args;
    if (pingClient != nullptr)
    {
        pingClient->internal_ping_timeout(hdl);
    }
}

void PingClient::on_ping_end(esp_ping_handle_t hdl, void *args)
{
    auto pingClient = (PingClient *)args;
    if (pingClient != nullptr)
    {
        pingClient->internal_ping_end(hdl);
    }
}