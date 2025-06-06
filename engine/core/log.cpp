// engine/core/log.cpp
#include "log.h"
#include <chrono>   // Para std::chrono::system_clock
#include <iomanip>  // Para std::put_time
#include <sstream>  // Para std::stringstream

namespace Engine {

// Inicializa o nível de log padrão (pode ser configurado em tempo de execução)
LogLevel Log::s_currentLogLevel = LogLevel::Info; // Nível padrão: apenas Info, Warn, Error, Critical

void Log::SetLogLevel(LogLevel level) {
    s_currentLogLevel = level;
    // Opcional: Log::Info(std::format("Nível de log definido para {}", GetLogLevelString(level)));
}

void Log::Trace(const std::string& message, const std::source_location& location) {
    LogMessage(LogLevel::Trace, message, location);
}

void Log::Debug(const std::string& message, const std::source_location& location) {
    LogMessage(LogLevel::Debug, message, location);
}

void Log::Info(const std::string& message, const std::source_location& location) {
    LogMessage(LogLevel::Info, message, location);
}

void Log::Warn(const std::string& message, const std::source_location& location) {
    LogMessage(LogLevel::Warn, message, location);
}

void Log::Error(const std::string& message, const std::source_location& location) {
    LogMessage(LogLevel::Error, message, location);
}

void Log::Critical(const std::string& message, const std::source_location& location) {
    LogMessage(LogLevel::Critical, message, location);
}

void Log::LogMessage(LogLevel level, const std::string& message, const std::source_location& location) {
    if (static_cast<int>(level) < static_cast<int>(s_currentLogLevel)) {
        return; // Ignora mensagens abaixo do nível configurado
    }

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
    
#ifdef _WIN32
    localtime_s(&tm_buf, &in_time_t); // Thread-safe version for Windows
#else
    localtime_r(&in_time_t, &tm_buf); // Thread-safe version for Unix-like
#endif

    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");

    // Formato da mensagem: [TIMESTAMP] [NIVEL] (Arquivo:Linha:Função) Mensagem
    std::string formatted_message = std::format("[{}] [{}] ({}:{}) {}\n",
                                                ss.str(),
                                                GetLogLevelString(level),
                                                location.file_name(),
                                                location.line(),
                                                message);

    // Saída no console. Em um sistema mais robusto, seria redirecionado para um arquivo, etc.
    if (level >= LogLevel::Warn) {
        std::cerr << formatted_message; // Erros e Warnings para std::cerr
    } else {
        std::cout << formatted_message; // Outros para std::cout
    }
}

std::string Log::GetLogLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:    return "TRACE";
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO "; // Adicionado espaço para alinhamento
        case LogLevel::Warn:     return "WARN ";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

} // namespace Engine