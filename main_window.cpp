#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "ConfigManager.h"
#include "WindowsIntegration.h"
#include "AIAssistantWindow.h"
#include "ClipboardManager.h"
#include "ScratchpadUI.h"
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <tchar.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <vector>
#include <string>

ID3D11Device*           g_pd3dDevice = nullptr;
ID3D11DeviceContext*    g_pd3dDeviceContext = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

static bool m_showClipboard = false;

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

HRESULT CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
#if defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0,
    };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == E_FAIL) return res;

    CreateRenderTarget();
    return S_OK;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void ResetDevice() {
    CleanupRenderTarget();
    g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    CreateRenderTarget();
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    // Ctrl+Shift+V для Clipboard Manager
    if (msg == WM_HOTKEY) {
        if (wParam == 1) { // Ctrl+Shift+V
            // Показать превью буфера обмена
            m_showClipboard = !m_showClipboard;
            return 0;
        }
    }

    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            ResetDevice();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void ApplyConfigTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = g_Config.m_window.rounding;
    style.FrameRounding = g_Config.m_window.rounding * 0.5f;
    style.GrabRounding = g_Config.m_window.rounding * 0.3f;
    style.ScrollbarRounding = g_Config.m_window.rounding;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int) {
    // Windows интеграция
    WindowsIntegration::EnableDPIAwareness();
    
    // Обработка аргументов (открытие файла из контекстного меню)
    std::vector<std::wstring> filesToOpen;
    int nArgs = 0;
    if (lpCmdLine && lpCmdLine[0] != '\0') {
        LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &nArgs);
        if (args) {
            for (int i = 1; i < nArgs; ++i) {
                std::wstring path(args[i]);
                if (PathFileExistsW(path.c_str())) {
                    filesToOpen.push_back(path);
                }
            }
            LocalFree(args);
        }
    }
    
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, _T("UltraIDE"), nullptr };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, _T("UltraIDE"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720, nullptr, nullptr, wc.hInstance, nullptr);

    // Snap Layouts поддержка
    WindowsIntegration::EnableSnapLayouts(hwnd);
    
    if (FAILED(CreateDeviceD3D(hwnd))) {
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Register Ctrl+Shift+V hotkey
    RegisterHotKey(hwnd, 1, MOD_CONTROL | MOD_SHIFT, 'V');
    
    // Start clipboard monitoring
    ClipboardManager::Instance().StartMonitoring();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ApplyConfigTheme();
    g_Config.Load("config.json");
    g_Config.ApplyToUI();
    
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    
    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;
        
        if (g_Config.ReloadIfChanged()) {
            ApplyConfigTheme();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking);

        if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("Explorer")) {
                ImGui::Text("File Explorer");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Editor")) {
                ImGui::Text("Code Editor");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Terminal")) {
                ImGui::Text("Integrated Terminal");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        // AI Assistant Panel
        static AIAssistantWindow aiWindow;
        static bool showAI = true;
        aiWindow.m_insertCodeAtCursor = [](const std::string& code) {
            // TODO: Insert code into active editor
        };
        aiWindow.Render(&showAI);

        // Clipboard History
        if (m_showClipboard) {
            ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
            ImGui::Begin("Clipboard History", &m_showClipboard, 
                ImGuiWindowFlags_NoCollapse);
            
            auto& history = ClipboardManager::Instance().GetHistory();
            
            for (size_t i = 0; i < history.size(); ++i) {
                const auto& entry = history[i];
                
                if (ImGui::TreeNode(("entry_" + std::to_string(i)).c_str(), 
                    "%s • %s • %zu bytes", entry.language.c_str(), 
                    entry.timestamp.c_str(), entry.size)) {
                    
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                    ImGui::TextWrapped("%s", entry.content.c_str());
                    ImGui::PopStyleColor();
                    
                    if (ImGui::Button(("Paste##" + std::to_string(i)).c_str())) {
                        ClipboardManager::Instance().InsertEntry(i);
                    }
                    
                    ImGui::TreePop();
                }
            }
            ImGui::End();
        }

        // Scratchpad tab
        static ScratchpadUI scratchpad;
        static bool showScratch = false;
        if (ImGui::BeginTabBar("MainTabs")) {
            if (ImGui::BeginTabItem("Scratchpad")) {
                scratchpad.Render(&showScratch);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
        ImGui::Render();
        const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    
    // Stop clipboard monitoring
    ClipboardManager::Instance().StopMonitoring();
    
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    return 0;
}