#pragma once
#include "Scratchpad.h"
#include <imgui.h>
#include <deque>

class ScratchpadUI {
public:
    void Render(bool* open) {
        if (ImGui::Begin("Scratchpad", open, 
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
            
            // Language selector
            const char* langs[] = {"Python", "JavaScript", "C#"};
            int currentLang = static_cast<int>(m_pad.GetConfig().language);
            if (ImGui::Combo("Language", &currentLang, langs, 3)) {
                m_pad.SetLanguage(static_cast<ScratchLanguage>(currentLang));
            }
            
            // Code editor
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            ImGui::BeginChild("CodeEditor", ImVec2(0, -80), true);
            ImGui::InputTextMultiline("##code", &m_code, 
                ImVec2(-1, ImGui::GetContentRegionAvail().y - 30), 
                ImGuiInputTextFlags_AllowTabInput);
            ImGui::EndChild();
            ImGui::PopStyleColor();
            
            // Buttons
            if (m_pad.IsRunning()) {
                if (ImGui::Button("Stop")) {
                    m_pad.Stop();
                }
            } else {
                if (ImGui::Button("Run (Ctrl+Enter)")) {
                    RunCode();
                }
            }
            
            // Console output
            ImGui::Separator();
            ImGui::Text("Console:");
            ImGui::BeginChild("Console", ImVec2(0, 100), true);
            for (const auto& line : m_output) {
                ImVec4 color = ImVec4(1,1,1,1);
                if (line.type == ConsoleOutput::StdErr) color = ImVec4(1, 0.3f, 0.3f, 1);
                else if (line.type == ConsoleOutput::Info) color = ImVec4(0.5f, 0.8f, 1, 1);
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextWrapped("%s", line.text.c_str());
                ImGui::PopStyleColor();
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }
    
    void OnHotKey() {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            RunCode();
        }
    }
    
private:
    void RunCode() {
        m_pad.SetCode(m_code);
        m_pad.SetOutputCallback([this](const ConsoleOutput& out) {
            m_output.push_back(out);
            if (m_output.size() > 500) m_output.pop_front();
        });
        m_output.clear();
        m_pad.Run();
    }
    
    Scratchpad m_pad;
    std::string m_code = R"(print("Hello from Scratchpad!"))";
    std::deque<ConsoleOutput> m_output;
};