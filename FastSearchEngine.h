#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <future>
#include <filesystem>
#include <fstream>
#include <algorithm>

struct SearchResult {
    std::string filePath;
    int lineNumber;
    std::string lineContent;
    int matchColumn;
};

class FastSearchEngine {
public:
    struct SearchConfig {
        bool caseSensitive = false;
        bool wholeWord = false;
        bool useRegex = false;
        std::vector<std::string> includeExts = {".cpp", ".h", ".py", ".js", ".ts", ".rs", ".go"};
    };
    
    static FastSearchEngine& Instance() {
        static FastSearchEngine instance;
        return instance;
    }
    
    // Запустить индексацию в фоне
    void StartIndexing(const std::string& rootPath) {
        if (m_indexing) return;
        m_indexing = true;
        m_rootPath = rootPath;
        m_indexFuture = std::async(std::launch::async, [this]() {
            BuildIndex();
            m_indexing = false;
        });
    }
    
    // Поиск (многопоточный)
    std::vector<SearchResult> Search(const std::string& query, 
                                    const SearchConfig& config = {}) {
        std::vector<SearchResult> results;
        
        // Используем предвычисленные блоки
        auto blocks = GetMatchingBlocks(query);
        
        // Параллельный поиск по блокам
        std::vector<std::future<std::vector<SearchResult>>> futures;
        for (const auto& block : blocks) {
            futures.push_back(std::async(std::launch::async, [&]() {
                return SearchInBlock(block, query, config);
            }));
        }
        
        for (auto& f : futures) {
            auto partial = f.get();
            results.insert(results.end(), partial.begin(), partial.end());
        }
        
        return results;
    }
    
    // Поиск с Bloom filter предфильтрацией
    std::vector<SearchResult> SearchFast(const std::string& query) {
        std::vector<SearchResult> results;
        
        // Bloom filter быстро отсекает отсутствующие термины
        if (!m_bloomFilter.Contains(query)) {
            return results;
        }
        
        // Прямой поиск в индексе
        auto it = m_invertedIndex.find(query);
        if (it != m_invertedIndex.end()) {
            for (const auto& ref : it->second) {
                results.push_back(GetResultFromRef(ref));
            }
        }
        
        return results;
    }
    
    bool IsIndexing() const { return m_indexing || m_indexFuture.valid(); }
    
private:
    struct FileBlock {
        std::string filePath;
        size_t offset;
        size_t size;
        std::string content; // memory-mapped slice
    };
    
    struct LineRef {
        std::string filePath;
        uint32_t lineNumber : 24;
        uint32_t column : 8;
    };
    
    // Bloom filter для быстрой проверки наличия
    class BloomFilter {
        std::vector<uint64_t> m_bits;
        size_t m_k; // количество хешей
        
    public:
        void Add(const std::string& s) {
            for (size_t i = 0; i < m_k; ++i) {
                m_bits[hash(s + std::to_string(i)) % m_bits.size()] = 1;
            }
        }
        
        bool Contains(const std::string& s) {
            for (size_t i = 0; i < m_k; ++i) {
                if (!m_bits[hash(s + std::to_string(i)) % m_bits.size()]) {
                    return false;
                }
            }
            return true;
        }
    };
    
    void BuildIndex() {
        m_invertedIndex.reserve(1000000);
        
        for (const auto& entry : std::filesystem::recursive_directory_iterator(m_rootPath)) {
            if (!entry.is_regular_file()) continue;
            
            std::string ext = entry.path().extension().string();
            bool include = std::find(m_config.includeExts.begin(), 
                                   m_config.includeExts.end(), ext) != m_config.includeExts.end();
            if (!include) continue;
            
            IndexFile(entry.path().string());
        }
    }
    
    void IndexFile(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file) return;
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        // Индексируем каждую строку
        std::istringstream stream(content);
        std::string line;
        int lineNum = 0;
        
        while (std::getline(stream, line)) {
            lineNum++;
            
            // Токенизация строки (4-буквенные комбинации)
            for (size_t i = 0; i + 4 <= line.size(); ++i) {
                std::string token = line.substr(i, 4);
                if (token.length() == 4) {
                    m_invertedIndex[token].push_back({filePath, lineNum, (uint8_t)i});
                    m_bloomFilter.Add(token);
                }
            }
        }
    }
    
    std::vector<FileBlock> GetMatchingBlocks(const std::string& query) {
        std::vector<FileBlock> result;
        // Используем первые 4 символа для поиска в блоках
        if (query.size() >= 4) {
            std::string key = query.substr(0, 4);
            auto it = m_blocks.find(key);
            if (it != m_blocks.end()) {
                result.push_back(it->second);
            }
        }
        return result;
    }
    
    std::vector<SearchResult> SearchInBlock(const FileBlock& block, 
                                           const std::string& query,
                                           const SearchConfig& config) {
        std::vector<SearchResult> results;
        
        std::istringstream stream(block.content);
        std::string line;
        int lineNum = 0;
        
        while (std::getline(stream, line)) {
            lineNum++;
            size_t pos = line.find(query);
            if (pos != std::string::npos) {
                if (config.wholeWord) {
                    bool wordBoundary = (pos == 0 || !isalnum(line[pos-1])) &&
                                       (pos + query.size() == line.size() || !isalnum(line[pos + query.size()]));
                    if (!wordBoundary) continue;
                }
                results.push_back({block.filePath, lineNum, line, (int)pos});
            }
        }
        
        return results;
    }
    
    SearchResult GetResultFromRef(const LineRef& ref) {
        return {ref.filePath, ref.lineNumber, "", ref.column};
    }
    
    // Хеш для Bloom filter
    uint64_t hash(const std::string& s) {
        uint64_t h = 0;
        for (char c : s) {
            h = h * 31 + c;
        }
        return h;
    }
    
    std::unordered_map<std::string, std::vector<LineRef>> m_invertedIndex;
    std::unordered_map<std::string, FileBlock> m_blocks;
    BloomFilter m_bloomFilter;
    SearchConfig m_config;
    std::string m_rootPath;
    std::atomic<bool> m_indexing{false};
    std::future<void> m_indexFuture;
};