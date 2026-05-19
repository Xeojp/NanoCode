#pragma once
#include <windows.h>
#include <wincred.h>
#include <wincrypt.h>
#include <string>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Wincred.lib")

class SecureKeyStorage {
public:
    // Получить ключ (сначала из Credential Manager, потом fallback на мастер-ключ)
    static std::string GetAPIKey();
    
    // Сохранить пользовательский ключ
    static bool SetAPIKey(const std::string& key);
    
    // Удалить ключ (используется мастер-ключ)
    static bool ClearAPIKey();
    
    // Проверка наличия пользовательского ключа
    static bool HasCustomKey();
    
private:
    static std::string GetMasterKey();
    static std::string GetStoredKey();
    static bool SaveToCredentialManager(const std::string& key);
    static bool DeleteFromCredentialManager();
};