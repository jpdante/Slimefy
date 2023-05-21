#pragma once

struct StorageManager
{
private:
    bool initialized = false;

public:
    StorageManager();

    void init();
};