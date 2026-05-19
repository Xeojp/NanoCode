#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

struct CompileError {
    std::string file;
    int line;
    std::string message;
};

class ContextCollector {
public:
    struct Context {
        std::string selection;
        std::string currentFile;
        std::string filePath;
        std::string projectTree;
        std::vector<CompileError> errors;
    };
    
    // Сбор полного контекста
    Context CollectContext(const std::string& selection, const std::string& currentFile, 
                          const std::string& filePath) {
        Context ctx;
        ctx.selection = selection;
        ctx.currentFile = currentFile;
        ctx.filePath = filePath;
        
        // 3. Структура проекта
        ctx.projectTree = BuildProjectTree(GetWorkspaceRoot(filePath));
        
        // 4. Ошибки компиляции (из кэша)
        ctx.errors = GetRecentErrors();
        
        return ctx;
    }
    
    // Форматирование контекста для промпта
    std::string FormatContext(const Context& ctx) {
        std::string prompt;
        
        // Выделенный код
        if (!ctx.selection.empty()) {
            prompt += "## Selected code:\n```\n" + ctx.selection + "\n```\n\n";
        }
        
        // Текущий файл  
        if (!ctx.currentFile.empty()) {
            prompt += "## Current file (" + ctx.filePath + "):\n```cpp\n" + 
                     ctx.currentFile.substr(0, 2000) + "\n```\n\n";
        }
        
        // Дерево проекта
        if (!ctx.projectTree.empty()) {
            prompt += "## Project structure:\n```\n" + ctx.projectTree + "\n```\n\n";
        }
        
        // Ошибки
        if (!ctx.errors.empty()) {
            prompt += "## Recent compile errors:\n";
            for (const auto& err : ctx.errors) {
                prompt += err.file + ":" + std::to_string(err.line) + ": " + err.message + "\n";
            }
            prompt += "\n";
        }
        
        return prompt;
    }
    
    // Установка выделения и ошибок
    void SetSelection(const std::string& sel) { m_selectionCache = sel; }
    void AddError(const CompileError& err) {
        m_errorCache.push_back(err);
        if (m_errorCache.size() > 10) m_errorCache.erase(m_errorCache.begin());
    }
    
private:
    std::string m_selectionCache;
    std::vector<CompileError> m_errorCache;
    
    std::string GetWorkspaceRoot(const std::string& filePath) {
        // Находим корень workspace (папка с .git или src)
        std::filesystem::path p(filePath);
        while (p.has_parent_path()) {
            if (std::filesystem::exists(p / ".git") || 
                std::filesystem::exists(p / "CMakeLists.txt")) {
                return p.string();
            }
            p = p.parent_path();
        }
        return filePath;
    }
    
    std::string BuildProjectTree(const std::string& root) {
        std::string tree;
        BuildTreeRecursive(root, tree, 0);
        return tree;
    }
    
    void BuildTreeRecursive(const std::string& path, std::string& out, int depth) {
        static const std::vector<std::string> skip = {".git", "build", "bin", "obj", "Debug", "Release"};
        
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            bool shouldSkip = false;
            for (const auto& s : skip) {
                if (entry.path().filename() == s) {
                    shouldSkip = true;
                    break;
                }
            }
            if (shouldSkip) continue;
            
            out += std::string(depth * 2, ' ') + (entry.is_directory() ? "📁 " : "📄 ") + 
                   entry.path().filename().string() + "\n";
            
            if (entry.is_directory() && depth < 3) {
                BuildTreeRecursive(entry.path().string(), out, depth + 1);
            }
        }
    }
    
    std::vector<CompileError> GetRecentErrors() {
        // Возвращаем последние 3 уникальные ошибки
        std::vector<CompileError> result;
        int count = 0;
        
        for (auto it = m_errorCache.rbegin(); it != m_errorCache.rend() && count < 3; ++it) {
            result.push_back(*it);
            ++count;
        }
        return result;
    }
};