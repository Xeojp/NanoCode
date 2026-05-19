#pragma once
#include "ClipboardManager.h"
#include <imgui.h>

class ClipboardManagerUI {
public:
    void Render(bool* open) {
        if (ImGui::Begin("Clipboard History", open, 
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
            
            auto& history = ClipboardManager::Instance().GetHistory();
            
            // Search
            ImGui::InputTextWithHint("##search", "Search...", &m_searchQuery);
            
            std::vector<size_t> visible;
            if (m_searchQuery.empty()) {
                for (size_t i = 0; i < history.size(); ++i) visible.push_back(i);
            } else {
                visible = ClipboardManager::Instance().Search(m_searchQuery);
            }
            
            // List entries
            for (size_t idx : visible) {
                const auto& entry = history[idx];
                
                ImGui::BeginChild(("entry_" + std::to_string(idx)).c_str(), 
                    ImVec2(0, 100), true);
                
                // Header with language and time
                ImGui::PushStyleColor(ImGuiCol_Border, GetLanguageColor(entry.language));
                ImGui::Text("%s • %s • %zu bytes [Ctrl+Shift+%d]", 
                    entry.language.c_str(), entry.timestamp.c_str(), entry.size, idx + 1);
                ImGui::PopStyleColor();
                
                // Preview (first 200 chars)
                std::string preview = entry.content.substr(0, 200);
                if (entry.content.size() > 200) preview += "...";
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                ImGui::TextWrapped("%s", preview.c_str());
                ImGui::PopStyleColor();
                
                // Insert button
                if (ImGui::Button(("Insert##" + std::to_string(idx)).c_str())) {
                    ClipboardManager::Instance().InsertEntry(idx);
                }
                
                ImGui::EndChild();
            }
            
            // Stats
            ImGui::Text("Total: %d entries", history.size());
        }
        ImGui::End();
    }
    
    void ShowPreviewPopup() {
        if (m_showPreview) {
            ImGui::OpenPopup("QuickPaste");
            m_showPreview = false;
        }
        
        if (ImGui::BeginPopupModal("QuickPaste", nullptr, 
            ImGuiWindowFlags_AlwaysAutoResize)) {
            
            auto& history = ClipboardManager::Instance().GetHistory();
            
            for (size_t i = 0; i < history.size() && i < 5; ++i) {
                const auto& entry = history[i];
                std::string label = std::to_string(i + 1) + ". " + entry.language;
                
                if (ImGui::Selectable(label.c_str())) {
                    ClipboardManager::Instance().InsertEntry(i);
                    ImGui::CloseCurrentPopup();
                }
            }
            
            ImGui::EndPopup();
        }
    }
    
    void OnHotKey(int index) {
        m_showPreview = true;
        // Index 1-9 maps to first 9 entries
        if (index >= 1 && index <= 9) {
            ClipboardManager::Instance().InsertEntry(index - 1);
        }
    }
    
private:
    ImVec4 GetLanguageColor(const std::string& lang) {
        if (lang == "cpp") return ImVec4(0.2f, 0.4f, 0.8f, 1.0f);
        if (lang == "python") return ImVec4(0.8f, 0.6f, 0.2f, 1.0f);
        if (lang == "javascript") return ImVec4(0.9f, 0.8f, 0.1f, 1.0f);
        if (lang == "rust") return ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
        return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
    
    std::string m_searchQuery;
    bool m_showPreview = false;
};