#pragma once
#include "MacroEngine.h"
#include <memory>
#include <atomic>

// Интерфейс для взаимодействия с редактором
class EditorInterface {
public:
    virtual void SelectLine(int line) = 0;
    virtual void TrimSpaces() = 0;
    virtual void WrapTryCatch() = 0;
    virtual void InsertText(const std::string& text) = 0;
    virtual void MoveCursor(int line, int pos) = 0;
    virtual std::pair<int, int> GetCursorPos() = 0;
    virtual std::string GetSelectedText() = 0;
};

// Macro Recorder - записывает действия
class MacroRecorder {
public:
    static MacroRecorder& Instance() {
        static MacroRecorder instance;
        return instance;
    }
    
    void StartRecording(const std::string& name) {
        m_recording = true;
        m_macro = std::make_unique<Macro>(name);
    }
    
    void StopRecording() {
        m_recording = false;
    }
    
    bool IsRecording() const { return m_recording; }
    
    // Вызывается при каждом действии пользователя
    void RecordAction(const Action& action) {
        if (m_recording && m_macro) {
            m_macro->AddAction(action);
        }
    }
    
    std::unique_ptr<Macro> StopAndGetMacro() {
        m_recording = false;
        return std::move(m_macro);
    }
    
    // Hook для перехвата действий в редакторе
    void OnAction(const std::function<Action()>& actionCreator) {
        if (m_recording) {
            RecordAction(actionCreator());
        }
    }
    
private:
    std::atomic<bool> m_recording{false};
    std::unique_ptr<Macro> m_macro;
};

// Macro Executor - выполнение макросов
class MacroExecutor {
public:
    static bool ExecuteFromFile(const std::string& path, EditorInterface* editor) {
        // Реальная реализация - парсинг JS/Lua
        // Здесь упрощенная версия
        Macro macro(path);
        macro.Execute(editor);
        return true;
    }
    
    static bool ExecuteFromString(const std::string& script, EditorInterface* editor) {
        // Парсим простой DSL
        Macro macro("inline");
        
        // Tokenize and execute
        std::istringstream stream(script);
        std::string line;
        while (std::getline(stream, line)) {
            if (line.find("select_line") != std::string::npos) {
                int lineNum = 0;
                sscanf(line.c_str(), "editor.select_line(%d)", &lineNum);
                macro.AddAction({ActionType::SelectLine, "", lineNum});
            }
            else if (line.find("trim_spaces") != std::string::npos) {
                macro.AddAction({ActionType::DeleteSpaces, "", -1});
            }
            else if (line.find("wrap_try_catch") != std::string::npos) {
                macro.AddAction({ActionType::WrapTryCatch, "", -1});
            }
        }
        
        macro.Execute(editor);
        return true;
    }
};