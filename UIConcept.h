#pragma once

// Minimal IDE UI Concept
// Design Philosophy: Content-first, progressive disclosure

enum class PanelType {
    Explorer,
    Search,
    Git,
    Debug,
    Extensions
};

enum class DockPos {
    Left,
    Right,
    Bottom,
    Floating,
    Hidden
};

struct PanelState {
    bool visible = false;
    DockPos dock = DockPos::Left;
    float width = 250.0f;
    bool autoHide = false;
    float hoverWidth = 50.0f;
    
    // Animation
    float animProgress = 0.0f;
    float targetWidth = 250.0f;
};

class UIManager {
public:
    // Command Palette - показывается по Ctrl+P
    // Открывается за ~50ms (предзагружен в памяти)
    void OpenCommandPalette();
    void UpdateCommandPalette(const char* query);
    
    // Tab система
    class Tab {
    public:
        enum class Type { Editor, Preview, Diff, AiChat };
        std::string title;
        Type type = Type::Editor;
        bool pinned = false;
        bool dirty = false;
        float animProgress = 1.0f; // для анимации закрытия
    };
    
    void AddTab(const Tab& tab);
    void CloseTab(size_t index);
    void SelectTab(size_t index);
    
    // AI Assistant Window
    class AiWindow {
    public:
        enum class Mode { Docked, Floating, Hidden };
        Mode mode = Mode::Docked;
        float opacity = 1.0f;
        bool followCursor = false; // AI "думает" за курсором
        
        // Быстрый доступ к часто используемым командам
        struct QuickAction {
            std::string icon;
            std::string name;
            std::string shortcut;
        };
    };

private:
    PanelState m_panels[5];
    std::vector<Tab> m_tabs;
    size_t m_activeTab = 0;
    AiWindow m_aiWindow;
    
    // Command Palette данные - предзагружены
    std::vector<std::string> m_commands;
    std::vector<std::string> m_files;
    std::vector<std::string> m_symbols;
};

// Лучшие практики для 50+ функций:

// 1. Progressive Disclosure Pattern
//    - Основная функциональность видна сразу
//    - Дополнительная скрыта за значками "..." или Ctrl+Shift+P
//    - Настройки → "Show advanced"

// 2. Mnemonic Groups
//    - File/Edit/View (основные)
//    - Go/Find/Run (работа с кодом)
//    - Terminal/Debug/AI (вспомогательные)

// 3. Context Menus
//    - Правый клик → только релевантные 5-7 пунктов
//    - Скрытые подменю появляются по наведению

// 4. Command Palette как основной UI
//    - 80% функций доступно через поиск
//    - Fuzzy search с подсветкой совпадений
//    - Ctrl+T для файлов, Ctrl+Shift+O для символов

// 5. Minimal Status Bar
//    - Только выбор ветки Git, позиция курсора
//    - Остальное в попапе по клику

// 6. Zen Mode
//    - F11: скрыть все панели, оставить только редактор
//    - Центрированный текст 80-100 символов

// 7. Activity Bar (слева)
//    6 иконок максимум:
//    - Explorer (Ctrl+Shift+E)
//    - Search (Ctrl+Shift+F)  
//    - Git (Ctrl+Shift+G)
//    - Debug (F5)
//    - AI (Ctrl+Shift+A)
//    - Extensions

// 8. Breadcrumbs внизу
//    - Путь к файлу → класс → метод
//    - Кликабельные сегменты

// 9. Smart Animations
//    - Плавность: 120-144 FPS
//    - Длительность: 100-150ms
//    - Ease-out функции

// 10. Modal Windows
//     - AI Assistant - Esc для закрытия
//     - Settings - сохранение по Ctrl+S