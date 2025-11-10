// tests/test_logger.cpp
// Teste do novo sistema de logging

#include "../engine/core/logger.h"
#include <thread>
#include <chrono>

using namespace Engine::Core;

void TestBasicLogging()
{
    Log::Info("=== TESTE 1: Logging Básico (backward compatible) ===");
    
    Log::Trace("Mensagem de trace - muito detalhada");
    Log::Debug("Mensagem de debug");
    Log::Info("Mensagem informativa");
    Log::Warn("Atenção: situação não ideal");
    Log::Error("Erro recuperável");
    Log::Critical("ERRO CRÍTICO!");
}

void TestCategoryLogging()
{
    Log::Info("=== TESTE 2: Logging com Categorias ===");
    
    Logger::GetInstance().Info("Jogador conectou-se", "Network");
    Logger::GetInstance().Info("Animação 'walk' iniciada", "Animation");
    Logger::GetInstance().Warn("Frame rate baixo detectado", "Performance");
    Logger::GetInstance().Error("Shader falhou ao compilar", "Render");
    Logger::GetInstance().Info("Item adicionado ao inventário", "Gameplay");
}

void TestCategoryFilters()
{
    Log::Info("=== TESTE 3: Filtros por Categoria ===");
    
    // Desabilitar logs Debug de Render (muito spam)
    Logger::GetInstance().SetCategoryLevel("Render", LogLevel::Warn);
    
    Logger::GetInstance().Debug("Este log NÃO aparece", "Render");
    Logger::GetInstance().Warn("Este log APARECE", "Render");
    Logger::GetInstance().Debug("Este log APARECE (sem categoria)");
    
    // Habilitar logs Trace apenas para Network
    Logger::GetInstance().SetCategoryLevel("Network", LogLevel::Trace);
    Logger::GetInstance().Trace("Packet recebido: 0x4A2F", "Network");
}

void TestThreadSafety()
{
    Log::Info("=== TESTE 4: Thread Safety (Async) ===");
    
    auto worker = [](int id) {
        for (int i = 0; i < 10; ++i) {
            Logger::GetInstance().Info(
                std::format("Thread {} - Mensagem {}", id, i),
                "Thread"
            );
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };
    
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    std::thread t3(worker, 3);
    
    t1.join();
    t2.join();
    t3.join();
    
    Log::Info("Threads finalizadas - nenhuma mensagem deve estar misturada!");
}

void TestFileOutput()
{
    Log::Info("=== TESTE 5: Saída para Arquivo ===");
    Log::Info("Verifique o diretório 'logs/' - deve ter um arquivo game-engine-*.log");
    Log::Info("O arquivo contém as mesmas mensagens do console (sem cores)");
    
    // Gerar algumas mensagens para testar rotação
    for (int i = 0; i < 50; ++i) {
        Logger::GetInstance().Info(
            std::format("Mensagem de teste para rotação de arquivo #{}", i),
            "FileTest"
        );
    }
}

int main()
{
    // Configurar logger
    LoggerConfig config;
    config.enableConsole = true;
    config.enableFile = true;
    config.enableColors = true;
    config.asyncMode = true;
    config.logDirectory = "logs";
    config.defaultLevel = LogLevel::Trace; // Mostrar tudo nos testes
    
    Logger::GetInstance().Initialize(config);
    
    Log::Info("╔══════════════════════════════════════════════════════╗");
    Log::Info("║   TESTE DO NOVO SISTEMA DE LOGGING                  ║");
    Log::Info("╚══════════════════════════════════════════════════════╝");
    
    TestBasicLogging();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    TestCategoryLogging();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    TestCategoryFilters();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    TestThreadSafety();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    TestFileOutput();
    
    Log::Info("╔══════════════════════════════════════════════════════╗");
    Log::Info("║   TESTES CONCLUÍDOS                                  ║");
    Log::Info("╚══════════════════════════════════════════════════════╝");
    
    // Shutdown (flush logs pendentes)
    Logger::GetInstance().Shutdown();
    
    return 0;
}
