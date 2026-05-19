#pragma once
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <future>
#include <atomic>

using Microsoft::WRL::ComPtr;

struct TextVertex {
    float x, y, u, v;
    uint32_t color;
};

struct GlyphInfo {
    float x, y, width, height;
    uint32_t texCoord;
    char32_t ch;
};

class TextRenderer {
public:
    struct RenderConfig {
        float fontSize = 14.0f;
        float lineHeight = 18.0f;
        uint32_t bgColor = 0xFF1E1E1E;
        uint32_t textColor = 0xFFD4D4D4;
    };

    bool Initialize(ID3D11Device* device, IDWriteFactory* dwriteFactory);
    void Render(ID3D11DeviceContext* ctx, const std::string& text, float x, float y);
    
private:
    ComPtr<ID2D1Factory> m_d2dFactory;
    ComPtr<ID2D1RenderTarget> m_rt;
    ComPtr<IDWriteTextFormat> m_textFormat;
    ComPtr<ID2D1SolidColorBrush> m_brush;
    ComPtr<ID3D11Texture2D> m_texture;
    ComPtr<ID3D11ShaderResourceView> m_srv;
};

// Асинхронная подсветка синтаксиса
class SyntaxHighlighter {
public:
    struct Token {
        size_t start, end;
        uint32_t color;
        enum Type { Keyword, String, Comment, Number, Operator, Default };
    };

    // Виртуальный буфер для больших файлов
    class VirtualBuffer {
    public:
        struct Line {
            size_t offset;
            size_t length;
            mutable std::atomic<uint32_t> version{0};
        };

        void LoadFile(const char* path);
        const Line& GetLine(size_t index) const;
        std::string GetLineText(size_t index) const;
        size_t LineCount() const { return m_lines.size(); }

    private:
        std::string m_fileData;
        std::vector<Line> m_lines;
        mutable std::shared_mutex m_mutex;
    };

    // Инкрементальная подсветка - только видимые строки + буфер
    void HighlightRange(VirtualBuffer& buf, size_t startLine, size_t endLine);
    
    // Компиляция в background thread
    std::future<std::vector<Token>> HighlightAsync(const std::string& text, size_t fromLine, size_t lineCount);

private:
    std::unordered_map<std::string, uint32_t> m_keywords;
    std::vector<std::future<std::vector<Token>>> m_pendingJobs;
};

// Оптимизированный рендерер текста для больших файлов
class TextEngine {
public:
    struct Viewport {
        float x, y, width, height;
        size_t startLine, endLine;
    };

    void Render(ID3D11DeviceContext* ctx, const SyntaxHighlighter::VirtualBuffer& buf, 
                const Viewport& vp, float scrollY);
    
    // Dirty rects - обновляем только измененные строки
    void MarkLineDirty(size_t lineIndex);
    void InvalidateCache(size_t fromLine, size_t count);

private:
    struct LineCacheEntry {
        ComPtr<IDWriteTextLayout> layout;
        std::vector<GlyphInfo> glyphs;
        uint32_t version = 0;
        bool highlighted = false;
    };

    std::vector<LineCacheEntry> m_lineCache;
    std::vector<size_t> m_dirtyLines;
    TextRenderer m_renderer;
    
    // Glyph atlas (90% строк использует ~10% символов)
    std::unordered_map<char32_t, GlyphInfo> m_glyphAtlas;
    std::vector<uint8_t> m_atlasData;
    
    SyntaxHighlighter m_highlighter;
    std::unordered_map<size_t, std::future<std::vector<SyntaxHighlighter::Token>>> m_pendingTokens;
};