#pragma once
#include "PluginHost.h"
#include "AsyncPluginLoader.h"
#include <imgui.h>

class PluginUI {
public:
    void Render(bool* open) {
        if (ImGui::Begin("Plugins", open, ImGuiWindowFlags_NoCollapse)) {
            
            // Plugin list
            ImGui::Text("Loaded plugins:");
            ImGui::BeginChild("PluginList", ImVec2(0, -60), true);
            
            for (const auto& [name, plugin] : PluginManager::Instance().GetPlugins()) {
                if (ImGui::TreeNode(name.c_str())) {
                    
                    // Commands
                    for (const auto& cmd : plugin->commands) {
                        if (ImGui::Button((cmd + "##" + name).c_str())) {
                            PluginManager::Instance().ExecuteCommand(name + "." + cmd);
                        }
                    }
                    
                    ImGui::TreePop();
                }
            }
            ImGui::EndChild();
            
            // Plugin controls
            if (ImGui::Button("Reload All")) {
                PluginManager::Instance().ReloadAll();
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Open Plugins Folder")) {
                ShellExecuteA(nullptr, "open", "plugins", nullptr, nullptr, SW_SHOWNORMAL);
            }
        }
        ImGui::End();
    }
};

// Plugin manifest format (plugin.json)
/*
{
    "name": "code-formatter",
    "version": "1.0",
    "entry": "main.wasm",
    "commands": [
        {
            "name": "format_document",
            "title": "Format Document",
            "shortcut": "Ctrl+Shift+F"
        }
    ]
}*/