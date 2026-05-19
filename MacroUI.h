#pragma once
#include "MacroRecorder.h"
#include <imgui.h>
#include <map>

class MacroUI {
public:
    void Render(bool* open) {
        if (ImGui::Begin("Macro Recorder", open, ImGuiWindowFlags_NoCollapse)) {
            
            if (MacroRecorder::Instance().IsRecording()) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button("Stop Recording")) {
                    StopRecording();
                }
                ImGui::PopStyleColor();
                
                ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Recording... %d actions", 
                    GetCurrentActionCount());
            } else {
                if (ImGui::Button("Start Recording")) {
                    StartRecording();
                }
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Save Macro")) {
                SaveCurrentMacro();
            }
            
            ImGui::Separator();
            ImGui::Text("Saved Macros:");
            ImGui::BeginChild("MacroList", ImVec2(0, 0), true);
            
            for (const auto& macro : m_savedMacros) {
                if (ImGui::TreeNode(macro.first.c_str())) {
                    if (ImGui::Button(("Run##" + macro.first).c_str())) {
                        ExecuteMacro(macro.first);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(("Delete##" + macro.first).c_str())) {
                        m_savedMacros.erase(macro.first);
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }
    
private:
    void StartRecording() {
        char name[64];
        sprintf_s(name, "macro_%d", m_macroCounter++);
        MacroRecorder::Instance().StartRecording(name);
    }
    
    void StopRecording() {
        auto macro = MacroRecorder::Instance().StopAndGetMacro();
        if (macro && !macro->GetActions().empty()) {
            m_savedMacros[macro->GetName()] = macro->GenerateScript();
        }
    }
    
    void SaveCurrentMacro() {
        // Сохранить в файл
        auto macro = MacroRecorder::Instance().StopAndGetMacro();
        if (macro) {
            std::string path = "macros/" + macro->GetName() + ".js";
            FILE* f = fopen(path.c_str(), "w");
            if (f) {
                fprintf(f, "%s", macro->GenerateScript().c_str());
                fclose(f);
            }
        }
    }
    
    void ExecuteMacro(const std::string& name) {
        // Найти и выполнить макрос
        auto it = m_savedMacros.find(name);
        if (it != m_savedMacros.end()) {
            MacroExecutor::ExecuteFromString(it->second, m_editor);
        }
    }
    
    int GetCurrentActionCount() {
        // Счётчик действий - упрощённый
        return 0;
    }
    
    std::map<std::string, std::string> m_savedMacros;
    int m_macroCounter = 0;
    EditorInterface* m_editor = nullptr;
};