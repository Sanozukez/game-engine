# Guia: Ajustar Animação In-Place no Blender

## 🎯 Objetivo

Criar animação de walk **in-place** (personagem anda no lugar) que **case perfeitamente** com a velocidade de movimento do código (`movement_speed = 5.5 m/s`), eliminando **foot sliding** (pés deslizando no chão).

---

## 📐 Teoria: Matching Animation com Velocidade

### **Problema: Foot Sliding**

Quando você anima **in-place**, o personagem se move programaticamente:
```cpp
position += forward * movementSpeed * dt; // 5.5 metros/segundo
```

Se a animação não "andar" a mesma distância virtual por segundo, os pés deslizam no chão.

### **Solução: Calcular Duração/Frames Corretos**

**Fórmula Básica**:
```
stride_length = distância que o pé avança em 1 passo completo (ex: 1.1 metros)
strides_per_second = movement_speed / stride_length
animation_duration = 1.0 / strides_per_second
```

**Exemplo (Walk típico MMORPG)**:
- **Velocidade**: 5.5 m/s
- **Stride Length** (comprimento da passada): ~1.1 metros (típico para humanóide)
- **Passos por segundo**: 5.5 / 1.1 = **5 passos/segundo**
- **Duração da animação**: 1.0 / 5.0 = **0.2 segundos** para 1 passo completo

Mas em walk, normalmente fazemos **2 passos** (esquerda + direita) em 1 ciclo:
- **Ciclo completo**: 2 passos = **0.4 segundos** (2.5 ciclos/segundo)
- **A 30 FPS**: 0.4s × 30fps = **12 frames** para ciclo completo
- **A 24 FPS**: 0.4s × 24fps = **9-10 frames** para ciclo completo

---

## 🧮 CALCULADORA: Seu Caso (movement_speed = 5.5)

### **Dados da Sua Animação Atual**

Primeiro, você precisa medir no Blender:

1. **FPS do projeto**: (veja Timeline → Framerate) - Normalmente 24 ou 30
2. **Frames totais do ciclo**: Quantos frames tem sua animação de walk completa?
3. **Stride Length**: Distância que o pé avança durante a animação

---

### **Cenário A: Animação Já Existe (Ajustar Duração)**

Se você já tem uma animação de walk, vamos calcular **quanto tempo ela deveria durar**:

#### **Medindo Stride Length no Blender**

1. Abra a animação de walk
2. Selecione o bone do pé esquerdo (`Foot.L` ou similar)
3. Vá para o frame onde o pé **toca o chão** (contact pose)
4. Anote a posição Z do pé (ex: `Z = 0.5`)
5. Avance para o próximo contact do **mesmo pé** (1 passada completa)
6. Anote a nova posição Z (ex: `Z = 1.6`)
7. **Stride Length** = diferença = `1.6 - 0.5 = 1.1 metros`

#### **Fórmula para Duração Ideal**

```python
# Dados
movement_speed = 5.5  # m/s (do config)
stride_length = 1.1   # metros (medido no Blender)

# Cálculo
steps_per_second = movement_speed / stride_length  # = 5.0 passos/s
duration_per_stride = 1.0 / steps_per_second       # = 0.2 segundos por passo

# Se animação tem 2 passos (esquerda + direita) em 1 ciclo:
animation_duration = duration_per_stride * 2       # = 0.4 segundos
```

#### **Converter para Frames**

```python
fps = 30  # Framerate do projeto Blender
frames_needed = animation_duration * fps  # = 0.4 * 30 = 12 frames
```

**Resultado**: Sua animação de walk deve ter **12 frames** a 30 FPS (ou **10 frames** a 24 FPS).

---

### **Cenário B: Criar Animação Nova (Recomendado)**

Se vai criar do zero, siga estas especificações:

#### **Specs para Walk @ 5.5 m/s**

| Parâmetro | Valor | Explicação |
|-----------|-------|------------|
| **FPS** | 30 | Padrão MMORPG (ou 24 para estilo mais "pesado") |
| **Frames Totais** | 12-16 | 12 = rápido, 16 = mais natural |
| **Duração** | 0.4s - 0.53s | 12f@30fps=0.4s, 16f@30fps=0.53s |
| **Stride Length** | 1.1m | Distância pé esquerdo → direito |
| **Root Motion** | 0 | In-place (root fica parado em X/Z) |
| **Hip Height Variance** | ±0.03m | Sutil movimento vertical (realismo) |

