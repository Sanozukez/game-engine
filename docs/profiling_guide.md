# Sistema de Profiling - Guia de Uso

## 📊 Overview

Sistema de profiling leve e fácil de usar para monitorar performance da engine.

**Características**:
- ✅ Zero overhead em Release builds (desabilitado automaticamente)
- ✅ API simples e intuitiva
- ✅ Três modos de uso: Scoped, Manual, Frame Timing
- ✅ Logs automáticos em unidades apropriadas (μs, ms, s)

---

## 🚀 Quick Start

### 1. Profiling Automático (Scoped Timer)

Mede tempo de execução de um bloco de código automaticamente:

```cpp
#include "engine/core/profiler.h"

void MyFunction() {
    PROFILE_SCOPE("MyFunction");  // <-- Adicione esta linha!
    
    // Seu código aqui
    for (int i = 0; i < 1000; i++) {
        // Processamento pesado
    }
    
} // Automaticamente loga tempo ao sair do escopo

// Output: [PROFILE] MyFunction took 1.23ms
```

### 2. Profiling de Função Inteira

Macro conveniente para profiling de função completa:

```cpp
void AnimationSystem::update(World& world, float dt) {
    PROFILE_FUNCTION();  // <-- Usa nome da função automaticamente!
    
    // Lógica da função
}

// Output: [PROFILE] AnimationSystem::update took 2.45ms
```

### 3. Profiling Manual

Controle total sobre start/stop:

```cpp
#include "engine/core/profiler.h"

Engine::Core::ManualTimer timer;

timer.start();
// Código a medir
doExpensiveOperation();
timer.stop();

float ms = timer.elapsedMilliseconds();
Log::Info(std::format("Operation took {:.2f}ms", ms));

// Reusar o timer
timer.reset();
timer.start();
// Outra operação
timer.stop();
```

### 4. Frame Timing (FPS Monitoring)

Monitoramento automático de FPS e frame times:

```cpp
#include "engine/core/profiler.h"

Engine::Core::FrameTimer frameTimer(1.0f); // Log stats a cada 1 segundo

while (gameRunning) {
    frameTimer.beginFrame();
    
    // Game loop
    updateGame(deltaTime);
    renderGame();
    
    frameTimer.endFrame();
    
    if (frameTimer.shouldLogStats()) {
        frameTimer.logStats();
    }
}

// Output (a cada 1 segundo):
// [FRAME] FPS: 60.2 | Avg: 16.61ms | Min: 15.02ms | Max: 18.45ms | Frames: 60
```

---

## 📋 Exemplos Práticos

### Exemplo 1: Profiling de Sistema ECS

```cpp
// engine/ecs/systems/animation_system.cpp

#include "../../core/profiler.h"

void AnimationSystem::update(World& world, float dt) {
    PROFILE_SCOPE("AnimationSystem::update");
    
    for (EntityID entity : m_entities) {
        // Profiling interno (nested)
        {
            PROFILE_SCOPE("AnimationSystem::ProcessEntity");
            
            // Keyframe sampling
            {
                PROFILE_SCOPE("KeyframeSampling");
                sampleKeyframes(entity, dt);
            }
            
            // Forward Kinematics
            {
                PROFILE_SCOPE("ForwardKinematics");
                computeBoneTransforms(entity);
            }
        }
    }
}

// Output:
// [PROFILE] KeyframeSampling took 150μs
// [PROFILE] ForwardKinematics took 280μs
// [PROFILE] AnimationSystem::ProcessEntity took 450μs
// [PROFILE] AnimationSystem::update took 2.15ms
```

### Exemplo 2: Profiling de Asset Loading

