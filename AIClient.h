#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <functional>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

#pragma comment(lib, "winhttp.lib")

class OpenRouterClient {
public:
    struct Message {
        std::string role; // "user", "assistant", "system"
        std::string content;
    };
    
    struct StreamCallback {
        std::function<void(const std::string& delta)> onChunk;
        std::function<void(const std::string& error)> onError;
        std::function<void()> onComplete;
    };
    
    OpenRouterClient(const std::string& apiKey) : m_apiKey(apiKey), m_running(false) {}
    
    // Асинхронный запрос с потоковой передачей
    void StreamChat(const std::vector<Message>& messages, const StreamCallback& callback, 
                   const std::string& model = "deepseek/deepseek-chat:free") {
        m_running = true;
        m_thread = std::thread([this, messages, callback, model]() {
            DoStreamRequest(messages, callback, model);
        });
    }
    
    void Cancel() {
        m_running = false;
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }
    
    ~OpenRouterClient() {
        Cancel();
    }
    
private:
    std::string m_apiKey;
    std::thread m_thread;
    std::atomic<bool> m_running;
    LONGLONG m_lastRequestTime = 0;
    std::mutex m_mutex;
    
    void DoStreamRequest(const std::vector<Message>& messages, const StreamCallback& callback, 
                        const std::string& model) {
        // Rate limiting: минимум 1 запрос в 500ms
        auto now = GetTickCount64();
        if (now - m_lastRequestTime < 500) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        m_lastRequestTime = now;
        
        HINTERNET hSession = WinHttpOpen(L"UltraIDE/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 
                                         WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) {
            callback.onError("Failed to create HTTP session");
            return;
        }
        
        HINTERNET hConnect = WinHttpConnect(hSession, L"https://openrouter.ai", 
                                            INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            callback.onError("Failed to connect to OpenRouter");
            return;
        }
        
        // JSON payload
        std::string body = R"({"model":"")" + model + R"(", "messages":[)";
        for (size_t i = 0; i < messages.size(); ++i) {
            if (i > 0) body += ",";
            body += "{\"role\":\"" + messages[i].role + "\",\"content\":\"" + 
                   EscapeJson(messages[i].content) + "\"}";
        }
        body += "], \"stream\":true}";
        
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", 
            L"/api/v1/chat/completions", nullptr, WINHTTP_NO_REFERER, 
            WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            callback.onError("Failed to create HTTP request");
            return;
        }
        
        // Заголовки
        std::wstring headers = L"Authorization: Bearer " + std::wstring(m_apiKey.begin(), m_apiKey.end()) +
                               L"\r\nContent-Type: application/json\r\nHTTP/1.1\r\n";
        
        if (!WinHttpSendRequest(hRequest, headers.c_str(), headers.size(), 
                               (LPVOID)body.c_str(), body.size(), body.size(), 0)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            callback.onError("Failed to send request");
            return;
        }
        
        if (!WinHttpReceiveResponse(hRequest, nullptr)) {
            Cleanup(hRequest, hConnect, hSession);
            callback.onError("Failed to receive response");
            return;
        }
        
        // Читаем потоковые данные
        DWORD dwSize = 0;
        std::string buffer;
        while (m_running && WinHttpQueryDataAvailable(hRequest, &dwSize)) {
            if (dwSize == 0) break;
            
            buffer.resize(dwSize);
            DWORD dwDownloaded = 0;
            if (WinHttpReadData(hRequest, (LPVOID)buffer.data(), dwSize, &dwDownloaded)) {
                ProcessSSEData(std::string(buffer.data(), dwDownloaded), callback);
            }
        }
        
        Cleanup(hRequest, hConnect, hSession);
        if (m_running) callback.onComplete();
    }
    
    void ProcessSSEData(const std::string& data, const StreamCallback& callback) {
        // SSE формат: "data: {...}\n\n"
        size_t pos = 0;
        while ((pos = data.find("data: ", pos)) != std::string::npos) {
            size_t end = data.find("\n\n", pos);
            if (end == std::string::npos) break;
            
            std::string jsonStr = data.substr(pos + 6, end - pos - 6);
            if (jsonStr != "[DONE]") {
                ParseSSEChunk(jsonStr, callback);
            }
            pos = end + 2;
        }
    }
    
    void ParseSSEChunk(const std::string& json, const StreamCallback& callback) {
        // Простой парсер JSON для извлечения delta
        // В продакшене используйте nlohmann/json
        size_t deltaPos = json.find("\"delta\":{");
        if (deltaPos != std::string::npos) {
            size_t contentPos = json.find("\"content\":\"", deltaPos);
            if (contentPos != std::string::npos) {
                size_t start = contentPos + 11;
                size_t end = json.find("\"", start);
                std::string content = json.substr(start, end - start);
                // Unescape JSON
                content = UnescapeJson(content);
                callback.onChunk(content);
            }
        }
    }
    
    std::string EscapeJson(const std::string& str) {
        std::string result;
        for (char c : str) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c;
            }
        }
        return result;
    }
    
    std::string UnescapeJson(const std::string& str) {
        std::string result;
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '\\' && i + 1 < str.size()) {
                switch (str[i + 1]) {
                    case '"': result += '"'; ++i; break;
                    case 'n': result += '\n'; ++i; break;
                    case 'r': result += '\r'; ++i; break;
                    case 't': result += '\t'; ++i; break;
                    case '\\': result += '\\'; ++i; break;
                    default: result += str[i]; break;
                }
            } else {
                result += str[i];
            }
        }
        return result;
    }
    
    void Cleanup(HINTERNET r, HINTERNET c, HINTERNET s) {
        if (r) WinHttpCloseHandle(r);
        if (c) WinHttpCloseHandle(c);
        if (s) WinHttpCloseHandle(s);
    }
};

// Глобальный клиент
static OpenRouterClient* g_AIClient = nullptr;

void InitAIClient() {
    const char* apiKey = "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";
    g_AIClient = new OpenRouterClient(apiKey);
}

void StreamAIResponse(const std::vector<OpenRouterClient::Message>& messages, 
                     const OpenRouterClient::StreamCallback& callback) {
    if (g_AIClient) {
        g_AIClient->StreamChat(messages, callback);
    }
}