#### **Recomendação (Testado em MMORPGs)**

**Walk @ 30 FPS = 16 frames** (0.53s de duração)

Por quê? 12 frames fica rápido demais (5 passos/segundo parece corrida leve). 16 frames dá **3.75 passos/segundo**, mais natural para walk.

Ajuste o `playbackSpeed` no código para compensar:

```cpp
// Walk padrão: 16 frames @ 30fps = 0.53s
// Velocidade ideal: 5.5 / 1.1 = 5 passos/s
// Velocidade da animação: 3.75 passos/s (16 frames)
// playbackSpeed = 5.0 / 3.75 = 1.33x

animComp.playbackSpeed = 1.33f; // Acelera 33% para casar com 5.5 m/s
```

**OU** crie duas versões:
- **walk.glb**: 16 frames @ 5.5 m/s → `playbackSpeed = 1.33f`
- **run.glb**: 12 frames @ 8.0 m/s → `playbackSpeed = 1.0f` (ideal para corrida)

---

## 🛠️ Passo-a-Passo no Blender

### **1. Setup Inicial**

1. Abra seu `character_test.blend`
2. Selecione o Armature
3. Entre em **Pose Mode** (`Ctrl+Tab`)
4. Abra o **Dope Sheet** (Animation workspace)
5. Certifique-se que FPS = 30 (Timeline → Framerate)

### **2. Criar Animação Walk (In-Place)**

#### **Frame 1: Contact Pose (Pé Esquerdo)**
- Pé esquerdo no chão (flat), joelho reto
- Pé direito atrás, calcanhar levantado
- Braço direito pra frente, esquerdo pra trás
- Hip (Hips-Main) na altura padrão

#### **Frame 5: Passing Pose**
- Pé direito passa pelo esquerdo
- Ambos os joelhos dobrados (lowest hip position)
- Braços no meio do swing

#### **Frame 9: Contact Pose (Pé Direito)**
- Pé direito no chão (flat), joelho reto
- Pé esquerdo atrás, calcanhar levantado
- Braço esquerdo pra frente, direito pra trás
- Hip na altura padrão

#### **Frame 13-16: Retorno ao Frame 1**
- Passing pose do pé esquerdo
- Interpolação suave de volta ao contact pose inicial

### **3. Keyframe os Bones**

**Bones essenciais** (marcar keyframes em TODOS os frames importantes):

- **Hips-Main**: Leve movimento Y (altura) e rotação X (pitch)
- **Spine-01/02/03**: Contra-rotação sutil (opposite arms)
- **Thigh.L/R**: Rotação X (forward/backward)
- **Shin.L/R**: Rotação X (knee bend)
- **Foot.L/R**: Rotação X (ankle flex)
- **Upper_arm.L/R**: Rotação X (swing)
- **Forearm.L/R**: Leve bend no elbow

**IMPORTANTE**: **NÃO** mova o Root bone em X ou Z! Apenas Y (altura mínima).

### **4. Verificar In-Place**

1. Selecione o Root bone (`root` ou `Hips-Main`)
2. No **Graph Editor**, verifique as curvas:
   - **Location X**: Deve estar em 0 constante
   - **Location Z**: Deve estar em 0 constante
   - **Location Y**: Pode ter leve variação (±0.03m)

Se X ou Z mudam, você tem **root motion** (personagem anda no Blender). Para in-place:

1. Selecione todas as curvas X e Z do Root
2. **Delete** ou **Set to 0**

### **5. Exportar para GLTF**

1. **File → Export → glTF 2.0**
2. **Settings**:
   - Format: `.glb` (Binary)
   - Include: Selected Objects (Armature + Mesh)
   - Transform: +Y Up (CRITICAL!)
   - Animation: ✅ Export Deformation Bones Only
   - Animation → Shape Keys: ❌ (se não usar)
   - Sampling Rate: 30 (ou seu FPS)
3. Salvar como `character_walk.glb` em `assets/models/`

---

## 📊 Tabela de Referência: Frames por Velocidade

