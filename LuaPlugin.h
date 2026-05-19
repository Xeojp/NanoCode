#pragma once
#include <string>
#include <functional>
#include <unordered_map>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

class LuaPluginAPI {
public:
    LuaPluginAPI() : L(luaL_newstate()) {
        luaL_openlibs(L);
        RegisterFunctions();
    }
    
    ~LuaPluginAPI() {
        if (L) lua_close(L);
    }
    
    // Register IDE functions to Lua
    void RegisterFunctions() {
        lua_register(L, "ide_log", [](lua_State* L) -> int {
            const char* msg = lua_tostring(L, 1);
            OutputDebugStringA(msg);
            return 0;
        });
        
        lua_register(L, "ide_get_active_file", [](lua_State* L) -> int {
            // Get from IDE state
            lua_pushstring(L, "");
            return 1;
        });
        
        lua_register(L, "ide_insert_text", [](lua_State* L) -> int {
            const char* text = lua_tostring(L, 1);
            // Insert to active document
            return 0;
        });
    }
    
    // Load and run plugin
    bool LoadPlugin(const std::string& path) {
        if (luaL_dofile(L, path.c_str()) != 0) {
            const char* err = lua_tostring(L, -1);
            OutputDebugStringA(err);
            return false;
        }
        return true;
    }
    
    // Call plugin function
    bool Call(const std::string& funcName, const std::string& arg = "") {
        lua_getglobal(L, funcName.c_str());
        if (!lua_isfunction(L, -1)) {
            lua_pop(L, 1);
            return false;
        }
        
        lua_pushstring(L, arg.c_str());
        return lua_pcall(L, 1, 0, 0) == 0;
    }
    
private:
    lua_State* L;
};

// Plugin manager with hot-reload
class PluginHost {
public:
    // Each plugin runs in isolated coroutine for async execution
    struct Plugin {
        LuaPluginAPI vm;
        std::string name;
        std::string path;
        std::vector<std::string> commands;
    };
    
    void LoadAll(const std::string& dir) {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.path().extension() == ".lua") {
                LoadPlugin(entry.path().string());
            }
        }
    }
    
    void LoadPlugin(const std::string& path) {
        auto plugin = std::make_unique<Plugin>();
        plugin->path = path;
        plugin->name = path.substr(path.find_last_of("/\\") + 1);
        plugin->name = plugin->name.substr(0, plugin->name.size() - 4);
        
        if (plugin->vm.LoadPlugin(path)) {
            m_plugins[plugin->name] = std::move(plugin);
        }
    }
    
    void Reload(const std::string& name) {
        auto it = m_plugins.find(name);
        if (it != m_plugins.end()) {
            LoadPlugin(it->second->path);
        }
    }
    
    bool Execute(const std::string& plugin, const std::string& func) {
        auto it = m_plugins.find(plugin);
        if (it != m_plugins.end()) {
            return it->second->vm.Call(func);
        }
        return false;
    }
    
private:
    std::unordered_map<std::string, std::unique_ptr<Plugin>> m_plugins;
};