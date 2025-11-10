// engine/core/logger.cpp
#include "logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Engine
{
    namespace Core
    {
        Logger& Logger::GetInstance()
        {
            static Logger instance;
            return instance;
        }

        Logger::~Logger()
        {
            Shutdown();
        }

        void Logger::Initialize(const LoggerConfig& config)
        {
            if (m_initialized.load())
            {
                return; // Já inicializado
            }

            m_config = config;

            // Criar diretório de logs se não existir
            if (m_config.enableFile)
            {
                std::filesystem::create_directories(m_config.logDirectory);
                
                // Abrir arquivo de log
                std::string filename = GetCurrentLogFileName();
                m_logFile.open(m_config.logDirectory / filename, std::ios::app);
                
                if (!m_logFile.is_open())
                {
                    std::cerr << "[Logger] ERRO: Não foi possível abrir arquivo de log: " 
                              << filename << std::endl;
                    m_config.enableFile = false;
                }
                else
                {
                    // Log inicial
                    m_logFile << "\n========================================\n";
                    m_logFile << "LOG INICIADO: " << filename << "\n";
                    m_logFile << "========================================\n\n";
                    m_logFile.flush();
                }
            }

            // Habilitar cores ANSI no Windows
            if (m_config.enableColors && m_config.enableConsole)
            {
#ifdef _WIN32
                HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
                if (hOut != INVALID_HANDLE_VALUE)
                {
                    DWORD dwMode = 0;
                    if (GetConsoleMode(hOut, &dwMode))
                    {
                        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                        SetConsoleMode(hOut, dwMode);
                    }
                }
                
                HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
                if (hErr != INVALID_HANDLE_VALUE)
                {
                    DWORD dwMode = 0;
                    if (GetConsoleMode(hErr, &dwMode))
                    {
                        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                        SetConsoleMode(hErr, dwMode);
                    }
                }
#endif
            }

            // Iniciar thread worker se modo async
            if (m_config.asyncMode)
            {
                m_running.store(true);
                m_workerThread = std::make_unique<std::thread>(&Logger::WorkerThread, this);
            }

            m_initialized.store(true);
            
            Info("Logger inicializado", "Logger");
        }

        void Logger::Shutdown()
        {
            if (!m_initialized.load())
            {
                return;
            }

            Info("Finalizando logger...", "Logger");

            // Parar thread worker
            if (m_config.asyncMode && m_running.load())
            {
                m_running.store(false);
                m_queueCV.notify_all();
                
                if (m_workerThread && m_workerThread->joinable())
                {
                    m_workerThread->join();
                }
            }

            // Fechar arquivo
            if (m_logFile.is_open())
            {
                m_logFile << "\n========================================\n";
                m_logFile << "LOG FINALIZADO\n";
                m_logFile << "========================================\n";
                m_logFile.close();
            }

            m_initialized.store(false);
        }

        void Logger::SetLogLevel(LogLevel level)
        {
            m_config.defaultLevel = level;
        }

        void Logger::SetCategoryLevel(const std::string& category, LogLevel level)
        {
            m_config.categoryLevels[category] = level;
        }

        void Logger::EnableColors(bool enable)
        {
            m_config.enableColors = enable;
        }

        void Logger::EnableConsole(bool enable)
        {
            m_config.enableConsole = enable;
        }

        void Logger::EnableFile(bool enable)
        {
            m_config.enableFile = enable;
        }

        void Logger::Trace(const std::string& message, const std::string& category, 
                          const std::source_location& location)
        {
            if (!ShouldLog(LogLevel::Trace, category)) return;

            LogMessage msg{
                LogLevel::Trace,
                message,
                category,
                location.file_name(),
                location.line(),
                std::chrono::system_clock::now()
            };

            if (m_config.asyncMode)
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                m_messageQueue.push(msg);
                m_queueCV.notify_one();
            }
            else
            {
                ProcessMessage(msg);
            }
        }

        void Logger::Debug(const std::string& message, const std::string& category,
                          const std::source_location& location)
        {
            if (!ShouldLog(LogLevel::Debug, category)) return;

            LogMessage msg{
                LogLevel::Debug,
                message,
                category,
                location.file_name(),
                location.line(),
                std::chrono::system_clock::now()
            };

            if (m_config.asyncMode)
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                m_messageQueue.push(msg);
                m_queueCV.notify_one();
            }
            else
            {
                ProcessMessage(msg);
            }
        }

        void Logger::Info(const std::string& message, const std::string& category,
                         const std::source_location& location)
        {
            if (!ShouldLog(LogLevel::Info, category)) return;

            LogMessage msg{
                LogLevel::Info,
                message,
                category,
                location.file_name(),
                location.line(),
                std::chrono::system_clock::now()
            };

            if (m_config.asyncMode)
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                m_messageQueue.push(msg);
                m_queueCV.notify_one();
            }
            else
            {
                ProcessMessage(msg);
            }
        }

        void Logger::Warn(const std::string& message, const std::string& category,
                         const std::source_location& location)
        {
            if (!ShouldLog(LogLevel::Warn, category)) return;

            LogMessage msg{
                LogLevel::Warn,
                message,
                category,
                location.file_name(),
                location.line(),
                std::chrono::system_clock::now()
            };

            if (m_config.asyncMode)
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                m_messageQueue.push(msg);
                m_queueCV.notify_one();
            }
            else
            {
                ProcessMessage(msg);
            }
        }

        void Logger::Error(const std::string& message, const std::string& category,
                          const std::source_location& location)
        {
            if (!ShouldLog(LogLevel::Error, category)) return;

            LogMessage msg{
                LogLevel::Error,
                message,
                category,
                location.file_name(),
                location.line(),
                std::chrono::system_clock::now()
            };

            if (m_config.asyncMode)
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                m_messageQueue.push(msg);
                m_queueCV.notify_one();
            }
            else
            {
                ProcessMessage(msg);
            }
        }

        void Logger::Critical(const std::string& message, const std::string& category,
                             const std::source_location& location)
        {
            if (!ShouldLog(LogLevel::Critical, category)) return;

            LogMessage msg{
                LogLevel::Critical,
                message,
                category,
                location.file_name(),
                location.line(),
                std::chrono::system_clock::now()
            };

            if (m_config.asyncMode)
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                m_messageQueue.push(msg);
                m_queueCV.notify_one();
            }
            else
            {
                ProcessMessage(msg);
            }
        }

        void Logger::WorkerThread()
        {
            while (m_running.load())
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_queueCV.wait(lock, [this] { 
                    return !m_messageQueue.empty() || !m_running.load(); 
                });

                while (!m_messageQueue.empty())
                {
                    LogMessage msg = m_messageQueue.front();
                    m_messageQueue.pop();
                    lock.unlock();

                    ProcessMessage(msg);

                    lock.lock();
                }
            }

            // Processar mensagens restantes
            std::lock_guard<std::mutex> lock(m_queueMutex);
            while (!m_messageQueue.empty())
            {
                ProcessMessage(m_messageQueue.front());
                m_messageQueue.pop();
            }
        }

        void Logger::ProcessMessage(const LogMessage& msg)
        {
            if (m_config.enableConsole)
            {
                WriteToConsole(msg);
            }

            if (m_config.enableFile)
            {
                WriteToFile(msg);
            }
        }

        void Logger::WriteToConsole(const LogMessage& msg)
        {
            std::string formatted = FormatLogMessage(msg, m_config.enableColors);
            
            if (msg.level >= LogLevel::Warn)
            {
                std::cerr << formatted;
            }
            else
            {
                std::cout << formatted;
            }
        }

        void Logger::WriteToFile(const LogMessage& msg)
        {
            std::lock_guard<std::mutex> lock(m_fileMutex);

            if (!m_logFile.is_open())
            {
                return;
            }

            // Escrever mensagem (sem cores no arquivo)
            std::string formatted = FormatLogMessage(msg, false);
            m_logFile << formatted;
            m_logFile.flush();

            // Atualizar tamanho do arquivo
            m_currentFileSize += formatted.size();

            // Rotacionar se necessário
            if (m_currentFileSize >= m_config.maxFileSizeBytes)
            {
                RotateLogFile();
            }
        }

        void Logger::RotateLogFile()
        {
            if (!m_logFile.is_open())
            {
                return;
            }

            // Fechar arquivo atual
            m_logFile.close();

            // Limpar arquivos antigos
            CleanOldLogFiles();

            // Abrir novo arquivo
            std::string filename = GetCurrentLogFileName();
            m_logFile.open(m_config.logDirectory / filename, std::ios::app);
            m_currentFileSize = 0;

            if (!m_logFile.is_open())
            {
                std::cerr << "[Logger] ERRO: Não foi possível rotacionar para: " 
                          << filename << std::endl;
                m_config.enableFile = false;
            }
        }

        void Logger::CleanOldLogFiles()
        {
            namespace fs = std::filesystem;

            std::vector<fs::path> logFiles;
            
            // Listar arquivos de log
            for (const auto& entry : fs::directory_iterator(m_config.logDirectory))
            {
                if (entry.is_regular_file())
                {
                    std::string filename = entry.path().filename().string();
                    if (filename.find(m_config.logFilePrefix) == 0 && 
                        filename.find(".log") != std::string::npos)
                    {
                        logFiles.push_back(entry.path());
                    }
                }
            }

            // Ordenar por tempo de modificação (mais recente primeiro)
            std::sort(logFiles.begin(), logFiles.end(), [](const fs::path& a, const fs::path& b) {
                return fs::last_write_time(a) > fs::last_write_time(b);
            });

            // Deletar arquivos excedentes
            for (size_t i = m_config.maxFileCount; i < logFiles.size(); ++i)
            {
                try
                {
                    fs::remove(logFiles[i]);
                }
                catch (const std::exception& e)
                {
                    std::cerr << "[Logger] Erro ao deletar arquivo antigo: " 
                              << e.what() << std::endl;
                }
            }
        }

        std::string Logger::GetCurrentLogFileName() const
        {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::tm tm_buf;

#ifdef _WIN32
            localtime_s(&tm_buf, &time);
#else
            localtime_r(&time, &tm_buf);
#endif

            std::ostringstream ss;
            ss << m_config.logFilePrefix << "-"
               << std::put_time(&tm_buf, "%Y-%m-%d_%H-%M-%S")
               << ".log";
            
            return ss.str();
        }

        std::string Logger::FormatLogMessage(const LogMessage& msg, bool useColors) const
        {
            // Timestamp
            auto time = std::chrono::system_clock::to_time_t(msg.timestamp);
            std::tm tm_buf;

#ifdef _WIN32
            localtime_s(&tm_buf, &time);
#else
            localtime_r(&time, &tm_buf);
#endif

            std::ostringstream ss;
            ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");

            // Cor
            std::string color = useColors ? GetLevelColor(msg.level) : "";
            std::string reset = useColors ? RESET : "";

            // Categoria
            std::string categoryStr = msg.category.empty() ? "" : "[" + msg.category + "] ";

            // Extrair apenas o nome do arquivo (sem path completo)
            std::filesystem::path filepath(msg.file);
            std::string filename = filepath.filename().string();

            // Formato: [TIMESTAMP] [NIVEL] [CATEGORIA] (arquivo:linha) mensagem
            std::ostringstream result;
            result << color
                   << "[" << ss.str() << "] "
                   << "[" << GetLevelString(msg.level) << "] "
                   << categoryStr
                   << "(" << filename << ":" << msg.line << ") "
                   << msg.message
                   << reset << "\n";

            return result.str();
        }

        std::string Logger::GetLevelString(LogLevel level) const
        {
            switch (level)
            {
                case LogLevel::Trace:    return "TRACE";
                case LogLevel::Debug:    return "DEBUG";
                case LogLevel::Info:     return "INFO ";
                case LogLevel::Warn:     return "WARN ";
                case LogLevel::Error:    return "ERROR";
                case LogLevel::Critical: return "CRITICAL";
                default:                 return "UNKNOWN";
            }
        }

        std::string Logger::GetLevelColor(LogLevel level) const
        {
            switch (level)
            {
                case LogLevel::Trace:    return "";           // Sem cor
                case LogLevel::Debug:    return "";           // Sem cor
                case LogLevel::Info:     return GRAY;         // Cinza
                case LogLevel::Warn:     return YELLOW;       // Amarelo
                case LogLevel::Error:    return RED;          // Vermelho
                case LogLevel::Critical: return BRIGHT_RED;   // Vermelho brilhante
                default:                 return "";
            }
        }

        bool Logger::ShouldLog(LogLevel level, const std::string& category) const
        {
            // Verificar nível específico da categoria
            if (!category.empty())
            {
                auto it = m_config.categoryLevels.find(category);
                if (it != m_config.categoryLevels.end())
                {
                    return static_cast<int>(level) >= static_cast<int>(it->second);
                }
            }

            // Usar nível padrão
            return static_cast<int>(level) >= static_cast<int>(m_config.defaultLevel);
        }

    } // namespace Core
} // namespace Engine
