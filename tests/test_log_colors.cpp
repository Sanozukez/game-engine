// test_log_colors.cpp
// Pequeno teste para verificar cores do sistema de log

#include "../engine/core/log.h"
#include <thread>
#include <chrono>

int main() {
    // Inicializar cores
    Engine::Core::Log::InitializeColors();
    
    Engine::Core::Log::Info("=== TESTE DE CORES DO SISTEMA DE LOG ===");
    Engine::Core::Log::Info("");
    
    // Testar cada nível
    Engine::Core::Log::Trace("Trace: Sem cor (padrão do terminal)");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    Engine::Core::Log::Debug("Debug: Sem cor (padrão do terminal)");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    Engine::Core::Log::Info("Info: Cinza claro");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    Engine::Core::Log::Warn("Warn: Amarelo");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    Engine::Core::Log::Error("Error: Vermelho");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    Engine::Core::Log::Critical("Critical: Vermelho brilhante");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    Engine::Core::Log::Info("");
    Engine::Core::Log::Info("=== FIM DO TESTE ===");
    
    return 0;
}
