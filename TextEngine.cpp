#include "TextEngine.h"
#include <thread>
#include <algorithm>

bool TextRenderer::Initialize(ID3D11Device* device, IDWriteFactory* dwriteFactory) {
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_d2dFactory);
    
    ComPtr<IDXGIDevice> dxgiDevice;
    device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgiDevice.GetAddressOf()));
    
    ComPtr<ID2D1Device> d2dDevice;
    m_d2dFactory->CreateDevice(dxgiDevice.Get(), nullptr, &d2dDevice);
    
    ComPtr<ID2D1DeviceContext> dc;
    d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &dc);
    m_rt = dc.Detach();

    dwriteFactory->CreateTextFormat(L"Consolas", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
                                    DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                    14.0f, L"", &m_textFormat);
    m_textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    m_rt->CreateSolidColorBrush(D2D1::ColorF(0xD4D4D4), &m_brush);
    return true;
}

void TextRenderer::Render(ID3D11DeviceContext* ctx, const std::string& text, float x, float y) {
    if (!m_rt || text.empty()) return;
    
    m_rt->BeginDraw();
    m_rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    m_rt->DrawText(text.c_str(), text.length(), m_textFormat.Get(), 
                   D2D1::RectF(x, y, x + 10000, y + 100), m_brush.Get());
    m_rt->EndDraw();
}

void SyntaxHighlighter::HighlightRange(VirtualBuffer& buf, size_t startLine, size_t endLine) {
    // Ограничиваем диапазон видимыми строками + 50% запас
    size_t maxLines = std::min(endLine - startLine, size_t(500));
    
    for (size_t i = 0; i < maxLines; ++i) {
        size_t lineIdx = startLine + i;
        if (lineIdx >= buf.LineCount()) break;
        
        auto future = HighlightAsync(buf.GetLineText(lineIdx), lineIdx, 1);
        m_pendingJobs.push_back(std::move(future));
    }
    
    // Очищаем завершенные задачи
    m_pendingJobs.erase(
        std::remove_if(m_pendingJobs.begin(), m_pendingJobs.end(),
            [](auto& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }),
        m_pendingJobs.end()
    );
}

std::future<std::vector<SyntaxHighlighter::Token>> 
SyntaxHighlighter::HighlightAsync(const std::string& text, size_t fromLine, size_t lineCount) {
    return std::async(std::launch::async, [text, fromLine, lineCount]() {
        std::vector<Token> tokens;
        
        enum State { Default, InString, InComment, InNumber } state = Default;
        uint32_t color = 0xFFD4D4D4;
        
        for (size_t i = 0; i < text.size(); ++i) {
            char c = text[i];
            
            switch (state) {
            case Default:
                if (c == '"') { state = InString; color = 0xFFCE9178; }
                else if (c == '/' && i + 1 < text.size() && text[i+1] == '/') { 
                    state = InComment; color = 0xFF6A9955; 
                }
                else if (std::isdigit(c)) { state = InNumber; color = 0xFFB5CEA8; }
                break;
            case InString:
                if (c == '"') { state = Default; color = 0xFFD4D4D4; }
                break;
            case InComment:
                break;
            case InNumber:
                if (!std::isdigit(c) && c != '.') { state = Default; color = 0xFFD4D4D4; }
                break;
            }
            
            tokens.push_back({i, i+1, color, Token::Default});
        }
        return tokens;
    });
}

void TextEngine::Render(ID3D11DeviceContext* ctx, const SyntaxHighlighter::VirtualBuffer& buf,
                        const Viewport& vp, float scrollY) {
    // Подсветка только видимых строк
    size_t startLine = vp.startLine;
    size_t endLine = std::min(vp.endLine, buf.LineCount());
    
    for (size_t line = startLine; line < endLine; ++line) {
        auto future = m_highlighter.HighlightAsync(buf.GetLineText(line), line, 1);
        m_pendingTokens[line] = std::move(future);
    }
}

void TextEngine::MarkLineDirty(size_t lineIndex) {
    m_dirtyLines.push_back(lineIndex);
}

void TextEngine::InvalidateCache(size_t fromLine, size_t count) {
    for (size_t i = 0; i < count && fromLine + i < m_lineCache.size(); ++i) {
        m_lineCache[fromLine + i].version++;
    }
}