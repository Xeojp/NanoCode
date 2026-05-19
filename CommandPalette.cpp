#include "UIConcept.h"
#include <algorithm>
#include <chrono>

// Command Palette Implementation
struct CommandPalette {
    bool isOpen = false;
    std::string query;
    std::vector<std::string> results;
    int selectedIndex = 0;
    
    float animOpen = 0.0f;    // 0.0 closed, 1.0 open
    float animHeight = 0.0f; // height animation
    
    std::chrono::steady_clock::time_point openTime;
    
    void Open() {
        isOpen = true;
        animOpen = 0.0f;
        animHeight = 0.0f;
        openTime = std::chrono::steady_clock::now();
        query.clear();
    }
    
    void Close() {
        animOpen = 1.0f; // animate out
    }
    
    // Update animation (call every frame)
    void Update(float deltaTime) {
        float target = isOpen ? 1.0f : 0.0f;
        animOpen += (target - animOpen) * deltaTime * 12.0f; // 12 = easing factor
        animHeight = std::min(600.0f, 800.0f * animOpen * animOpen); // quadratic easing
    }
    
    // Fuzzy search по предзагруженным данным
    void Search(const std::vector<std::string>& commands, const std::vector<std::string>& files) {
        results.clear();
        
        // Оптимизация: ищем только в первых 3-х символах запроса
        if (query.size() < 2) {
            // Показываем недавние команды
            for (int i = 0; i < 10 && i < commands.size(); ++i) {
                results.push_back(commands[i]);
            }
            return;
        }
        
        // Быстрый fuzzy search O(n) вместо O(n²)
        for (const auto& cmd : commands) {
            if (FuzzyMatch(query, cmd)) {
                results.push_back(cmd);
                if (results.size() > 50) break; // лимит
            }
        }
    }
    
private:
    bool FuzzyMatch(const std::string& pattern, const std::string& text) {
        size_t pi = 0, ti = 0;
        while (pi < pattern.size() && ti < text.size()) {
            if (tolower(pattern[pi]) == tolower(text[ti])) pi++;
            ti++;
        }
        return pi == pattern.size();
    }
};

// Анимированные табы
struct AnimatedTabs {
    struct TabAnim {
        float x, width;
        float scale = 1.0f;
        float opacity = 1.0f;
    };
    
    std::vector<TabAnim> anims;
    
    void AnimateTabClose(size_t index, float deltaTime) {
        // Анимация "схлопывания" таба
        anims[index].scale += deltaTime * 8.0f; // ускоряем
        anims[index].opacity -= deltaTime * 6.0f;
        anims[index].width = std::max(0.0f, anims[index].width - deltaTime * 200.0f);
    }
    
    void RenderTab(const UIManager::Tab& tab, const TabAnim& anim) {
        // Горячие клавиши для табов: Ctrl+1, Ctrl+2...
    }
};