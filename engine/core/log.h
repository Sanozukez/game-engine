// engine/core/log.h
#pragma once

#include <string>
#include <iostream> // Para saída no console por enquanto
#include <source_location> // C++20, para obter informações de arquivo/linha
#include <chrono>    // Para timestamps
#include <format>    // C++20, para formatação de strings

namespace Engine {

// Níveis de log
enum class LogLevel {
    Trace = 0,   // Detalhes muito finos, para depuração extensiva
    Debug,       // Informações de debug, úteis durante o desenvolvimento
    Info,        // Mensagens informativas sobre o progresso normal da aplicação
    Warn,        // Avisos sobre situações não ideais, mas não críticas
    Error,       // Erros que podem causar problemas, mas o programa continua
    Critical     // Erros graves que levam à terminação do programa
};

class Log {
public:
    // Define o nível mínimo de log a ser exibido.
    // Mensagens com um nível menor que o atual serão ignoradas.
    static void SetLogLevel(LogLevel level);

    // Métodos de log para cada nível
    static void Trace(const std::string& message, 
                      const std::source_location& location = std::source_location::current());
    static void Debug(const std::string& message,
                       const std::source_location& location = std::source_location::current());
    static void Info(const std::string& message,
                      const std::source_location& location = std::source_location::current());
    static void Warn(const std::string& message,
                     const std::source_location& location = std::source_location::current());
    static void Error(const std::string& message,
                       const std::source_location& location = std::source_location::current());
    static void Critical(const std::string& message,
                        const std::source_location& location = std::source_location::current());

private:
    static LogLevel s_currentLogLevel; // Nível de log atual
    
    // Função auxiliar para formatar e imprimir a mensagem
    static void LogMessage(LogLevel level, const std::string& message, const std::source_location& location);
    static std::string GetLogLevelString(LogLevel level);
};

} // namespace Engine