```cpp
// engine/asset/gltf_loader.cpp

std::shared_ptr<Model> GLTFLoader::loadGLTF(const std::string& path) {
    PROFILE_FUNCTION();
    
    cgltf_data* data = nullptr;
    
    {
        PROFILE_SCOPE("cgltf_parse_file");
        cgltf_parse_file(path.c_str(), &options, &data);
    }
    
    {
        PROFILE_SCOPE("cgltf_load_buffers");
        cgltf_load_buffers(&options, data, path.c_str());
    }
    
    {
        PROFILE_SCOPE("ProcessMeshes");
        processMeshes(data);
    }
    
    {
        PROFILE_SCOPE("ProcessAnimations");
        processAnimations(data);
    }
    
    cgltf_free(data);
    return model;
}

// Output:
// [PROFILE] cgltf_parse_file took 3.45ms
// [PROFILE] cgltf_load_buffers took 8.23ms
// [PROFILE] ProcessMeshes took 12.67ms
// [PROFILE] ProcessAnimations took 5.89ms
// [PROFILE] GLTFLoader::loadGLTF took 30.24ms
```

### Exemplo 3: Comparar Performance de Algoritmos

```cpp
void benchmarkAlgorithms() {
    Engine::Core::ManualTimer timer;
    
    // Algoritmo 1
    timer.start();
    for (int i = 0; i < 10000; i++) {
        algorithmA(i);
    }
    timer.stop();
    float timeA = timer.elapsedMilliseconds();
    
    // Algoritmo 2
    timer.reset();
    timer.start();
    for (int i = 0; i < 10000; i++) {
        algorithmB(i);
    }
    timer.stop();
    float timeB = timer.elapsedMilliseconds();
    
    Log::Info(std::format("Algorithm A: {:.2f}ms", timeA));
    Log::Info(std::format("Algorithm B: {:.2f}ms", timeB));
    Log::Info(std::format("Speedup: {:.2f}x", timeA / timeB));
}

// Output:
// Algorithm A: 45.67ms
// Algorithm B: 12.34ms
// Speedup: 3.70x
```

---

## ⚙️ Configuração

### Habilitar/Desabilitar Profiling

Profiling é **automático** baseado no build type:

```cmake
# CMakeLists.txt

# Debug → ENABLE_PROFILING definido (profiling ativo)
# Release → ENABLE_PROFILING NÃO definido (zero overhead)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(ENABLE_PROFILING)
endif()
```

### Forçar Habilitação em Release

Se quiser profiling em Release build (para testes finais):

```cmake
# CMakeLists.txt
add_compile_definitions(ENABLE_PROFILING)  # Sempre ativo
```

Ou compile com:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

---

## 📈 Interpretando Resultados

### FPS Target (60 FPS)

```
60 FPS = 16.67ms por frame

Budget típico:
├─ Input:       1ms
├─ Physics:     2ms
├─ Animation:   3ms
├─ AI:          2ms
├─ Rendering:   6ms
├─ UI:          1ms
└─ Outros:      1.67ms
    TOTAL:      16.67ms ✅
```

### Sinais de Alerta

```
❌ FPS abaixo de 60 (frame time > 16.67ms)
   → Otimizar sistemas mais lentos

❌ Max frame time >> Avg frame time
   → Spikes de performance (memory allocation, asset loading)

❌ Sistema individual > 5ms
   → Candidato para otimização urgente

❌ Variação grande entre frames
   → Instabilidade (garbage collection, I/O síncrono)
```

### Otimização Por Prioridade

```
1. Sistema > 10ms → CRÍTICO (otimizar imediatamente)
2. Sistema > 5ms  → ALTO (otimizar em breve)
3. Sistema > 2ms  → MÉDIO (monitorar)
4. Sistema < 2ms  → OK (nenhuma ação necessária)
```

---

## 🎯 Boas Práticas

### ✅ DO:

