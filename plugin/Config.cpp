#include "Config.hpp"
#include "Logger.hpp"

#include <windows.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

Config& Config::Instance() {
    static Config instance;
    return instance;
}

static std::string GetExecutableDir() {
    char path[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
        return ".";

    std::string s(path, len);
    size_t slash = s.find_last_of("\\/");
    if (slash == std::string::npos)
        return ".";
    return s.substr(0, slash);
}

static std::string Trim(const std::string& s) {
    const char* ws = " \t\r\n";
    size_t a = s.find_first_not_of(ws);
    size_t b = s.find_last_not_of(ws);
    if (a == std::string::npos || b == std::string::npos)
        return "";
    return s.substr(a, b - a + 1);
}

static std::string DefaultConfigText() {
    return
        "; MSFSDock runtime configuration.\r\n"
        "; File is read from the plugin root directory (next to msfsDock.exe) on startup.\r\n"
        "\r\n"
        "[ui]\r\n"
        "; Minimum interval between two consecutive per-key image redraws, in milliseconds.\r\n"
        "; Range ~100-200 ms corresponds to ~10-5 Hz. Lower = smoother but more CPU/bandwidth.\r\n"
        "; Default value: 200\r\n"
        "min_update_interval_ms = 200\r\n";
}

void Config::LoadOrCreate(const std::string& exeDir) {
    const std::string iniPath = exeDir + "\\config.ini";

    std::ifstream in(iniPath);
    if (!in.is_open()) {
        // Auto-create with defaults
        std::ofstream out(iniPath, std::ios::binary | std::ios::trunc);
        if (out.is_open()) {
            out << DefaultConfigText();
            out.close();
            LogMessage("Config: created default config.ini at " + iniPath);
        } else {
            LogError("Config: failed to create config.ini at " + iniPath);
        }
        // Defaults already set as member initializers.
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        // Strip comments
        size_t comment = line.find_first_of(";#");
        if (comment != std::string::npos)
            line = line.substr(0, comment);

        std::string t = Trim(line);
        if (t.empty())
            continue;

        // Skip section headers
        if (t.front() == '[')
            continue;

        size_t eq = t.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = Trim(t.substr(0, eq));
        std::string val = Trim(t.substr(eq + 1));
        if (key.empty() || val.empty())
            continue;

        if (key == "min_update_interval_ms") {
            try {
                int v = std::stoi(val);
                if (v > 0)
                    minUpdateIntervalMs_ = static_cast<uint32_t>(v);
            } catch (...) {
                // ignore malformed entries, keep default
            }
        }
    }

    LogMessage("Config: min_update_interval_ms=" + std::to_string(minUpdateIntervalMs_));
}

void Config::Init() {
    std::string exeDir = GetExecutableDir();
    LoadOrCreate(exeDir);
}