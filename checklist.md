# ✅ Checklist de Módulos Fundamentais da Engine

## 🎯 Objetivo
Construir uma base sólida e escalável da engine para que, após a implementação inicial, novos conteúdos (shaders, entidades, cenas) possam ser adicionados via arquivos externos e módulos especializados, sem retrabalho estrutural em C++.

---

## 🔧 Núcleo Técnico (C++)

### ✅ Inicialização
- [x] Setup do `main.cpp` com janela GLFW + contexto OpenGL
- [x] Inicialização do Glad, Depth Test e viewport

### ✅ Renderização
- [x] Sistema de Shader modularizado (`shader.h/.cpp`)
- [x] Carregamento de vertex/fragment shader a partir de arquivos externos
- [x] Loader de arquivos com path absoluto dinâmico (via `std::filesystem`)

### ⬜ Sistema de Materiais e Uniforms
- [ ] `setVec3`, `setFloat`, `setMat4` etc. (métodos em `Shader`)
- [ ] Padronização de uniforms para transformação (model/view/projection)

### ⬜ Geometria
- [x] Grid base 3D
- [ ] Loader genérico de geometria (ex: `.obj`)
- [ ] Classe primitiva para esfera, cubo e plano

### ⬜ Sistema de Câmera
- [ ] Câmera com controles via teclado e mouse
- [ ] Sistema de movimentação com velocidade e aceleração
- [ ] Sensibilidade e inversão de eixo configuráveis

---

## 🎮 Entidades e Mundo

### ⬜ Sistema de Entidade Básico
- [ ] Classe `Entity` com Transform (pos/rot/scale)
- [ ] Hierarquia de cena (ex: filhos seguem pais)

### ⬜ Jogador (Personagem)
- [ ] Esfera como placeholder
- [ ] Movimento com física básica (pular, andar)
- [ ] Controle da câmera em terceira ou primeira pessoa

---

## 🧪 Utilitários

### ⬜ Loggers
- [ ] Logger básico para debug com categorias (info, warning, error)

### ⬜ Sistema de Tempo
- [ ] DeltaTime e frameRate
- [ ] FPS counter

---

## 📦 Conteúdo Externo

### ⬜ Sistema de Shaders
- [ ] Shader "default"
- [ ] Shader com iluminação Phong
- [ ] Shader para UI

### ⬜ Sistema de Assets
- [ ] Estrutura de pastas para modelos, texturas e shaders
- [ ] Loader genérico para assets com cache

---

> ⚙️ À medida que os módulos acima forem finalizados, novos recursos serão adicionados alterando apenas arquivos externos (shaders, modelos, configurações), mantendo o núcleo em C++ estável e desacoplado.

