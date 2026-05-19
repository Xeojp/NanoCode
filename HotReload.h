#pragma once
#include <functional>
#include <thread>
#include <atomic>

class HotReloadSystem {
public:
    using Callback = std::function<void()>;
    
    struct WatchEntry {
        std::string path;
        std::filesystem::file_time_type::rep lastWrite = 0;
        Callback callback;
    };
    
    void AddWatch(const std::string& path, Callback cb) {
        m_entries.push_back({path, 0, cb});
    }
    
    void StartWatching() {
        m_running = true;
        m_thread = std::thread([this]() {
            while (m_running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                CheckChanges();
            }
        });
    }
    
    void Stop() {
        m_running = false;
        if (m_thread.joinable()) m_thread.join();
    }
    
private:
    void CheckChanges() {
        for (auto& entry : m_entries) {
            auto current = std::filesystem::last_write_time(entry.path);
            auto currentRep = current.time_since_epoch().count();
            if (currentRep != entry.lastWrite) {
                entry.lastWrite = currentRep;
                entry.callback();
            }
        }
    }
    
    std::vector<WatchEntry> m_entries;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
};