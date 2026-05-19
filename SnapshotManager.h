#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <filesystem>
#include <algorithm>

struct Snapshot {
    std::string content;
    std::chrono::system_clock::time_point timestamp;
    size_t size = 0;
    std::string hash; // для быстрого сравнения
};

class DeltaTracker {
public:
    // Вычисление разницы между двумя версиями (Myers diff)
    static std::vector<std::string> ComputeDiff(const std::string& old, const std::string& current) {
        // Упрощенный LCS diff - возвращает добавленные/удаленные строки
        std::vector<std::string> changes;
        std::istringstream oldStream(old);
        std::istringstream curStream(current);
        std::string oldLine, curLine;
        
        int lineNum = 0;
        while (std::getline(oldStream, oldLine) || std::getline(curStream, curLine)) {
            if (!oldStream && curStream) {
                while (std::getline(curStream, curLine)) {
                    changes.push_back("+ " + curLine);
                }
                break;
            }
            if (oldStream && !curStream) {
                changes.push_back("- " + oldLine);
            } else if (oldLine != curLine) {
                changes.push_back("- " + oldLine);
                changes.push_back("+ " + curLine);
            }
            oldLine.clear();
            curLine.clear();
            lineNum++;
        }
        return changes;
    }
    
    // Применение дельты к базовой версии
    static std::string ApplyDelta(const std::string& base, const std::vector<std::string>& delta) {
        std::istringstream stream(base);
        std::ostringstream result;
        std::string line;
        int lineNum = 0;
        
        while (std::getline(stream, line)) {
            bool modified = false;
            for (const auto& change : delta) {
                if (change.substr(2) == line) {
                    if (change[0] == '-') {
                        modified = true;
                        break;
                    }
                }
            }
            if (!modified) {
                result << line << "\n";
            }
        }
        return result.str();
    }
};

class SnapshotManager {
public:
    struct FileHistory {
        std::string filePath;
        std::string currentContent; // для быстрого сравнения
        std::vector<std::pair<size_t, std::vector<std::string>>> deltas; // hash -> delta
        std::vector<std::chrono::system_clock::time_point> timestamps;
        size_t maxSnapshots = 20;
    };
    
    static SnapshotManager& Instance() {
        static SnapshotManager instance;
        return instance;
    }
    
    // Создание снимка
    void CreateSnapshot(const std::string& filePath, const std::string& content, 
                     const std::string& reason = "save") {
        auto hash = ComputeHash(content);
        
        auto& history = m_files[filePath];
        
        // Проверяем, нужно ли сохранять (изменилось ли содержимое)
        if (!history.currentContent.empty() && history.currentContent == content) {
            return;
        }
        
        // Вычисляем дельту
        std::vector<std::string> delta;
        if (!history.currentContent.empty()) {
            delta = DeltaTracker::ComputeDiff(history.currentContent, content);
            history.deltas.push_back({std::stoull(hash.substr(0, 16), nullptr, 16), delta});
        }
        
        history.currentContent = content;
        history.timestamps.push_back(std::chrono::system_clock::now());
        
        // Ограничиваем количество снимков
        while (history.timestamps.size() > history.maxSnapshots) {
            history.timestamps.erase(history.timestamps.begin());
            if (!history.deltas.empty()) {
                history.deltas.erase(history.deltas.begin());
            }
        }
    }
    
    // Получить содержимое на N шагов назад
    std::string GetVersion(const std::string& filePath, int stepsBack) {
        auto it = m_files.find(filePath);
        if (it == m_files.end() || stepsBack <= 0) {
            return "";
        }
        
        auto& history = it->second;
        if (stepsBack >= history.timestamps.size()) {
            stepsBack = history.timestamps.size() - 1;
        }
        
        // Восстанавливаем через дельты
        std::string content = history.currentContent;
        for (int i = 0; i < stepsBack && !history.deltas.empty(); ++i) {
            // Обратная дельта (swap +/-)
            std::vector<std::string> reverseDelta;
            for (const auto& change : history.deltas[history.deltas.size() - 1 - i].second) {
                if (change[0] == '+') reverseDelta.push_back("-" + change.substr(1));
                else if (change[0] == '-') reverseDelta.push_back("+" + change.substr(1));
            }
            content = DeltaTracker::ApplyDelta(content, reverseDelta);
        }
        
        return content;
    }
    
