#pragma once
#include <string>
#include <cstdint>

// Loaded from config.ini located next to msfsDock.exe (plugin root directory).
// The file is auto-generated with default values on first launch if missing.
class Config {
public:
    static Config& Instance();

    // Resolves the executable directory, loads config.ini (creating it with defaults when absent).
    void Init();

    // Minimum delay between two consecutive per-action image redraws, in milliseconds.
    // Lower = smoother UI but more CPU work and SimConnect traffic.
    // Default 200 ms => ~5 Hz (fits the 5-10 Hz target).
    uint32_t MinUpdateIntervalMs() const { return minUpdateIntervalMs_; }

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    uint32_t minUpdateIntervalMs_ = 200;

    void LoadOrCreate(const std::string& exeDir);
};