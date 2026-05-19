#pragma once
#include "AIAssistant.h"
#include "ContextCollector.h"
#include <imgui.h>
#include <deque>
#include <future>
#include <sstream>

struct CodeBlock {
    std::string language;
    std::string code;
    ImVec2 size;
};

class AIAssistantWindow {
public:
    enum class State { Collapsed, Visible, Floating };
    
    AIAssistantWindow() : m_state(State::Visible), m_inputActive(false) {
        m_dockId = ImGui::GetID("AIAssistant");
    }
    
    // Установка контекста
    void SetCurrentFile(const std::string& path, const std::string& content) {
        m_context.filePath = path;
        m_context.currentFile = content;
    }
    
    void SetSelection(const std::string& sel) {
        m_context.selection = sel;
    }
    
    void AddCompileError(const CompileError& err) {
        m_collector.AddError(err);
    }
    
    void SubmitQuery() {
        if (m_inputBuffer.empty()) return;
        
        // Собираем контекст
        auto ctx = m_collector.CollectContext(m_context.selection, m_context.currentFile, 
                                             m_context.filePath);
        std::string fullPrompt = m_collector.FormatContext(ctx) + "\n## Question:\n" + m_inputBuffer;
        
        auto question = m_inputBuffer;
        m_inputBuffer.clear();
        m_inputActive = false;
        
        m_messages.push_back({"user", question});
        m_thinking = true;
        m_ai.StreamChat({{"user", fullPrompt}}, {
            [this](const std::string& chunk) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_pendingChunks += chunk;
            },
            [this](const std::string& err) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_thinking = false;
                m_error = err;
            },
            [this]() {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_thinking = false;
            }
        }, "deepseek/deepseek-chat:free");
    }
    
    void Render(bool* open) {
        if (m_state == State::Collapsed) return;
        
        ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.95f);
        
        if (ImGui::Begin("AI Assistant", open, 
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
            
            // Apply pending chunks to UI
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (!m_pendingChunks.empty()) {
                    if (m_messages.empty() || m_messages.back().role != "assistant") {
                        m_messages.push_back({"assistant", ""});
                    }
                    m_messages.back().content += m_pendingChunks;
                    m_pendingChunks.clear();
                }
            }
            
            // Chat history
            ImGui::BeginChild("ChatHistory", ImVec2(0, -80), true);
            for (const auto& msg : m_messages) {
                if (msg.role == "user") {
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
                    ImGui::TextWrapped("You: %s", msg.content.c_str());
                    ImGui::PopStyleColor();
                } else {
                    RenderAIMessage(msg.content);
                }
            }
            
            // Auto-scroll to bottom
            if (m_autoScroll && ImGui::GetScrollY() < ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0f);
            }
            ImGui::EndChild();
            
            // Loading indicator
            if (m_thinking) {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "AI is thinking...");
            }
            
            // Input
            ImGui::Separator();
            if (m_inputActive) {
                if (ImGui::InputTextMultiline("##input", &m_inputBuffer, 
                    ImVec2(-100, 40), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    SubmitQuery();
                }
                ImGui::SameLine();
                if (ImGui::Button("Send")) {
                    SubmitQuery();
                }
            } else {
                if (ImGui::InputTextWithHint("##input", "Ask AI...", &m_inputBuffer, 
                    ImGuiInputTextFlags_EnterReturnsTrue)) {
                    SubmitQuery();
                }
            }
        }
        ImGui::End();
    }
    
    void InsertCodeAtCursor(const std::string& code) {
        if (m_insertCodeAtCursor) {
            m_insertCodeAtCursor(code);
        }
    }
    
    std::function<void(const std::string&)> m_insertCodeAtCursor;
    
private:
    struct Message {
        std::string role;
        std::string content;
    };
    
    void RenderAIMessage(const std::string& content) {
        auto blocks = ParseMarkdown(content);
        
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        ImGui::TextWrapped("AI:");
        ImGui::PopStyleColor();
        
        for (const auto& block : blocks) {
            if (block.language.empty()) {
                ImGui::TextWrapped("%s", block.code.c_str());
            } else {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
                ImGui::BeginChild("code_block", ImVec2(0, 120), true);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
                ImGui::TextUnformatted(block.code.c_str());
                ImGui::PopStyleColor();
                if (ImGui::Button("Insert Code")) {
                    InsertCodeAtCursor(block.code);
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }
        }
    }
    
    std::vector<CodeBlock> ParseMarkdown(const std::string& markdown) {
        std::vector<CodeBlock> blocks;
        std::istringstream stream(markdown);
        std::string line;
        CodeBlock current;
        bool inCode = false;
        
        while (std::getline(stream, line)) {
            if (line.size() >= 3 && line.substr(0, 3) == "```") {
                if (inCode) {
                    blocks.push_back(current);
                    current = {};
                    inCode = false;
                } else {
                    inCode = true;
                    current.language = line.substr(3);
                }
            } else if (inCode) {
                current.code += line + "\n";
            } else {
                if (!current.code.empty() || !current.language.empty()) {
                    blocks.push_back(current);
                    current = {};
                }
                current.code = line + "\n";
            }
        }
        
        if (!current.code.empty()) blocks.push_back(current);
        return blocks;
    }
    
    State m_state;
    bool m_inputActive;
    bool m_thinking = false;
    bool m_autoScroll = true;
    std::string m_inputBuffer;
    std::string m_pendingChunks;
    std::string m_error;
    int m_codeCounter = 0;
    
    std::vector<Message> m_messages;
    AIAssistant m_ai;
    
    ContextCollector m_collector;
    ContextCollector::Context m_context;
    
    std::mutex m_mutex;
    ImGuiID m_dockId;
};