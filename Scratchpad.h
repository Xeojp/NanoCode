#pragma once
#include <string>
#include <functional>
#include <memory>
#include <thread>

enum class ScratchLanguage { JavaScript, Python, CSharp };

struct ConsoleOutput {
    enum Type { StdOut, StdErr, Info } type;
    std::string text;
};

class Scratchpad {
public:
    struct Config {
        ScratchLanguage language = ScratchLanguage::Python;
        bool autoRun = false;
        int timeoutMs = 10000; // 10 seconds
    };
    
    using OutputCallback = std::function<void(const ConsoleOutput&)>;
    
    void SetCode(const std::string& code) { m_code = code; }
    void SetLanguage(ScratchLanguage lang) { m_config.language = lang; }
    void SetOutputCallback(OutputCallback cb) { m_onOutput = cb; }
    
    // Запуск кода
    bool Run() {
        if (m_running) return false;
        
        m_running = true;
        m_thread = std::thread([this]() {
            ExecuteCode();
        });
        return true;
    }
    
    void Stop() {
        if (m_process) {
            TerminateProcess(m_process, 0);
        }
        m_running = false;
        if (m_thread.joinable()) m_thread.join();
    }
    
    bool IsRunning() const { return m_running; }
    
private:
    void ExecuteCode() {
        Output("Running...\n", ConsoleOutput::Info);
        
        std::string tempFile;
        std::string cmd;
        
        switch (m_config.language) {
        case ScratchLanguage::Python:
            tempFile = CreateTempFile(m_code, ".py");
            cmd = "python \"" + tempFile + "\"";
            break;
        case ScratchLanguage::JavaScript:
            cmd = "node -e \"" + EscapeForCmd(m_code) + "\"";
            break;
        case ScratchLanguage::CSharp: {
            std::string csFile = CreateTempFile(m_code, ".cs");
            Output("Compiling C#...\n", ConsoleOutput::Info);
            std::string dll = csFile.substr(0, csFile.size() - 3) + ".dll";
            std::string cscCmd = "csc /nologo /out:\"" + dll + "\" \"" + csFile + "\" 2>&1";
            
            auto result = RunProcess(cscCmd);
            if (result.exitCode != 0) {
                Output("Compilation error:\n" + result.stderr + "\n", ConsoleOutput::StdErr);
                m_running = false;
                return;
            }
            cmd = "dotnet \"" + dll + "\"";
            break;
        }
        }
        
        RunProcessWithOutput(cmd);
        m_running = false;
    }
    
    struct ProcessResult {
        int exitCode = 0;
        std::string stdout;
        std::string stderr;
    };
    
    ProcessResult RunProcess(const std::string& cmd) {
        // Simplified - реальная реализация через CreateProcess
        return {};
    }
    
    void RunProcessWithOutput(const std::string& cmd) {
        SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
        
        // Create pipes для stdout/stderr
        HANDLE hOutRead, hOutWrite, hErrRead, hErrWrite;
        CreatePipe(&hOutRead, &hOutWrite, &sa, 0);
        CreatePipe(&hErrRead, &hErrWrite, &sa, 0);
        
        SetHandleInformation(hOutRead, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(hErrRead, HANDLE_FLAG_INHERIT, 0);
        
        STARTUPINFO si = { sizeof(si) };
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hOutWrite;
        si.hStdError = hErrWrite;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        
        PROCESS_INFORMATION pi = {};
        if (CreateProcess(nullptr, const_cast<char*>(cmd.c_str()), 
                         nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
            
            CloseHandle(hOutWrite);
            CloseHandle(hErrWrite);
            
            // Читаем вывод
            char buffer[4096];
            DWORD bytesRead;
            
            while (WaitForSingleObject(pi.hProcess, 10) == WAIT_TIMEOUT) {
                while (ReadFile(hOutRead, buffer, sizeof(buffer)-1, &bytesRead, nullptr) && bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    Output(std::string(buffer), ConsoleOutput::StdOut);
                }
                while (ReadFile(hErrRead, buffer, sizeof(buffer)-1, &bytesRead, nullptr) && bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    Output(std::string(buffer), ConsoleOutput::StdErr);
                }
            }
            
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        
        CloseHandle(hOutRead);
        CloseHandle(hErrRead);
    }
    
    std::string CreateTempFile(const std::string& content, const std::string& ext) {
        char tempPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tempPath);
        
        char tempFile[MAX_PATH];
        GetTempFileNameA(tempPath, "scratch", 0, tempFile);
        
        std::string fullPath = std::string(tempFile) + ext;
        
        HANDLE hFile = CreateFileA(fullPath.c_str(), GENERIC_WRITE, 0, nullptr, 
                                   CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD written;
            WriteFile(hFile, content.c_str(), static_cast<DWORD>(content.size()), &written, nullptr);
            CloseHandle(hFile);
        }
        
        return fullPath;
    }
    
    std::string EscapeForCmd(const std::string& s) {
        std::string result;
        for (char c : s) {
            if (c == '"') result += "\\\"";
            else if (c == '\\') result += "\\\\";
            else result += c;
        }
        return result;
    }
    
    void Output(const std::string& text, ConsoleOutput::Type type) {
        if (m_onOutput) m_onOutput({type, text});
    }
    
    Config m_config;
    std::string m_code;
    std::thread m_thread;
    bool m_running = false;
    HANDLE m_process = nullptr;
    OutputCallback m_onOutput;
};