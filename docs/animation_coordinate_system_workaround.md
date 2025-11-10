# Animation Coordinate System Workaround - Documentação Completa

## 📋 Sumário Executivo

**Status**: ✅ **FUNCIONANDO PERFEITAMENTE** (10/11/2025)

Após **50+ iterações de testes**, identificamos e implementamos uma solução funcional para importar skeletal animations de GLTF (Blender) para a engine OpenGL/GLM.

**Resultado**:
- ✅ Estrutura óssea mantida (sem colapso)
- ✅ Personagem em pé (não de cabeça para baixo)
- ✅ Orientação correta (olhando para frente)
- ✅ Todos os controles funcionando (WASD, Q/E, RMB+A/D, Click-To-Move)
- ✅ Terrain tracking funcional

---

## 🔍 Análise do Problema

### Contexto
A engine utiliza:
- **GLTF 2.0**: Formato de asset 3D (via cgltf)
- **Blender**: Ferramenta de modelagem (exporta GLTF Y-up Right-Handed)
- **OpenGL 3.3**: API gráfica
- **GLM 0.9.9.8**: Biblioteca de matemática (Right-Handed)

### Descoberta Crítica

**APENAS rotação X=180° mantém a estrutura óssea humanóide.**

Testamos sistematicamente:
- ❌ Sem correção: Colapso total
- ❌ X=90°, X=-90°: Colapso
- ❌ Y=180°, Y=90°, Y=-90°: Colapso
- ❌ Z=180°, Z=90°, Z=-90°: Colapso
- ❌ Combinações (X=180°+Y=180°, etc): Colapso
- ✅ **X=180° APENAS**: Estrutura mantida ✓

### Verificação Externa
Modelo exibido **CORRETAMENTE** em:
- ✅ McCurdy GLTF Viewer (web)
- ✅ Blender
- ✅ Outros viewers GLTF

**Conclusão**: Dados do modelo estão corretos. Problema é interpretação da engine.

### Root Cause (Hipótese)

A pipeline de animação da engine (Forward Kinematics + Inverse Bind Matrices) interpreta dados GLTF de forma que requer rotação X=180° para manter hierarquia óssea.

**Não é bug da câmera**: Camera foi corrigida independentemente (Left→Right-Handed) e ainda necessita X=180°.

---

## ⚙️ Configuração Completa (WORKING)

### 1️⃣ Animation Data Loading
**Arquivo**: `engine/asset/animation_data_mapper.cpp`

#### A) Rotation Keyframes (linha ~342)
```cpp
// Dentro de mapAnimationData(), ao processar rotations
glm::quat coordinateCorrection = glm::angleAxis(
    glm::radians(180.0f), 
    glm::vec3(1.0f, 0.0f, 0.0f)  // X-axis
);
rotationValue = coordinateCorrection * rotationValue;
```

**Motivo**: X=180° é ÚNICA rotação que mantém estrutura óssea (provado empiricamente).

**Side Effect**: Inverte Y-axis (upside-down) E Z-axis (backwards).

#### B) Inverse Bind Matrices (linha ~175)
```cpp
// Dentro de processBoneNode(), ao processar IBMs
glm::mat4 coordinateCorrection = glm::rotate(
    glm::mat4(1.0f), 
    glm::radians(180.0f), 
    glm::vec3(1.0f, 0.0f, 0.0f)  // X-axis
);
ibm = ibm * glm::inverse(coordinateCorrection);
```

**Motivo**: IBMs também precisam da mesma correção para manter consistência.

**⚠️ CRÍTICO**: Ambas correções (keyframes + IBMs) são **OBRIGATÓRIAS**. Remover qualquer uma causa colapso.

---

### 2️⃣ Player Transform Setup
**Arquivo**: `src/app/app_setup.cpp` (linhas ~143-154)

```cpp
// Transform Component
world.addComponent<Transform>(playerEntity);
auto& playerTransform = world.getComponent<Transform>(playerEntity);

// WORKAROUND: X=180° pipeline inverte Y+Z
glm::vec3 correctedPos = startPos;
correctedPos.y = -correctedPos.y;  // ⚠️ Negativa devido ao Scale Y=-1

playerTransform.position = Engine::Math::Vec3(correctedPos);
playerTransform.rotation = Engine::Math::Quat(glm::identity<glm::quat>()); // ✅ Identity = frente
playerTransform.scale = Engine::Math::Vec3(1.0f, -1.0f, 1.0f);  // ⚠️ Y=-1 flipa upside-down
```

