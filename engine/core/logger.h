// engine/core/logger.h
#pragma once

#include <string>
#include <source_location>
#include <memory>
#include <filesystem>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <fstream>
#include <chrono>
#include <unordered_map>

namespace Engine
{
    namespace Core
    {
        // Níveis de log
        enum class LogLevel
        {
            Trace = 0,
            Debug,
            Info,
            Warn,
            Error,
            Critical
        };

        // Estrutura de uma mensagem de log
        struct LogMessage
        {
            LogLevel level;
            std::string message;
            std::string category;
            std::string file;
            uint32_t line;
            std::chrono::system_clock::time_point timestamp;
        };

        // Configuração do logger
        struct LoggerConfig
        {
            bool enableConsole = true;
            bool enableFile = true;
            bool enableColors = true;
            bool asyncMode = true;
            
            std::filesystem::path logDirectory = "logs";
            std::string logFilePrefix = "game-engine";
            
            // Rotação de arquivos
            size_t maxFileSizeBytes = 10 * 1024 * 1024; // 10MB padrão
            uint32_t maxFileCount = 10; // Manter últimos 10 arquivos
            
            // Níveis por categoria
            LogLevel defaultLevel = LogLevel::Info;
            std::unordered_map<std::string, LogLevel> categoryLevels;
        };

        class Logger
        {
        public:
            static Logger& GetInstance();
            
            // Inicialização/shutdown
            void Initialize(const LoggerConfig& config = LoggerConfig());
            void Shutdown();
            
            // Configuração dinâmica
            void SetLogLevel(LogLevel level);
            void SetCategoryLevel(const std::string& category, LogLevel level);
            void EnableColors(bool enable);
            void EnableConsole(bool enable);
            void EnableFile(bool enable);
            
            // Métodos de log (backward compatible com Log class)
            void Trace(const std::string& message, 
                      const std::string& category = "",
                      const std::source_location& location = std::source_location::current());
            
            void Debug(const std::string& message,
                      const std::string& category = "",
                      const std::source_location& location = std::source_location::current());
            
            void Info(const std::string& message,
                     const std::string& category = "",
                     const std::source_location& location = std::source_location::current());
            
            void Warn(const std::string& message,
                     const std::string& category = "",
                     const std::source_location& location = std::source_location::current());
            
            void Error(const std::string& message,
                      const std::string& category = "",
                      const std::source_location& location = std::source_location::current());
            
            void Critical(const std::string& message,
                         const std::string& category = "",
                         const std::source_location& location = std::source_location::current());

        private:
            Logger() = default;
            ~Logger();
            Logger(const Logger&) = delete;
            Logger& operator=(const Logger&) = delete;

            // Thread worker
            void WorkerThread();
            void ProcessMessage(const LogMessage& msg);
            
            // Saída
            void WriteToConsole(const LogMessage& msg);
            void WriteToFile(const LogMessage& msg);
            
            // Rotação de arquivos
            void RotateLogFile();
            void CleanOldLogFiles();
            std::string GetCurrentLogFileName() const;
            
            // Formatação
            std::string FormatLogMessage(const LogMessage& msg, bool useColors) const;
            std::string GetLevelString(LogLevel level) const;
            std::string GetLevelColor(LogLevel level) const;
            
            // Filtros
            bool ShouldLog(LogLevel level, const std::string& category) const;

            // Estado
            LoggerConfig m_config;
            std::atomic<bool> m_running{false};
            std::atomic<bool> m_initialized{false};
            
            // Thread async
            std::unique_ptr<std::thread> m_workerThread;
            std::queue<LogMessage> m_messageQueue;
            std::mutex m_queueMutex;
            std::condition_variable m_queueCV;
            
            // Arquivo
            std::ofstream m_logFile;
            std::mutex m_fileMutex;
            size_t m_currentFileSize = 0;
            
            // Cores ANSI
            static constexpr const char* RESET = "\033[0m";
            static constexpr const char* GRAY = "\033[90m";
            static constexpr const char* YELLOW = "\033[33m";
            static constexpr const char* RED = "\033[31m";
            static constexpr const char* BRIGHT_RED = "\033[91m";
            static constexpr const char* CYAN = "\033[36m";
            static constexpr const char* WHITE = "\033[37m";
        };

        // Wrapper para compatibilidade com código existente (Log::Info, etc)
        class Log
        {
        public:
            static void SetLogLevel(LogLevel level) { Logger::GetInstance().SetLogLevel(level); }
            static void EnableColors(bool enable) { Logger::GetInstance().EnableColors(enable); }
            static void InitializeColors() { Logger::GetInstance().Initialize(); }
            
            static void Trace(const std::string& message, const std::source_location& location = std::source_location::current()) {
                Logger::GetInstance().Trace(message, "", location);
            }
            static void Debug(const std::string& message, const std::source_location& location = std::source_location::current()) {
                Logger::GetInstance().Debug(message, "", location);
            }
            static void Info(const std::string& message, const std::source_location& location = std::source_location::current()) {
                Logger::GetInstance().Info(message, "", location);
            }
            static void Warn(const std::string& message, const std::source_location& location = std::source_location::current()) {
                Logger::GetInstance().Warn(message, "", location);
            }
            static void Error(const std::string& message, const std::source_location& location = std::source_location::current()) {
                Logger::GetInstance().Error(message, "", location);
            }
            static void Critical(const std::string& message, const std::source_location& location = std::source_location::current()) {
                Logger::GetInstance().Critical(message, "", location);
            }
        };

    } // namespace Core
} // namespace Engine
