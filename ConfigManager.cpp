#include "ConfigManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>

using json = nlohmann::json;

ConfigManager g_Config;

bool ConfigManager::Load(const std::string& path) {
    try {
        std::ifstream f(path);
        if (!f.is_open()) return false;
        json j;
        f >> j;
        ParseJson(j);
        return true;
    } catch (...) {
        return false;
    }
}

bool ConfigManager::ReloadIfChanged() {
    auto lastWrite = std::filesystem::last_write_time(m_configPath);
    if (lastWrite != m_lastWriteTime) {
        m_lastWriteTime = lastWrite;
        return Load(m_configPath);
    }
    return false;
}

void ConfigManager::ApplyToUI() {
    // Цвета и стили применяются в main_window.cpp
    m_needsAnimationUpdate = true;
}

const std::string& ConfigManager::GetHotkey(const std::string& action) const {
    static std::string empty;
    auto it = m_actionToKey.find(action);
    return it != m_actionToKey.end() ? it->second : empty;
}

void ConfigManager::ParseJson(const json& j) {
    // Theme
    if (j.contains("theme")) {
        auto& t = j["theme"]["colors"];
        m_theme.editor_bg = ParseHexColor(t.value("editor_bg", "#1E1E1E"));
        m_theme.editor_fg = ParseHexColor(t.value("editor_fg", "#D4D4D4"));
        m_theme.sidebar_bg = ParseHexColor(t.value("sidebar_bg", "#252526"));
    }

    // Window
    if (j.contains("window")) {
        auto& w = j["window"];
        m_window.rounding = w.value("rounding", 4);
        m_window.opacity = w.value("opacity", 1.0f);
    }

    // Animations
    if (j.contains("animations")) {
        auto& a = j["animations"];
        m_animations.duration = a.value("duration", 150.0f);
        m_animations.fade_speed = a.value("fade_speed", 3.0f);
    }

    // Hotkeys
    if (j.contains("hotkeys")) {
        auto& h = j["hotkeys"];
        m_hotkeys.command_palette = h.value("command_palette", "Ctrl+P");
        m_hotkeys.toggle_sidebar = h.value("toggle_sidebar", "Ctrl+B");
        m_hotkeys.ai_assistant = h.value("ai_assistant", "Ctrl+Shift+A");

        m_actionToKey.clear();
        for (auto& [action, key] : h.items()) {
            m_actionToKey[action] = key.get<std::string>();
        }
    }
}

uint32_t ConfigManager::ParseHexColor(const std::string& hex) {
    uint32_t color = 0xFF000000;
    if (hex.size() >= 6) {
        color |= (uint32_t)strtol(hex.substr(1, 2).c_str(), nullptr, 16) << 16;
        color |= (uint32_t)strtol(hex.substr(3, 2).c_str(), nullptr, 16) << 8;
        color |= (uint32_t)strtol(hex.substr(5, 2).c_str(), nullptr, 16);
    }
    return color;
}

bool g_needsAnimationUpdate = false;

void UpdateConfig() {
    if (g_Config.ReloadIfChanged()) {
        g_Config.ApplyToUI();
        g_needsAnimationUpdate = true;
    }
}