#### Explicação:
1. **Scale Y=-1**: Flipa personagem de ponta-cabeça de volta para em pé
   - X=180° inverte Y-axis → Scale Y=-1 corrige
2. **Position Y negada**: Compensação para Scale Y=-1
   - Scale negativa inverte coordenadas Y → negar antes compensa
3. **Rotation Identity**: Quaternion identidade = olhando para frente
   - Não precisa Y=180° (isso invertia orientação)

---

### 3️⃣ Player Movement Controls
**Arquivo**: `engine/ecs/systems/player_system.cpp`

#### A) WASD Z-Inversion (linhas ~136-137)
```cpp
// Direções baseadas na câmera
glm::vec3 camF = m_camera.getForwardVector();
glm::vec3 camR = m_camera.getRightVector();
camF.y = 0.0f;
camR.y = 0.0f;
camF = Engine::Camera::Math::safeNormalize(camF);
camR = Engine::Camera::Math::safeNormalize(camR);

// WORKAROUND: X=180° inverte Z-axis dos controles
camF.z = -camF.z;
camR.z = -camR.z;
```

**Motivo**: X=180° inverte Z-axis. Invertemos vetores de câmera para compensar.

#### B) Q/E Strafe Inversion (linhas ~151-154)
```cpp
// Q/E = strafe (invertido para compensar X=180° pipeline)
if (m_inputManager.IsKeyPressed(GLFW_KEY_Q))
    dir += camR;  // Q vai para DIREITA (normalmente seria -= camR)
if (m_inputManager.IsKeyPressed(GLFW_KEY_E))
    dir -= camR;  // E vai para ESQUERDA (normalmente seria += camR)
```

**Motivo**: Z-inversion inverte lateralidade (left/right). Q/E invertidos para compensar.

#### C) RMB+A/D Strafe Inversion (linhas ~160-163)
```cpp
// Com RMB, A/D viram strafe (invertido como Q/E)
if (isRMB)
{
    if (m_inputManager.IsKeyPressed(GLFW_KEY_A))
        dir += camR;  // A vai para DIREITA (normalmente seria -= camR)
    if (m_inputManager.IsKeyPressed(GLFW_KEY_D))
        dir -= camR;  // D vai para ESQUERDA (normalmente seria += camR)
}
```

**Motivo**: Mesma razão que Q/E - compensação para Z-inversion.

#### D) Click-To-Move (CTM) - Normal
```cpp
// CTM (sem inversão especial - atan2 funciona normal)
glm::vec3 to = mv.targetDestination.toGLM() - tr.position.toGLM();
tr.position += Engine::Math::Vec3(glm::normalize(to) * vel);

glm::vec3 horiz(to.x, 0.0f, to.z);
if (glm::length(horiz) > 1e-3f)
{
    float targetYaw = atan2(horiz.x, horiz.z);  // ✅ Sem inversão
    tr.rotation = Engine::Math::Quat(glm::angleAxis(targetYaw, glm::vec3(0, 1, 0)));
}
```

**Motivo**: `atan2` calcula ângulo diretamente de vetor. Z-inversion já aplicada nos vetores, então CTM funciona normal.

---

### 4️⃣ Terrain Tracking
**Arquivo**: `engine/ecs/systems/terrain_tracking_system.cpp` (linhas ~108-112)

```cpp
// Ajusta altura baseado em raycasting
if (transform.scale.y < 0.0f)  // ⚠️ Detecta Scale Y=-1
{
    // Inverte altura + offset para root bone
    transform.position.y = -groundHeight + 1.5f;
    // +1.5f = altura do root bone acima do "pé" do personagem
}
else
{
    transform.position.y = groundHeight;
}
```

**Motivo**: Scale Y=-1 inverte coordenadas Y. Negamos altura + offset empírico (1.5 unidades).

---

### 5️⃣ Camera System (Fix Independente)
**Arquivo**: `engine/camera/orbit_camera.cpp` (linha ~89)

