#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <regex>

struct ClipboardEntry {
    std::string content;
    std::string language;
    std::string timestamp;
    size_t size;
};

class ClipboardManager {
public:
    static ClipboardManager& Instance() {
        static ClipboardManager instance;
        return instance;
    }
    
    void StartMonitoring() {
        if (m_running) return;
        m_running = true;
        m_thread = std::thread(&ClipboardManager::MonitorLoop, this);
    }
    
    void StopMonitoring() {
        m_running = false;
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }
    
    // Получить историю для превью
    const std::deque<ClipboardEntry>& GetHistory() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_history;
    }
    
    // Вставка по индексу
    bool InsertEntry(size_t index) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (index >= m_history.size()) return false;
        
        const auto& entry = m_history[index];
        return SetClipboardText(entry.content);
    }
    
    // Очистка истории
    void Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_history.clear();
    }
    
    // Поиск по содержимому
    std::vector<size_t> Search(const std::string& query) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<size_t> results;
        size_t idx = 0;
        for (const auto& entry : m_history) {
            if (entry.content.find(query) != std::string::npos) {
                results.push_back(idx);
            }
            ++idx;
        }
        return results;
    }
    
private:
    ClipboardManager() = default;
    ~ClipboardManager() { StopMonitoring(); }
    
    void MonitorLoop() {
        std::string lastText;
        while (m_running) {
            std::string current = GetClipboardText();
            if (!current.empty() && current != lastText) {
                OnClipboardChange(current);
                lastText = current;
            }
            Sleep(200); // Poll every 200ms
        }
    }
    
    void OnClipboardChange(const std::string& text) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        ClipboardEntry entry;
        entry.content = text;
        entry.language = DetectLanguage(text);
        entry.timestamp = GetCurrentTime();
        entry.size = text.size();
        
        // Добавляем в историю
        m_history.push_front(entry);
        
        // Ограничиваем до 20 записей
        if (m_history.size() > 20) {
            m_history.pop_back();
        }
    }
    
    std::string DetectLanguage(const std::string& text) {
        // Простой детектор по синтаксису
        std::regex cpp(R"(#include|std::|cout|cin|>>|<<|\bclass\b|\bvoid\b)");
        std::regex python(R"(\bdef\b|\bimport\b|\bprint\(|if __name__|\belif\b|\bfor . in\b)");
        std::regex javascript(R"((\bvar\b|\blet\b|\bconst\b|\bfunction\b|\b=>\b|\bconsole\.))");
        std::regex rust(R"(\blet mut\b|\bmut\b|\bfn\b|\bimpl\b|\bstruct\b)");
        
        if (std::regex_search(text, cpp)) return "cpp";
        if (std::regex_search(text, python)) return "python";
        if (std::regex_search(text, javascript)) return "javascript";
        if (std::regex_search(text, rust)) return "rust";
        
        return "text";
    }
    
    std::string GetCurrentTime() {
        SYSTEMTIME st;
        GetLocalTime(&st);
        char buf[32];
        sprintf_s(buf, "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
        return buf;
    }
    
    std::string GetClipboardText() {
        if (!OpenClipboard(nullptr)) return "";
        
        std::string text;
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (hData) {
            text = reinterpret_cast<char*>(hData);
        }
        
        CloseClipboard();
        return text;
    }
    
    bool SetClipboardText(const std::string& text) {
        if (!OpenClipboard(nullptr)) return false;
        
        EmptyClipboard();
        
        size_t len = text.size() + 1;
        HANDLE hMem = GlobalAlloc(GMEM_MOVEABLE, len);
        if (!hMem) {
            CloseClipboard();
            return false;
        }
        
        memcpy(GlobalLock(hMem), text.c_str(), len);
        GlobalUnlock(hMem);
        
        SetClipboardData(CF_TEXT, hMem);
        CloseClipboard();
        return true;
    }
    
    static const size_t MAX_HISTORY = 20;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    std::deque<ClipboardEntry> m_history;
    mutable std::mutex m_mutex;
};