| Velocidade (m/s) | Tipo | Stride (m) | FPS | Frames | Duração (s) | playbackSpeed |
|------------------|------|------------|-----|--------|-------------|---------------|
| **5.5** | Walk | 1.1 | 30 | 16 | 0.53 | 1.33x |
| **8.0** | Run | 1.6 | 30 | 12 | 0.40 | 1.0x |
| **11.0** | Sprint | 2.2 | 30 | 12 | 0.40 | 1.25x |
| **14.0** | Mount | 2.8 | 30 | 12 | 0.40 | 1.67x |

**Como usar**:

1. Anime com os **Frames** da tabela
2. Configure `playbackSpeed` no código quando mudar velocidade
3. OU crie animações separadas (walk/run/sprint) e troque via `playAnimation()`

---

## 🎮 Integração com o Código

### **Opção 1: Usar playbackSpeed (Simples)**

```cpp
// src/app/app_setup.cpp (ou onde criar player)
auto& animComp = world.getComponent<AnimationComponent>(playerID);

// Animação base: 16 frames @ 30fps = 0.53s
// Velocidade ideal: 5.5 / 1.1 = 5 passos/s
// Velocidade real da anim: 3.75 passos/s
// Correção: 5.0 / 3.75 = 1.33x
animComp.playbackSpeed = 1.33f;
```

### **Opção 2: Criar Animação Exata (Profissional)**

Criar animação com duração **exata**:

```python
# Calcular frames exatos
movement_speed = 5.5
stride_length = 1.1
fps = 30

strides_per_second = movement_speed / stride_length  # 5.0
duration = 2.0 / strides_per_second  # 0.4s (2 passos)
frames = int(duration * fps)  # 12 frames

# No Blender: Criar animação com exatamente 12 frames
```

Então no código:
```cpp
animComp.playbackSpeed = 1.0f; // Animação já perfeita!
```

### **Opção 3: Diferentes Velocidades (MMORPG)**

```cpp
// PlayerSystem::update()
auto& animComp = world.getComponent<AnimationComponent>(playerID);
auto& movement = world.getComponent<Movement>(playerID);

float currentSpeed = movement.movementSpeed;

if (currentSpeed < 1.0f) {
    // Idle
    playAnimation(animComp, ANIM_IDLE, 0.3f);
    animComp.playbackSpeed = 1.0f;
}
else if (currentSpeed <= 6.0f) {
    // Walk
    playAnimation(animComp, ANIM_WALK, 0.2f);
    // Ajustar playbackSpeed baseado na velocidade real
    // walk.glb foi feita para 5.5 m/s
    animComp.playbackSpeed = currentSpeed / 5.5f;
}
else if (currentSpeed <= 10.0f) {
    // Run
    playAnimation(animComp, ANIM_RUN, 0.2f);
    // run.glb foi feita para 8.0 m/s
    animComp.playbackSpeed = currentSpeed / 8.0f;
}
else {
    // Sprint
    playAnimation(animComp, ANIM_SPRINT, 0.15f);
    // sprint.glb foi feita para 11.0 m/s
    animComp.playbackSpeed = currentSpeed / 11.0f;
}
```

---

## 🧪 Teste no Engine

### **1. Verificar se playbackSpeed funciona**

```cpp
// Teste rápido: Acelerar animação 2x
animComp.playbackSpeed = 2.0f;
// Se funcionar, animação deve rodar 2x mais rápida
```

### **2. Verificar foot sliding visualmente**

1. Rode o jogo
2. Olhe para o pé do personagem enquanto anda
3. **Pé no chão deve estar estático** (sem deslizar)
4. Se deslizar:
   - playbackSpeed muito alto → diminuir
   - playbackSpeed muito baixo → aumentar

### **3. Ajuste Fino**

```cpp
// Começar com valor teórico
animComp.playbackSpeed = 1.33f;

// Testar no jogo e ajustar por feel:
// - Se pés deslizam pra frente: aumentar (1.4f, 1.5f...)
// - Se pés deslizam pra trás: diminuir (1.2f, 1.1f...)
```

**Tipicamente**: Diferença de ±0.1 já é perceptível.

---

## 🎨 Dicas de Animação (Estilo MMORPG)

### **Walk Natural (Rohan/MU Style)**

- **Hip Sway**: Leve rotação Y do Hip (±5°) seguindo perna de apoio
- **Shoulder Rotation**: Ombros rotacionam opposite aos hips (±3°)
- **Head Bob**: Cabeça acompanha levemente o movimento vertical (±0.02m)
- **Arm Swing**: Braços swing ~30° pra frente, ~20° pra trás
- **Toe Roll**: Pé rola do calcanhar → ball → toe (3 keyframes por passo)

