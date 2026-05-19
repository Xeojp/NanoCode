#pragma once
#include <windows.h>
#include <shellscalingapi.h>
#include <d3d11.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <string>

using Microsoft::WRL::ComPtr;

class WindowsIntegration {
public:
    // Snap Layouts (Windows 11)
    // Добавляем поддержку системного размещения окна
    static void EnableSnapLayouts(HWND hwnd) {
        // Windows 11+ поддержка системных кнопок размещения
        HWND taskbarWnd = FindWindow(L"Shell_TrayWnd", nullptr);
        if (taskbarWnd) {
            // Разрешаем системе управлять окном через Snap Layouts
            SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, 
                SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOACTIVATE);
        }
        
        // Активируем caption buttons в Windows 11 style
        HMODULE hUser = GetModuleHandle(L"user32.dll");
        if (hUser) {
            typedef HRESULT (WINAPI *SetWindowCompositionAttributeFunc)(
                HWND, WINCOMPATTR_DATA*);
            auto pSetWindowCompositionAttribute = 
                (SetWindowCompositionAttributeFunc)GetProcAddress(hUser, "SetWindowCompositionAttribute");
        }
    }
    
    // Аппаратное ускорение: создание D3D11 устройства с поддержкой HW acceleration
    static HRESULT CreateHardwareDevice(ID3D11Device** device, ID3D11DeviceContext** context) {
        DXGI_ADAPTER_DESC1 desc;
        ComPtr<IDXGIFactory1> factory;
        CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(factory.GetAddressOf()));
        
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        for (UINT i = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &hardwareAdapter); ++i) {
            desc = hardwareAdapter->GetDesc1();
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
            
            // Проверяем поддержку D3D11
            if (SUCCEEDED(D3D11CreateDevice(
                hardwareAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0,
                nullptr, 0, D3D11_SDK_VERSION, device, nullptr, context))) {
                return S_OK;
            }
        }
        return E_FAIL;
    }
    
    // Поддержка High DPI и Per-Monitor V2
    static void EnableDPIAwareness() {
        HMODULE hUser = LoadLibrary(L"user32.dll");
        if (hUser) {
            typedef BOOL (WINAPI *SetProcessDpiAwarenessContextFunc)(DPI_AWARENESS_CONTEXT);
            auto pSetProcessDpiAwarenessContext = 
                (SetProcessDpiAwarenessContextFunc)GetProcAddress(hUser, "SetProcessDpiAwarenessContext");
            if (pSetProcessDpiAwarenessContext) {
                pSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            }
        }
    }
    
    // Регистрация файловых ассоциаций
    static void RegisterFileAssociations() {
        HKEY hKey;
        // .cpp, .h, .py, .js, .ts, .rs, .go
        const wchar_t* extensions[] = {L".cpp", L".h", L".py", L".js", L".ts", L".rs", L".go"};
        
        for (const auto& ext : extensions) {
            std::wstring keyPath = std::wstring(ext) + L"\\OpenWithProgids";
            RegCreateKeyEx(HKEY_CLASSES_ROOT, keyPath.c_str(), 0, nullptr, 0, 
                          KEY_WRITE, nullptr, &hKey, nullptr);
            RegSetValueEx(hKey, L"UltraIDE", 0, REG_SZ, 
                         reinterpret_cast<const BYTE*>(L"UltraIDE"), 18);
            RegCloseKey(hKey);
        }
    }
    
    // Цветовая схема Windows (светлая/темная)
    static bool IsDarkMode() {
        HKEY hKey;
        DWORD value = 0;
        DWORD size = sizeof(DWORD);
        
        if (RegOpenKeyEx(HKEY_CURRENT_USER, 
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 
            0, KEY_READ, &hKey) == S_OK) {
            RegQueryValueEx(hKey, L"AppsUseLightTheme", nullptr, nullptr, 
                           reinterpret_cast<LPBYTE>(&value), &size);
            RegCloseKey(hKey);
            return value == 0; // 0 = dark mode
        }
        return false;
    }
};