```cpp
glm::vec3 OrbitCamera::getForwardVector() const
{
    // Right-Handed: -Z forward (GLM padrão)
    return glm::vec3(sin(yaw), 0, -cos(yaw));  // ✅ -cos para Right-Handed
}
```

**Motivo**: Camera estava usando lógica Left-Handed (+Z forward) com funções GLM Right-Handed (-Z forward). Fix independente do workaround de animação, mas ambos necessários.

---

## 🧪 Testes Realizados

### Matriz de Testes (50+ iterações)

| Configuração | Keyframes | IBMs | Scale | Rotation | Resultado |
|--------------|-----------|------|-------|----------|-----------|
| Sem correção | Identity | Identity | (1,1,1) | Identity | ❌ Colapso total |
| X=90° | X=90° | X=90° | (1,1,1) | Identity | ❌ Colapso |
| X=-90° | X=-90° | X=-90° | (1,1,1) | Identity | ❌ Colapso |
| Y=180° | Y=180° | Y=180° | (1,1,1) | Identity | ❌ Colapso |
| Z=180° | Z=180° | Z=180° | (1,1,1) | Identity | ❌ Colapso |
| X=180° | X=180° | X=180° | (1,1,1) | Identity | ✅ Estrutura OK, upside-down |
| X=180° | X=180° | X=180° | (1,-1,1) | Identity | ✅ Estrutura OK, em pé, de costas |
| X=180° | X=180° | X=180° | (1,-1,1) | Y=180° | ✅ Estrutura OK, em pé, frente (controles invertidos) |
| **FINAL** | **X=180°** | **X=180°** | **(1,-1,1)** | **Identity** | ✅✅✅ **PERFEITO** (com inversões de controle) |

### Verificações
- ✅ Bone hierarchy mantida (logs FK mostram Y-axis correto)
- ✅ Personagem em pé (não upside-down)
- ✅ Orientação correta (frente = -Z)
- ✅ WASD: W=frente, S=trás, A/D=rotação câmera
- ✅ Q/E: Q=strafe direita, E=strafe esquerda
- ✅ RMB+A/D: A=strafe direita, D=strafe esquerda
- ✅ CTM: Anda de frente para destino clicado
- ✅ Terrain tracking: Personagem na altura correta do chão

---

## 📊 Impacto no Código

### Arquivos Modificados
1. ✅ `engine/asset/animation_data_mapper.cpp` - X=180° em keyframes + IBMs
2. ✅ `src/app/app_setup.cpp` - Scale Y=-1, Position Y negada, Rotation Identity
3. ✅ `engine/ecs/systems/player_system.cpp` - Z-inversion + inversões Q/E/A/D
4. ✅ `engine/ecs/systems/terrain_tracking_system.cpp` - Compensação Scale Y=-1
5. ✅ `engine/camera/orbit_camera.cpp` - Right-Handed fix (independente)

### Código Limpo (Sem Modificações)
- ✅ `engine/ecs/systems/animation_system.cpp` - Usa rest pose puro
- ✅ `engine/shaders/animated.vert` - Shader padrão (sem correções)
- ✅ `engine/animation/forward_kinematics.cpp` - FK puro (sem correções)
- ✅ `engine/asset/gltf_loader.cpp` - Contém função unused (dead code)

---

## 🔮 Solução Long-Term

### Opção 1: Re-exportar Modelo (RECOMENDADO)
**Objetivo**: Eliminar workarounds re-exportando modelo de Blender com eixos corretos.

**Passos**:
1. Abrir modelo no Blender
2. Aplicar rotação X=180° ao armature ANTES de exportar
3. Exportar GLTF com nova configuração
4. Remover workarounds do código:
   - Remover X=180° de `animation_data_mapper.cpp`
   - Remover Scale Y=-1 de `app_setup.cpp`
   - Remover inversões de controles de `player_system.cpp`
   - Remover compensação de `terrain_tracking_system.cpp`

**Prós**:
- ✅ Código limpo (sem workarounds)
- ✅ Alinhado com convenções da indústria
- ✅ Facilita debugging futuro

**Contras**:
- ⚠️ Requer acesso ao asset original no Blender
- ⚠️ Precisa re-testar tudo após re-export

