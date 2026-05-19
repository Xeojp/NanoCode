#pragma once
#include <string>
#include <unordered_map>
#include <filesystem>

class ConfigManager {
public:
    struct Theme {
        uint32_t editor_bg = 0xFF1E1E1E;
        uint32_t editor_fg = 0xFFD4D4D4;
        uint32_t sidebar_bg = 0xFF252526;
    };
    
    struct WindowConfig {
        int rounding = 4;
        float opacity = 1.0f;
        float position_x = 100.0f;
        float position_y = 100.0f;
        float size_width = 1280.0f;
        float size_height = 720.0f;
    };
    
    struct AnimationConfig {
        float duration = 150.0f;
        float fade_speed = 3.0f;
        bool enabled = true;
    };
    
    struct Hotkeys {
        std::string command_palette = "Ctrl+P";
        std::string toggle_sidebar = "Ctrl+B";
        std::string ai_assistant = "Ctrl+Shift+A";
    };
    
    Theme m_theme;
    WindowConfig m_window;
    AnimationConfig m_animations;
    Hotkeys m_hotkeys;
    
    bool Load(const std::string& path);
    bool ReloadIfChanged();
    void ApplyToUI();
    const std::string& GetHotkey(const std::string& action) const;
    
private:
    std::string m_configPath = "config.json";
    std::filesystem::file_time_type::rep m_lastWriteTime = 0;
    std::unordered_map<std::string, std::string> m_actionToKey;
    uint32_t ParseHexColor(const std::string& hex);
    void ParseJson(const class json& j);
};

extern ConfigManager g_Config;