### **Timing**

```
Contact (Frame 1) → Down (Frame 3) → Passing (Frame 5) → Up (Frame 7) → Contact (Frame 9)
```

- **Contact**: Perna extendida, pé flat no chão
- **Down**: Peso transfere, hip abaixa (~-0.03m)
- **Passing**: Pé livre passa pela perna de apoio, hip no ponto mais baixo
- **Up**: Pé livre à frente, hip sobe (~+0.03m)
- **Contact**: Pé livre toca o chão, ciclo recomeça

### **Reference Videos**

- **MU Online Walk**: https://www.youtube.com/watch?v=... (procure "MU Online character walk")
- **Rohan Walk**: https://www.youtube.com/watch?v=... (procure "Rohan Blood Feud walk")
- **Mixamo Walk**: https://www.mixamo.com/ (download walk @ 30fps, 16 frames)

---

## 🐛 Troubleshooting

### **"Pés deslizam mesmo com playbackSpeed correto"**

**Possíveis causas**:
1. **Root bone se movendo**: Verifique no Graph Editor (X/Z devem ser 0)
2. **Stride length errado**: Medir de novo a distância entre contact poses
3. **Ciclo não loopa**: Último frame deve ser igual ao primeiro (ou bem próximo)
4. **FPS diferente**: GLTF exportado a 24fps mas engine roda a 30fps

**Solução**: Re-exportar GLTF com "Sampling Rate" = 30 (ou FPS do engine)

### **"Animação parece 'robótica'"**

**Causas**:
1. Poucos keyframes (só 2-3 por ciclo)
2. Interpolação Linear em vez de Bezier
3. Sem breakdown poses (passing, up, down)

**Solução**: Adicionar breakdowns e mudar interpolação:
- Select All Keyframes → Key → Interpolation Mode → Bezier

### **"playbackSpeed > 1.5x parece cartoon"**

**Normal!** Acima de 1.5x, a animação fica "acelerada".

**Soluções**:
1. Criar animação separada (run.glb) para velocidades 8+ m/s
2. Limitar playbackSpeed max a 1.5x no código
3. Aceitar o estilo "arcade" (MU Online faz isso!)

### **"Como medir stride length se animação é in-place?"**

Se animação é 100% in-place (pé em 0,0,0 sempre), você **escolhe** o stride:

**Típico humanóide**:
- **Walk**: 1.0 - 1.2 metros
- **Run**: 1.5 - 1.8 metros
- **Sprint**: 2.0 - 2.5 metros

Use esses valores na fórmula e ajuste `playbackSpeed` depois.

---

## 📝 Checklist: Animação Pronta?

- [ ] Animação tem 12-16 frames @ 30 FPS
- [ ] Root bone X/Z = 0 (in-place)
- [ ] Ciclo loopa perfeitamente (frame 1 = frame último)
- [ ] Exportado como GLTF 2.0 (.glb)
- [ ] +Y Up no export
- [ ] Sampling Rate = 30 fps
- [ ] Testado no engine com `playbackSpeed = 1.0f` (baseline)
- [ ] Ajustado `playbackSpeed` para casar com `movement_speed = 5.5`
- [ ] Sem foot sliding visualmente

---

## 🚀 Próximos Passos

1. **Criar animação walk** seguindo este guia (16 frames @ 30fps)
2. **Exportar** como `character_walk.glb`
3. **Adicionar ao asset_dictionary.json**
4. **Testar** com `playbackSpeed = 1.33f`
5. **Ajustar** por feel até eliminar foot sliding
6. **Criar run.glb** (12 frames, 8 m/s) para Sprint skill futuro

---

## 📚 Recursos Adicionais

- **Blender Animation Course**: https://www.youtube.com/playlist?list=... (procure "walk cycle tutorial")
- **MMORPG Animation References**: Mixamo, CGTrader, Sketchfab
- **Root Motion vs In-Place**: https://docs.unrealengine.com/... (conceito universal)

---

**Autor**: GitHub Copilot  
**Data**: 10 de Novembro de 2025  
**Engine**: game-engine v0.1 (C++20, OpenGL 3.3)  
**Config Atual**: `movement_speed = 5.5 m/s` ✅
