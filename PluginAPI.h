#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

// WASM Plugin API
#ifdef __cplusplus
extern "C" {
#endif

// Core API functions (imported by plugins)
typedef struct {
    void (*log)(const char* message);
    void (*show_message)(const char* title, const char* message);
    char* (*get_active_file)();
    void (*set_active_file)(const char* path);
    int (*get_cursor_line)();
    int (*get_cursor_column)();
    void (*insert_text)(const char* text);
    void (*replace_selection)(const char* text);
    void (*run_command)(const char* command);
} IDE_API;

// Plugin exports
typedef void (*plugin_init_fn)(const IDE_API* api);
typedef void (*plugin_deinit_fn)();
typedef void (*plugin_execute_fn)(const char* input, char** output);

#ifdef __cplusplus
}
#endif

// WASM Runtime (WASI-compatible)
class WASMRuntime {
public:
    WASMRuntime();
    ~WASMRuntime();
    
    bool LoadPlugin(const std::string& pluginPath);
    void UnloadPlugin(const std::string& pluginName);
    
    // Call plugin function
    std::string CallFunction(const std::string& pluginName, const std::string& funcName, 
                           const std::string& input = "");
    
    // Plugin management
    std::vector<std::string> GetLoadedPlugins() const;
    
private:
    struct PluginModule {
        void* wasmMemory = nullptr;
        uint8_t* memoryPtr = nullptr;
        size_t memorySize = 0;
        plugin_init_fn init = nullptr;
        void* moduleHandle = nullptr;
        IDE_API api;
    };
    
    std::unordered_map<std::string, std::unique_ptr<PluginModule>> m_plugins;
    IDE_API m_api;
};

// Hot-reload support
class PluginManager {
public:
    static PluginManager& Instance() {
        static PluginManager instance;
        return instance;
    }
    
    // Watch plugin directory for changes
    void StartHotReload(const std::string& pluginsDir);
    void StopHotReload();
    
    // Register plugin command
    void RegisterCommand(const std::string& name, 
                        const std::function<void(const std::string&)>& handler);
    
    // Execute plugin command
    bool ExecuteCommand(const std::string& name, const std::string& input = "");
    
private:
    void OnFileChanged(const std::string& path);
    
    WASMRuntime m_runtime;
    std::unordered_map<std::string, std::function<void(const std::string&)>> m_commands;
    std::string m_pluginsDir;
    bool m_watching = false;
};