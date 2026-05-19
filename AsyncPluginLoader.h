#pragma once
#include "PluginAPI.h"
#include "LuaPlugin.h"
#include <chrono>
#include <thread>
#include <atomic>

// Precompiled plugin cache
struct PluginCache {
    std::string sourceHash;      // Hash of .wasm or .lua
    std::vector<uint8_t> binary; // Precompiled bytecode
    std::chrono::time_point<std::chrono::steady_clock> lastLoad;
};

// Async plugin loader - non-blocking
class AsyncPluginLoader {
public:
    struct LoadResult {
        bool success;
        std::string pluginName;
        std::string error;
    };
    
    // Non-blocking load with timeout
    std::future<LoadResult> LoadPluginAsync(const std::string& path, 
                                          const std::string& type) {
        return std::async(std::launch::async, [path, type]() {
            LoadResult result;
            auto start = std::chrono::steady_clock::now();
            
            try {
                if (type == "wasm") {
                    // Use WebAssembly runtime
                    if (WASMRuntime().LoadPlugin(path)) {
                        result.success = true;
                        result.pluginName = path;
                    }
                } else {
                    // Use Lua
                    if (LuaPluginAPI().LoadPlugin(path)) {
                        result.success = true;
                        result.pluginName = path;
                    }
                }
            } catch (...) {
                result.error = "Load failed";
            }
            
            // Timeout check - shouldn't take more than 100ms
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed > std::chrono::milliseconds(100)) {
                result.error = "Timeout";
            }
            
            return result;
        });
    }
    
    // Memory-mapped loading for large plugins
    void* MapPluginMemory(const std::string& path) {
        // Use CreateFileMapping for zero-copy access
        HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, 
                                FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) return nullptr;
        
        HANDLE hMapping = CreateFileMapping(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
        CloseHandle(hFile);
        
        if (!hMapping) return nullptr;
        
        void* mapped = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
        CloseHandle(hMapping);
        
        return mapped;
    }
};

// Plugin performance monitor
class PluginProfiler {
public:
    static void BeginCall(const std::string& plugin) {
        m_startTimes[plugin] = std::chrono::steady_clock::now();
    }
    
    static double EndCall(const std::string& plugin) {
        auto it = m_startTimes.find(plugin);
        if (it == m_startTimes.end()) return 0.0;
        
        auto elapsed = std::chrono::steady_clock::now() - it->second;
        return std::chrono::duration<double, std::milli>(elapsed).count();
    }
    
    // Auto-disable slow plugins (>50ms per call)
    static bool ShouldDisable(const std::string& plugin, double timeMs) {
        return timeMs > 50.0;
    }
    
private:
    static std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_startTimes;
};