    // Получить список точек времени для UI ползунка
    std::vector<std::chrono::system_clock::time_point> GetTimeline(const std::string& filePath) {
        auto it = m_files.find(filePath);
        if (it == m_files.end()) return {};
        return it->second.timestamps;
    }
    
    // Получить имена файлов с историей
    std::vector<std::string> GetTrackedFiles() {
        std::vector<std::string> files;
        for (const auto& [path, _] : m_files) {
            files.push_back(path);
        }
        return files;
    }
    
    // Очистка старых снимков
    void CleanupOldSnapshots(const std::string& filePath, int keepLast = 10) {
        auto it = m_files.find(filePath);
        if (it == m_files.end()) return;
        
        auto& history = it->second;
        while (history.timestamps.size() > keepLast) {
            history.timestamps.erase(history.timestamps.begin());
            if (!history.deltas.empty()) {
                history.deltas.erase(history.deltas.begin());
            }
        }
    }
    
    // Сжатие дельт (удаление похожих)
    void CompressDeltas(const std::string& filePath) {
        auto it = m_files.find(filePath);
        if (it == m_files.end()) return;
        
        // Простое удаление дублирующихся дельт
        std::unordered_map<std::string, std::vector<std::string>> uniqueDeltas;
        for (auto& entry : it->second.deltas) {
            std::string key;
            for (const auto& line : entry.second) {
                key += line;
            }
            if (uniqueDeltas.find(key) == uniqueDeltas.end()) {
                uniqueDeltas[key] = entry.second;
            }
        }
        
        // Пересобираем дельты
        std::vector<std::pair<size_t, std::vector<std::string>>> newDeltas;
        for (const auto& [key, delta] : uniqueDeltas) {
            newDeltas.push_back({0, delta});
        }
        it->second.deltas = newDeltas;
    }
    
private:
    std::string ComputeHash(const std::string& content) {
        // Простой хеш CRC32
        uint32_t crc = 0;
        for (char c : content) {
            crc ^= static_cast<uint8_t>(c);
            for (int i = 0; i < 8; ++i) {
                crc = (crc >> 1) ^ (crc & 1 ? 0xEDB88320 : 0);
            }
        }
        return std::to_string(crc);
    }
    
    std::unordered_map<std::string, FileHistory> m_files;
};

// UI для навигации по версиям
class VersionControlUI {
public:
    void Render(bool* open) {
        if (ImGui::Begin("Version History", open, ImGuiWindowFlags_NoCollapse)) {
            
            // Файлы с историей
            auto files = SnapshotManager::Instance().GetTrackedFiles();
            for (const auto& path : files) {
                if (ImGui::TreeNode(path.c_str())) {
                    auto timeline = SnapshotManager::Instance().GetTimeline(path);
                    int maxSteps = static_cast<int>(timeline.size()) - 1;
                    
                    if (maxSteps > 0) {
                        int steps = m_selectedSteps[path];
                        ImGui::SliderInt(("Revert##" + path).c_str(), &steps, 0, maxSteps);
                        m_selectedSteps[path] = steps;
                        
                        if (ImGui::Button(("Apply##" + path).c_str())) {
                            auto content = SnapshotManager::Instance().GetVersion(path, steps);
                            // TODO: обновить файл в редакторе
                        }
                    }
                    ImGui::TreePop();
                }
            }
        }
        ImGui::End();
    }
    
    void SelectVersion(const std::string& filePath, int stepsBack) {
        m_selectedSteps[filePath] = stepsBack;
    }
    
private:
    std::unordered_map<std::string, int> m_selectedSteps;
};