#pragma once
#include "FastSearchEngine.h"
#include <concurrentqueue.h>
#include <future>

class AsyncSearchPipeline {
public:
    // Три этапа поиска
    struct SearchStage1 { // Bloom filter
        std::vector<std::string> candidates;
    };
    
    struct SearchStage2 { // Блоки с 4-символьными ключами
        std::vector<FastSearchEngine::FileBlock> blocks;
    };
    
    struct SearchStage3 { // Результаты
        std::vector<SearchResult> results;
    };
    
    // Комбинированный поиск
    std::future<std::vector<SearchResult>> SearchAsync(const std::string& query) {
        return std::async(std::launch::async, [query]() {
            FastSearchEngine& engine = FastSearchEngine::Instance();
            
            // Stage 1: Bloom filter (1-5ms)
            if (!engine.SearchFast(query).empty()) {
                // Stage 2: Блоки (5-20ms)
                auto results = engine.Search(query);
                // Stage 3: Сортировка и ограничение (1-5ms)
                std::sort(results.begin(), results.end(), 
                    [](const auto& a, const auto& b) {
                        return a.filePath < b.filePath;
                    });
                if (results.size() > 1000) results.resize(1000);
                return results;
            }
            return std::vector<SearchResult>();
        });
    }
};