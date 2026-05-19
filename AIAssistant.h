#pragma once
#include "AIClient.h"
#include "SecureKeyStorage.h"
#include <string>
#include <vector>
#include <memory>

const std::string SYSTEM_PROMPT = R"(
You are an AI coding assistant inside UltraIDE. Response rules:

1. Code-first, minimal explanation
2. Max 30 lines per solution  
3. Modern C++17/20, RAII, smart pointers
4. No introductions, no sign-offs
5. Match project code style

Example format:
```cpp
// Fix: use RAII
auto ptr = std::make_unique<Type>();
```

Current context: )" + std::string(__DATE__) + " " + __TIME__;

class AIAssistant {
public:
    AIAssistant() : m_client(SecureKeyStorage::GetAPIKey()) {
        m_messages.push_back({"system", SYSTEM_PROMPT});
    }
    
    void StreamChat(const std::vector<OpenRouterClient::Message>& messages, 
                   const OpenRouterClient::StreamCallback& callback,
                   const std::string& model = "deepseek/deepseek-chat:free") {
        m_client.StreamChat(messages, callback, model);
    }
    
    void Clear() {
        m_messages.clear();
        m_messages.push_back({"system", SYSTEM_PROMPT});
    }
    
    // Изменить API ключ
    static void SetCustomAPIKey(const std::string& key) {
        SecureKeyStorage::SetAPIKey(key);
        // Перезагружаем клиент с новым ключом
        g_instance = std::make_unique<AIAssistant>();
    }
    
    std::vector<std::pair<std::string, std::string>> m_messages;
    
private:
    OpenRouterClient m_client;
    static std::unique_ptr<AIAssistant> g_instance;
};