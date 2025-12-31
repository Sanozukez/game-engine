# 🏗️ Plano de Refatoração Arquitetural - Game Engine

## 📋 **Objetivo**

Separar a engine em módulos **Core (compartilhado)** e **Client (gráficos)** sem perder NENHUMA funcionalidade, especialmente o sistema de animação que funciona perfeitamente.

---

## ⚠️ **ARQUIVOS CRÍTICOS - NÃO TOCAR ATÉ FASE FINAL**

Estes arquivos contêm o **workaround de coordenadas X=180°** que resolve o colapso de bones:

```
🔴 CRÍTICO - NÃO MEXER:
├── engine/animation/animation_data_mapper.cpp
│   └── Linhas 169 (IBM) e 341 (keyframes) - Rotação X=180°
├── src/app/app_setup.cpp
│   └── Linhas 143-154 - Scale Y=-1 + Position correction
├── engine/ecs/systems/player_system.cpp
│   └── Linhas 136-163 - Controles WASD/QE invertidos (Z-axis)
├── engine/ecs/systems/terrain_tracking_system.cpp
│   └── Linhas 108-112 - Height compensation para Scale Y=-1
└── engine/camera/orbit_camera.cpp
    └── Linha 89 - Right-Handed camera fix
```

**Documentação completa**: `docs/animation_coordinate_system_workaround.md`

---

## 📦 **Estado Atual (Snapshot)**

### **Estrutura Atual:**
```
game-engine/
├── engine/          # TUDO JUNTO (core + graphics)
│   ├── animation/   # ✅ Core
│   ├── asset/       # ✅ Core
│   ├── ecs/         # ✅ Core
│   ├── camera/      # ❌ Client only
│   ├── input/       # ❌ Client only
│   ├── render/      # ❌ Client only
│   ├── ui/          # ❌ Client only
│   └── window/      # ❌ Client only
├── src/
│   ├── app/         # ❌ Client only
│   ├── main.cpp     # Entry point
│   └── server/      # (vazio)
└── tools/
    └── dictionary_compiler/
```

### **Dependências Circulares Atuais:**
```
❌ PROBLEMA:
engine/ecs/world.h → #include render/renderer.h
engine/render/ → usa engine/ecs/

SOLUÇÃO:
Render NÃO deve ser incluído no core
Usar injeção de dependência
```

---

## 🎯 **Estrutura Alvo (Objetivo Final)**

```
game-engine/
├── engine/
│   ├── core/              # ✅ COMPARTILHADO (Math, Config, Log)
│   ├── ecs/               # ✅ COMPARTILHADO
│   ├── asset/             # ✅ COMPARTILHADO
│   ├── animation/         # ✅ COMPARTILHADO
│   ├── physics/           # ✅ COMPARTILHADO (criar depois)
│   └── shared/            # ✅ Structs/Types compartilhados
│
├── client/                # 🎨 NOVO - Graphics/UI
│   ├── render/            # Movido de engine/render/
│   ├── graphics/          # (criar)
│   ├── camera/            # Movido de engine/camera/
│   ├── window/            # Movido de engine/window/
│   ├── input/             # Movido de engine/input/
│   └── ui/                # Movido de engine/ui/
│
├── src/
│   ├── client/            # 🎮 Main game client
│   │   ├── game_client.cpp  # Novo entry point
│   │   └── app/           # Movido de src/app/
│   └── server/            # 🖥️ Server (estrutura vazia por ora)
│
├── tools/
│   └── dictionary_compiler/
│
└── shared/                # 📦 Binary formats
    └── mmap_format/
```

---

## 📅 **Cronograma de Execução**

### **FASE 1: Preparação e Backup** ⏱️ 10 minutos
- [x] Criar este documento
- [ ] Commit snapshot atual
- [ ] Criar tag `v1.0-pre-refactor`
- [ ] Criar branch `refactor/architecture-separation`
- [ ] Teste: Compilar e rodar (baseline)

### **FASE 2: Criar Estrutura de Pastas** ⏱️ 5 minutos
- [ ] Criar `client/` (vazio)
- [ ] Criar `src/client/` (vazio)
- [ ] Criar `engine/core/` (vazio)
- [ ] Criar `engine/shared/` (vazio)
- [ ] Teste: Compilar (deve funcionar igual)

### **FASE 3: Separar Core Utils** ⏱️ 15 minutos
- [ ] Mover `engine/core/log.*` → `engine/core/` (já está no lugar)
- [ ] Mover `engine/core/config_manager.*` → `engine/core/`
- [ ] Mover `engine/math/` → `engine/core/math/` (se existir separado)
- [ ] Atualizar CMakeLists.txt
- [ ] Atualizar includes
- [ ] Teste: Compilar + Rodar animação

### **FASE 4: Separar Window/Input** ⏱️ 20 minutos
- [ ] Mover `engine/window/` → `client/window/`
- [ ] Mover `engine/input/` → `client/input/`
- [ ] Atualizar CMakeLists.txt
- [ ] Atualizar includes em:
  - `src/app/`
  - `engine/ecs/systems/player_system.cpp`
- [ ] Teste: Compilar + Rodar + Testar W para walk

### **FASE 5: Separar UI** ⏱️ 10 minutos
- [ ] Mover `engine/ui/` → `client/ui/`
- [ ] Atualizar CMakeLists.txt
- [ ] Atualizar includes
- [ ] Teste: Compilar + Rodar