```cpp
// Profile sistemas grandes
void AnimationSystem::update() {
    PROFILE_FUNCTION();  // ✅
}

// Profile loops importantes
for (auto& entity : entities) {
    PROFILE_SCOPE("ProcessEntity");  // ✅
}

// Use frame timer no game loop
while (running) {
    frameTimer.beginFrame();
    // ...
    frameTimer.endFrame();  // ✅
}

// Profile operações de I/O
void loadLevel() {
    PROFILE_SCOPE("LoadLevel");  // ✅
}
```

### ❌ DON'T:

```cpp
// Não profile dentro de loops ultra-rápidos (overhead!)
for (int i = 0; i < 1000000; i++) {
    PROFILE_SCOPE("TinyOperation");  // ❌ Overhead > operação!
}

// Não profile getters simples
int getX() {
    PROFILE_FUNCTION();  // ❌ Desnecessário
    return x;
}

// Não deixe profiling ativo em shipping builds
// (já desabilitado automaticamente em Release)
```

---

## 🔧 API Completa

### ScopedTimer

```cpp
Engine::Core::ScopedTimer timer("Name");
// Automaticamente loga ao destruir
```

### ManualTimer

```cpp
Engine::Core::ManualTimer timer;

timer.start();              // Inicia cronômetro
timer.stop();               // Para cronômetro
timer.reset();              // Reseta para 0

int64_t us = timer.elapsedMicroseconds();
float ms = timer.elapsedMilliseconds();
float s = timer.elapsedSeconds();

bool running = timer.isRunning();
```

### FrameTimer

```cpp
Engine::Core::FrameTimer frameTimer(1.0f);  // Log interval (seconds)

frameTimer.beginFrame();    // Início do frame
frameTimer.endFrame();      // Fim do frame

bool shouldLog = frameTimer.shouldLogStats();
frameTimer.logStats();      // Log FPS/times

float lastFrame = frameTimer.getLastFrameTime();   // ms
float fps = frameTimer.getCurrentFPS();
```

---

## 💡 Tips & Tricks

### 1. Profiling Condicional

```cpp
#ifdef ENABLE_DETAILED_PROFILING
    PROFILE_SCOPE("DetailedOperation");
#endif
```

### 2. Profiling com Variáveis Dinâmicas

```cpp
for (int i = 0; i < systems.size(); i++) {
    std::string name = std::format("System_{}", i);
    PROFILE_SCOPE(name);
    systems[i].update();
}
```

### 3. Coletar Estatísticas Customizadas

```cpp
Engine::Core::ManualTimer timer;
std::vector<float> samples;

for (int frame = 0; frame < 100; frame++) {
    timer.start();
    doWork();
    timer.stop();
    
    samples.push_back(timer.elapsedMilliseconds());
    timer.reset();
}

// Calcular média, min, max, desvio padrão, etc
float avg = std::accumulate(samples.begin(), samples.end(), 0.0f) / samples.size();
```

---

## 📊 Exemplo de Output Completo

```
[PROFILE] KeyframeSampling took 125μs
[PROFILE] ForwardKinematics took 267μs
[PROFILE] AnimationSystem::update took 2.34ms
[PROFILE] PlayerSystem::update took 0.58ms
[PROFILE] TerrainTrackingSystem::update took 0.12ms
[PROFILE] RenderSystem::update took 4.89ms
[PROFILE] World::update took 8.15ms
[FRAME] FPS: 60.3 | Avg: 16.58ms | Min: 15.23ms | Max: 18.91ms | Frames: 60
```

---

## 🚀 Próximos Passos

### Profiling Avançado (Futuro):

1. **GPU Profiling**: Medir tempo de shaders (via query objects)
2. **Memory Profiling**: Track allocations/deallocations
3. **Visual Profiler**: Gráficos em tempo real (ImGui)
4. **Export to File**: Salvar dados para análise offline
5. **Tracy Integration**: Profiler visual completo

---

**Status**: ✅ Implementado e funcionando!  
**Overhead**: ~1-2μs por PROFILE_SCOPE (negligível)  
**Build Type**: Automático (Debug: ON, Release: OFF)
