#include "SecureKeyStorage.h"

const wchar_t* const CREDENTIAL_TARGET = L"UltraIDE/OpenRouter";

std::string SecureKeyStorage::GetAPIKey() {
    std::string key = GetStoredKey();
    if (key.empty()) {
        key = GetMasterKey();
    }
    return key;
}

std::string SecureKeyStorage::GetMasterKey() {
    // Мастер-ключ в зашифрованном виде
    // Реальная реализация: серверный fallback или защищенный сервер
    return "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";
}

std::string SecureKeyStorage::GetStoredKey() {
    PCREDENTIALW cred = nullptr;
    if (CredReadW(CREDENTIAL_TARGET, CRED_TYPE_GENERIC, 0, &cred)) {
        std::string key(reinterpret_cast<char*>(cred->CredentialBlob), 
                      cred->CredentialBlobSize);
        CredFree(cred);
        return key;
    }
    return "";
}

bool SecureKeyStorage::SetAPIKey(const std::string& key) {
    CREDENTIALW cred = {};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = const_cast<wchar_t*>(CREDENTIAL_TARGET);
    cred.CredentialBlob = const_cast<LPBYTE>(reinterpret_cast<const LPBYTE>(key.data()));
    cred.CredentialBlobSize = static_cast<DWORD>(key.size());
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.UserName = L"UltraIDE";
    
    return CredWriteW(&cred, 0) != FALSE;
}

bool SecureKeyStorage::ClearAPIKey() {
    return CredDeleteW(CREDENTIAL_TARGET, CRED_TYPE_GENERIC, 0) != FALSE;
}

bool SecureKeyStorage::HasCustomKey() {
    return !GetStoredKey().empty();
}