### **FASE 6: Separar Camera** ⏱️ 15 minutos
- [ ] Mover `engine/camera/` → `client/camera/`
- [ ] **⚠️ CUIDADO**: Manter linha 89 de `orbit_camera.cpp` (Right-Handed fix)
- [ ] Atualizar CMakeLists.txt
- [ ] Atualizar includes em:
  - `src/app/`
  - `engine/ecs/systems/player_system.h`
- [ ] Teste: Compilar + Rodar + Verificar câmera funcionando

### **FASE 7: Separar Render (ÚLTIMA - MAIS CRÍTICO)** ⏱️ 30 minutos
- [ ] Mover `engine/render/` → `client/render/`
- [ ] Atualizar CMakeLists.txt
- [ ] Remover includes circulares de `engine/ecs/`
- [ ] Atualizar todos os includes
- [ ] Teste: Compilar + Rodar + **VERIFICAR ANIMAÇÃO COMPLETA**

### **FASE 8: Reorganizar src/app/** ⏱️ 15 minutos
- [ ] Mover `src/app/` → `src/client/app/`
- [ ] Criar `src/client/game_client.cpp` (novo entry point)
- [ ] Atualizar CMakeLists.txt
- [ ] Teste: Compilar + Rodar

### **FASE 9: Criar Libs Separadas no CMake** ⏱️ 20 minutos
- [ ] `add_library(engine_core ...)` - Core compartilhado
- [ ] `add_library(engine_client ...)` - Client graphics
- [ ] `add_executable(game-engine ...)` - Link ambas
- [ ] Teste: Compilar + Rodar

### **FASE 10: Validação Final** ⏱️ 10 minutos
- [ ] Rodar jogo completo
- [ ] Testar idle/walk (W key)
- [ ] Testar todos os controles (WASD, Q/E, RMB+drag, Click-to-move)
- [ ] Verificar terreno tracking
- [ ] Verificar câmera orbit
- [ ] Commit: `refactor: arquitetura client/core separada`
- [ ] Merge para main

---

## 🧪 **Testes de Validação**

Após **CADA FASE**, executar:

```bash
# 1. Compilar
cmake --build build --target game-engine

# 2. Executar
cd build/src/Debug
./game-engine.exe

# 3. Validar:
✅ Jogo abre sem crash
✅ Personagem aparece em pé (não upside-down)
✅ Pressionar W → animação walk
✅ Soltar W → animação idle
✅ WASD move corretamente
✅ Camera orbit funciona
✅ Terreno tracking funciona
```

---

## 🔧 **CMakeLists.txt - Estrutura Alvo**

### **engine/CMakeLists.txt (Core)**
```cmake
# CORE - Compartilhado (sem graphics)
add_library(engine_core STATIC
    core/log.cpp
    core/config_manager.cpp
    ecs/world.cpp
    ecs/systems/animation_system.cpp
    asset/asset_manager.cpp
    animation/forward_kinematics.cpp
    # ... sem render/camera/window/input
)

target_link_libraries(engine_core
    # Sem GLFW, GLAD, OpenGL
    glm
)
```

### **client/CMakeLists.txt (Graphics)**
```cmake
# CLIENT - Graphics/UI
add_library(engine_client STATIC
    render/renderer.cpp
    render/shader.cpp
    camera/orbit_camera.cpp
    window/window.cpp
    input/input_manager.cpp
    ui/imgui_layer.cpp
)

target_link_libraries(engine_client
    engine_core  # Depende do core
    GLFW
    GLAD
    glm
    imgui
)
```

### **src/client/CMakeLists.txt (Executável)**
```cmake
add_executable(game-engine
    game_client.cpp
    app/app.cpp
    app/app_setup.cpp
)

target_link_libraries(game-engine
    engine_core
    engine_client
)
```

---

## 🔄 **Como Reverter se der Problema**

### **Voltar para tag de backup:**
```bash
git checkout v1.0-pre-refactor
```

### **Ou resetar a branch:**
```bash
git checkout main
git branch -D refactor/architecture-separation
```

---

## 📚 **Arquivos de Documentação Relacionados**

- `docs/animation_coordinate_system_workaround.md` - **CRÍTICO**: Workaround X=180°
- `docs/CONTINUACAO_ANIMATION_v101.md` - Sistema de animação v101
- `.github/copilot-instructions.md` - Instruções gerais
- `docs/asset_pipeline_anim.md` - Pipeline de assets

---

## ✅ **Checklist de Segurança**

Antes de fazer merge para `main`:

- [ ] Todos os testes passam
- [ ] Animação idle/walk funcionando
- [ ] Controles WASD/QE funcionando
- [ ] Câmera orbit funcionando
- [ ] Terrain tracking funcionando
- [ ] Personagem em pé (não upside-down)
- [ ] Bones não colapsados
- [ ] Sem warnings de compilação críticos
- [ ] Documentação atualizada

---

## 📝 **Notas Importantes**

### **Sobre o Workaround de Coordenadas:**
O sistema atual usa **X=180° rotation** que:
- ✅ Mantém estrutura óssea (única rotação que funciona)
- ⚠️ Inverte Y-axis (compensado com Scale Y=-1)
- ⚠️ Inverte Z-axis (compensado invertendo controles)

**NÃO mexer neste sistema até entender completamente!**

### **Sobre Dependencies:**
- Core NÃO pode depender de Client
- Client PODE depender de Core
- Usar injeção de dependência quando necessário

---

**Data de Criação**: 30/12/2025
**Última Atualização**: 30/12/2025
