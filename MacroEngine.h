#pragma once
#include <string>
#include <vector>
#include <functional>
#include <sstream>

// Action - действие пользователя
enum class ActionType {
    SelectLine,
    DeleteSpaces,
    WrapTryCatch,
    InsertText,
    MoveCursor,
    DeleteLine,
    DuplicateLine
};

struct Action {
    ActionType type;
    std::string param;  // для InsertText, MoveCursor
    int line = -1;      // номер строки
    int pos = -1;       // позиция
    
    // Сериализация в скрипт
    std::string ToScript() const {
        std::ostringstream ss;
        switch (type) {
            case ActionType::SelectLine:
                ss << "editor.select_line(" << line << ");";
                break;
            case ActionType::DeleteSpaces:
                ss << "editor.trim_spaces();";
                break;
            case ActionType::WrapTryCatch:
                ss << "editor.wrap_try_catch();";
                break;
            case ActionType::InsertText:
                ss << "editor.insert('" << param << "');";
                break;
            case ActionType::MoveCursor:
                ss << "editor.move(" << line << ", " << pos << ");";
                break;
            default:
                ss << "// unknown action";
        }
        return ss.str();
    }
};

// Макрос - последовательность действий
class Macro {
public:
    Macro(const std::string& name = "") : m_name(name) {}
    
    void AddAction(const Action& action) {
        m_actions.push_back(action);
    }
    
    // Генерация кода скрипта
    std::string GenerateScript() const {
        std::ostringstream ss;
        ss << "// Macro: " << m_name << "\n";
        ss << "function run_macro() {\n";
        for (const auto& action : m_actions) {
            ss << "  " << action.ToScript() << "\n";
        }
        ss << "}\n";
        return ss.str();
    }
    
    // Выполнение действий
    void Execute(class EditorInterface* editor) {
        for (const auto& action : m_actions) {
            ExecuteAction(editor, action);
        }
    }
    
    const std::vector<Action>& GetActions() const { return m_actions; }
    const std::string& GetName() const { return m_name; }
    
private:
    void ExecuteAction(EditorInterface* editor, const Action& action) {
        switch (action.type) {
            case ActionType::SelectLine:
                editor->SelectLine(action.line);
                break;
            case ActionType::DeleteSpaces:
                editor->TrimSpaces();
                break;
            case ActionType::WrapTryCatch:
                editor->WrapTryCatch();
                break;
            case ActionType::InsertText:
                editor->InsertText(action.param);
                break;
        }
    }
    
    std::string m_name;
    std::vector<Action> m_actions;
};