### Opção 2: Manter Workarounds
**Objetivo**: Documentar e manter solução atual.

**Prós**:
- ✅ Funciona perfeitamente agora
- ✅ Não requer modificar assets

**Contras**:
- ⚠️ Código complexo (múltiplos workarounds)
- ⚠️ Dificulta onboarding de novos devs
- ⚠️ Risco de quebrar acidentalmente

---

## 🚨 AVISOS CRÍTICOS

### ⛔ NÃO FAÇA:
1. ❌ **Remover X=180° de keyframes OU IBMs**: Causa colapso imediato
2. ❌ **Mudar para outro ângulo/eixo**: 50+ testes provam que APENAS X=180° funciona
3. ❌ **Remover Scale Y=-1**: Personagem fica upside-down
4. ❌ **Remover inversões de controles**: Controles ficam espelhados
5. ❌ **Modificar apenas 1 arquivo**: Workaround é multi-arquivo (sistema coordenado)

### ✅ FAÇA:
1. ✅ **Consultar esta doc antes de modificar animação**
2. ✅ **Testar TODOS os controles** após qualquer mudança
3. ✅ **Verificar modelo em viewer externo** se houver colapso
4. ✅ **Logar FK_DEBUG** se houver problemas de hierarquia
5. ✅ **Considerar re-export do Blender** como solução permanente

---

## 📝 Histórico de Mudanças

### 2025-11-10: ✅ Solução Final Implementada
- X=180° em keyframes + IBMs (animation_data_mapper.cpp)
- Scale Y=-1 + Position Y negada + Rotation Identity (app_setup.cpp)
- Z-inversion + Q/E/A/D inversões (player_system.cpp)
- Scale Y=-1 compensation (terrain_tracking_system.cpp)
- Camera Right-Handed fix (orbit_camera.cpp)
- **Status**: TUDO FUNCIONANDO PERFEITAMENTE ✅

### 2025-11-09: 🔬 Fase de Testes Intensivos
- 50+ iterações testando diferentes rotações/configurações
- Descoberta: APENAS X=180° mantém estrutura
- Verificação externa: Modelo correto em McCurdy viewer
- Consulta Google Gemini: Identificou bug de handedness da câmera
- Camera fix: Left→Right-Handed (independente)

### 2025-11-08: 🐛 Problema Inicial
- Skeletal animation com colapso de bones
- Tentativas iniciais com diferentes correções
- Ainda não identificada necessidade de X=180°

---

## 🔗 Referências

### Código Relevante
- `engine/asset/animation_data_mapper.cpp`: Linhas 175 (IBM), 342 (keyframes)
- `src/app/app_setup.cpp`: Linhas 143-154 (Transform setup)
- `engine/ecs/systems/player_system.cpp`: Linhas 136-137 (Z-inv), 151-154 (Q/E), 160-163 (RMB+A/D)
- `engine/ecs/systems/terrain_tracking_system.cpp`: Linhas 108-112 (Height)
- `engine/camera/orbit_camera.cpp`: Linha 89 (Forward vector)

### Documentação
- `.github/copilot-instructions.md`: Seção "Animation Pipeline (CRITICAL)"
- `docs/asset_pipeline_anim.md`: Pipeline de importação GLTF
- `docs/need_to_refactor.md`: Débito técnico conhecido

### Ferramentas de Verificação
- [McCurdy GLTF Viewer](https://gltf-viewer.donmccurdy.com/)
- Blender 3.x+ (File > Import > GLTF 2.0)

---

## 🎓 Lições Aprendidas

1. **Testes Sistemáticos Funcionam**: 50+ iterações identificaram solução única
2. **Verificação Externa é Crucial**: McCurdy viewer provou que modelo estava correto
3. **Coordinate Systems são Complexos**: Handedness + IBMs + FK = muitas variáveis
4. **Workarounds Temporários OK**: Se documentados adequadamente
5. **Re-export de Assets > Workarounds Code**: Long-term, sempre priorize dados corretos

---

**Última Atualização**: 10/11/2025  
**Status**: ✅ PRODUÇÃO - FUNCIONANDO  
**Maintainer**: Equipe Engine Dev  
**Documentado por**: AI Assistant + User Testing
