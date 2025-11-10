# Convenção de Nomenclatura de Animações - Padrão Profissional

## 🎯 Objetivo

Definir **padrão de nomenclatura** para animações em GLB/GLTF, garantindo consistência entre Blender, Engine e Editor visual futuro.

---

## 📊 **RESPOSTA RÁPIDA: Padrões da Indústria**

### ✅ **SIM, é padrão profissional:**

1. **Um arquivo GLB com múltiplas animações** (NLA Actions no Blender)
   - ✅ Unreal Engine: Skeleton + AnimSequences em 1 arquivo
   - ✅ Unity: FBX com múltiplos clips
   - ✅ Godot: GLTF com múltiplas animations
   - ✅ Source Engine (Half-Life, CS:GO): .mdl + .ani

2. **Nomenclatura padronizada** (ex: `idle`, `walk`, `run`)
   - ✅ **CRÍTICO** para ferramentas automatizadas
   - ✅ Mixamo usa: `Idle`, `Walk`, `Run`, `Jump`
   - ✅ AAA games: `chr_idle`, `chr_walk_fwd`, `chr_run_start`

### ✅ **Onde especificar os nomes?**

**Opção A - Asset Dictionary (RECOMENDADO para MMORPG)**:
```json
{
  "assets": [
    {
      "id": 1001,
      "name": "character_male_warrior.glb",
      "type": "CharacterModel",
      "animations": {
        "idle": "idle_combat",
        "walk": "walk_forward",
        "run": "run_sprint",
        "attack_1": "attack_sword_slash",
        "skill_1": "skill_whirlwind"
      }
    }
  ]
}
```

**Opção B - Dentro do GLB (metadata)**:
```json
// Embedded no GLTF "extras" field
{
  "animations": [
    {
      "name": "idle_combat",
      "extras": {
        "gameAnimationType": "idle",
        "priority": 0,
        "looping": true
      }
    }
  ]
}
```

---

## 🏗️ **ARQUITETURA RECOMENDADA (3 Camadas)**

```
┌─────────────────────────────────────────────────────┐
│          LAYER 1: Blender (Source Truth)           │
│  Actions NLA:                                       │
│  - idle_combat                                      │
│  - walk_forward                                     │
│  - run_sprint                                       │
│  - attack_sword_slash                               │
│                                                     │
│  Export → character_male_warrior.glb                │
└─────────────────┬───────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────┐
│     LAYER 2: Asset Dictionary (Mapping Layer)      │
│  data/asset_dictionary.json:                        │
│  {                                                  │
│    "id": 1001,                                      │
│    "animations": {                                  │
│      "idle": "idle_combat",    ← Engine usa "idle" │
│      "walk": "walk_forward",   ← Blender tem isso  │
│      "attack_1": "attack_sword_slash"               │
│    }                                                │
│  }                                                  │
└─────────────────┬───────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────┐
│        LAYER 3: Engine Runtime (Hash IDs)          │
│  AnimationComponent:                                │
│  - currentAnimationID = hash("idle")                │
│                                                     │
│  AssetManager resolve:                              │
│  hash("idle") → "idle_combat" (do dictionary)       │
│              → AnimationAsset* (do GLB)             │
└─────────────────────────────────────────────────────┘
```

---

## 📝 **CONVENÇÃO DE NOMENCLATURA (Baseado em AAA Games)**

### **Estrutura Padrão**

```
[category]_[action]_[variation]_[direction]

Exemplos:
- idle_combat
- idle_relaxed
- walk_forward
- walk_backward
- run_sprint
- run_combat
- attack_sword_slash
- attack_sword_thrust
- skill_whirlwind_start
- skill_whirlwind_loop
- skill_whirlwind_end
- emote_wave
- death_forward
```

### **Categorias (Prefixos)**

| Categoria | Prefixo | Exemplos |
|-----------|---------|----------|
| **Locomotion** | `idle`, `walk`, `run`, `jump` | `idle_combat`, `walk_forward` |
| **Combat** | `attack`, `block`, `parry`, `dodge` | `attack_sword_slash` |
| **Skills** | `skill`, `cast`, `channel` | `skill_fireball_cast` |
| **Emotes** | `emote`, `gesture` | `emote_wave`, `emote_sit` |
| **States** | `stun`, `death`, `knockback` | `death_forward`, `stun_loop` |
| **Transitions** | `start`, `loop`, `end` | `run_start`, `run_loop`, `run_end` |

### **Direções (Sufixos)**

```
_forward   (fwd)
_backward  (bwd)
_left      (l)
_right     (r)
_up        (climb, jump)
_down      (fall, crouch)
```

---

## 🛠️ **IMPLEMENTAÇÃO: Asset Dictionary v2.0**

### **Estrutura Estendida**

```json
{
  "version": "2.0",
  "assets": [
    {
      "id": 1001,
      "name": "character_male_warrior.glb",
      "type": "CharacterModel",
      "path": "assets/models/characters/warrior_male.glb",
      "description": "Modelo base guerreiro masculino (mesh + skeleton + animations)",
      
      "skeleton": {
        "root_bone": "root",
        "bone_count": 65
      },
      
      "animations": {
        "idle": {
          "source_name": "idle_combat",
          "duration": 2.0,
          "looping": true,
          "blend_in": 0.2,
          "blend_out": 0.2,
          "priority": 0,
          "tags": ["locomotion", "combat_ready"]
        },
        "walk": {
          "source_name": "walk_forward",
          "duration": 0.53,
          "looping": true,
          "blend_in": 0.2,
          "blend_out": 0.2,
          "priority": 1,
          "movement_speed": 5.5,
          "playback_speed": 1.33,
          "tags": ["locomotion", "movement"]
        },
        "run": {
          "source_name": "run_sprint",
          "duration": 0.4,
          "looping": true,
          "blend_in": 0.15,
          "blend_out": 0.15,
          "priority": 1,
          "movement_speed": 8.0,
          "playback_speed": 1.0,
          "tags": ["locomotion", "movement"]
        },
        "attack_1": {
          "source_name": "attack_sword_slash",
          "duration": 0.6,
          "looping": false,
          "blend_in": 0.05,
          "blend_out": 0.1,
          "priority": 10,
          "interrupt_window": [0.3, 0.6],
          "damage_frame": 0.25,
          "tags": ["combat", "melee", "sword"]
        },
        "skill_whirlwind": {
          "source_name": "skill_whirlwind_loop",
          "duration": 1.5,
          "looping": true,
          "blend_in": 0.1,
          "blend_out": 0.2,
          "priority": 15,
          "tags": ["combat", "skill", "aoe"]
        }
      },
      
      "metadata": {
        "author": "ArtTeam",
        "date_created": "2025-11-10",
        "blender_version": "4.0",
        "exported_fps": 30
      }
    }
  ]
}
```

### **Campos Importantes**

| Campo | Tipo | Descrição |
|-------|------|-----------|
| `source_name` | string | Nome exato da Action no Blender NLA |
| `duration` | float | Duração em segundos (para validação) |
| `looping` | bool | Se animação loopa ou é one-shot |
| `blend_in/out` | float | Tempo de transição (usado pelo playAnimation()) |
| `priority` | int | Para resolver conflitos (attack > walk) |
| `movement_speed` | float | Velocidade de movimento ideal (m/s) |
| `playback_speed` | float | Multiplicador de velocidade padrão |
| `tags` | array | Para busca/filtro no editor |

---

## 🎮 **COMO FUNCIONA NO ENGINE**

### **1. Asset Loading (AssetManager)**

```cpp
// engine/asset/asset_manager.cpp
std::shared_ptr<Model> AssetManager::loadCharacterModel(uint32_t assetID)
{
    // 1. Carregar GLB via GLTFLoader (como já faz)
    auto model = GLTFLoader::loadGLTF("warrior_male.glb");
    
    // 2. Carregar metadata do asset_dictionary.json
    auto assetMeta = m_assetDictionary.getAsset(assetID);
    
    // 3. Criar mapeamento: engine name → source name
    for (const auto& [engineName, animMeta] : assetMeta.animations) {
        std::string sourceName = animMeta.source_name; // "idle_combat"
        uint32_t hash = hashString(engineName); // hash("idle")
        
        // 4. Buscar animação no GLB pelo source_name
        AnimationAsset* anim = model->findAnimationByName(sourceName);
        
        if (anim) {
            // 5. Armazenar metadados extras
            anim->metadata.blendInTime = animMeta.blend_in;
            anim->metadata.blendOutTime = animMeta.blend_out;
            anim->metadata.priority = animMeta.priority;
            anim->metadata.movementSpeed = animMeta.movement_speed;
            anim->metadata.defaultPlaybackSpeed = animMeta.playback_speed;
            
            // 6. Registrar no mapa hash → animation
            model->registerAnimation(hash, anim);
        } else {
            Log::Error(std::format(
                "Animation '{}' (source: '{}') not found in GLB!",
                engineName, sourceName));
        }
    }
    
    return model;
}
```

### **2. Usage in Game Code**

```cpp
// src/app/player_controller.cpp
void PlayerController::updateAnimation(float dt)
{
    auto& animComp = world.getComponent<AnimationComponent>(playerID);
    auto& movement = world.getComponent<Movement>(playerID);
    
    // Engine usa nomes genéricos ("idle", "walk")
    // Asset Dictionary mapeia para "idle_combat", "walk_forward"
    
    if (movement.velocity.length() < 0.1f) {
        // Trocar para idle
        uint32_t idleHash = hashString("idle"); // Compile-time com constexpr
        if (animComp.currentAnimationID != idleHash) {
            animSystem.playAnimation(animComp, idleHash, 0.2f);
        }
    }
    else if (movement.velocity.length() < 7.0f) {
        // Walk
        uint32_t walkHash = hashString("walk");
        if (animComp.currentAnimationID != walkHash) {
            animSystem.playAnimation(animComp, walkHash, 0.2f);
            
            // Aplicar playback speed do metadata (se definido)
            auto* model = assetManager.getModel(animComp.animationAssetID);
            auto* anim = model->getAnimation(walkHash);
            if (anim && anim->metadata.defaultPlaybackSpeed > 0.0f) {
                animComp.playbackSpeed = anim->metadata.defaultPlaybackSpeed;
            }
        }
    }
    else {
        // Run
        uint32_t runHash = hashString("run");
        if (animComp.currentAnimationID != runHash) {
            animSystem.playAnimation(animComp, runHash, 0.15f);
        }
    }
}
```

---

## 🖥️ **EDITOR VISUAL (Futuro)**

### **Interface Proposta**

```
┌────────────────────────────────────────────────────┐
│  Asset Inspector: character_male_warrior.glb       │
├────────────────────────────────────────────────────┤
│                                                    │
│  [3D Preview]                                      │
│  ┌──────────────────────┐                          │
│  │                      │  ◄── Live preview        │
│  │     T-Pose Model     │                          │
│  │                      │                          │
│  └──────────────────────┘                          │
│                                                    │
│  Animation List:                                   │
│  ┌──────────────────────────────────────────────┐  │
│  │ Engine Name  │ Source Name       │ Duration │  │
│  ├──────────────┼───────────────────┼──────────┤  │
│  │ ► idle       │ idle_combat       │ 2.00s    │  │ ← Play button
│  │   walk       │ walk_forward      │ 0.53s    │  │
│  │   run        │ run_sprint        │ 0.40s    │  │
│  │   attack_1   │ attack_sword_slash│ 0.60s    │  │
│  │   ...        │ ...               │ ...      │  │
│  └──────────────────────────────────────────────┘  │
│                                                    │
│  [Add Mapping] [Remove] [Import from Blender]     │
│                                                    │
│  Selected: idle                                    │
│  ┌────────────────────────────────────────────┐   │
│  │ Source Name: idle_combat                   │   │
│  │ Duration: 2.0s                             │   │
│  │ Looping: ☑                                 │   │
│  │ Blend In: [0.2] Blend Out: [0.2]          │   │
│  │ Priority: [0]                              │   │
│  │ Movement Speed: [0.0] (static)             │   │
│  │ Playback Speed: [1.0]                      │   │
│  │ Tags: [locomotion] [combat_ready]          │   │
│  └────────────────────────────────────────────┘   │
│                                                    │
│  [Save to Dictionary]  [Export Config]            │
└────────────────────────────────────────────────────┘
```

### **Features do Editor**

1. **Auto-detect**: Scan GLB, listar todas as animations encontradas
2. **Drag-and-drop**: Mapear source_name → engine_name
3. **Live preview**: Reproduzir animação com playback speed ajustável
4. **Validation**: Avisar se source_name não existe no GLB
5. **Batch import**: Importar vários GLBs, aplicar naming convention automaticamente

---

## 📋 **CHECKLIST: Implementação Passo-a-Passo**

### **Fase 1: Estrutura de Dados (Agora)**

- [ ] Adicionar `AnimationMetadata` struct em `animation.h`
- [ ] Estender `asset_dictionary.json` com seção `animations`
- [ ] Implementar `AssetDictionary::loadAnimationMappings()`
- [ ] Adicionar `Model::findAnimationByName(string)` helper

### **Fase 2: Runtime Loading (Próxima)**

- [ ] Modificar `GLTFLoader` para ler animation names do GLB
- [ ] Criar mapeamento hash(engine_name) → AnimationAsset*
- [ ] Aplicar metadata (blend times, playback speed) no load
- [ ] Adicionar validação: warn se source_name não encontrado

### **Fase 3: Editor Visual (Futuro)**

- [ ] UI: Asset Inspector com lista de animações
- [ ] 3D Preview com playback controls
- [ ] Drag-and-drop para mapear animações
- [ ] Export/Import de asset_dictionary.json
- [ ] Auto-complete de nomes baseado em convenção

---

## 🎨 **BLENDER: Boas Práticas**

### **Naming Convention no NLA**

```
NLA Editor:
├─ idle_combat          (Action 1)
├─ idle_relaxed         (Action 2)
├─ walk_forward         (Action 3)
├─ walk_backward        (Action 4)
├─ run_sprint           (Action 5)
├─ attack_sword_slash   (Action 6)
├─ attack_sword_thrust  (Action 7)
├─ skill_whirlwind_loop (Action 8)
└─ death_forward        (Action 9)
```

**IMPORTANTE**:
- ✅ **Lowercase + underscores** (ex: `idle_combat`, não `IdleCombat`)
- ✅ **Descritivo** (ex: `attack_sword_slash`, não `atk1`)
- ✅ **Consistente** (sempre `_forward`, não misturar `_fwd`)
- ✅ **Sem espaços** (GLTF não gosta: `idle combat` → erro)

### **Exportar GLB com Todas as Actions**

```
Blender Export Settings:
┌────────────────────────────┐
│ ☑ Animation                │
│   ☑ Export All Actions     │ ← CRITICAL!
│   ☐ NLA Strips             │
│   ☑ Always Sample          │
│   Sampling Rate: 30        │
│ ☑ Skinning                 │
│   ☑ Include All Bones      │
└────────────────────────────┘
```

Se desmarcar "Export All Actions", apenas a action ATIVA será exportada!

---

## 🔍 **DEBUGGING: Listar Animações do GLB**

```cpp
// engine/asset/gltf_loader.cpp
void GLTFLoader::debugPrintAnimations(const cgltf_data* data)
{
    Log::Info(std::format("GLB contains {} animations:", data->animations_count));
    
    for (size_t i = 0; i < data->animations_count; ++i) {
        const cgltf_animation& anim = data->animations[i];
        std::string name = anim.name ? anim.name : "<unnamed>";
        
        Log::Info(std::format(
            "  [{}] '{}' - {} channels",
            i, name, anim.channels_count));
    }
}
```

**Output esperado**:
```
GLB contains 9 animations:
  [0] 'idle_combat' - 65 channels
  [1] 'walk_forward' - 65 channels
  [2] 'run_sprint' - 65 channels
  [3] 'attack_sword_slash' - 65 channels
  ...
```

---

## 📚 **COMPARAÇÃO: Como Outras Engines Fazem**

### **Unreal Engine**

```cpp
// Animation Blueprint
UPROPERTY(EditAnywhere, Category = "Animations")
UAnimSequence* IdleAnim;  // Assigned in editor

// Asset path: /Game/Characters/Warrior/Anims/idle_combat
```

**Mapeamento**: Editor visual (drag-and-drop de .uasset)

### **Unity**

```csharp
// Animator Controller
public AnimationClip idleClip; // Drag GLB clip in inspector

// Runtime
animator.Play("idle"); // String name (slow)
// OR
int idleHash = Animator.StringToHash("idle");
animator.Play(idleHash); // Hash (fast)
```

**Mapeamento**: Inspector (drag clip) + Animator State Machine

### **Godot**

```gdscript
# AnimationPlayer node
$AnimationPlayer.play("idle_combat")

# GLB import creates AnimationLibrary with all actions
```

**Mapeamento**: Automático (importa todos os clips do GLB)

### **Nossa Engine (Proposta)**

```cpp
// Compile-time hash
constexpr uint32_t ANIM_IDLE = hashString("idle");

// Runtime
animSystem.playAnimation(animComp, ANIM_IDLE, 0.2f);
```

**Mapeamento**: asset_dictionary.json + runtime hash table

---

## ✅ **RECOMENDAÇÃO FINAL**

### **Para Seu Engine MMORPG**

1. **Um GLB por "Character Class"**:
   ```
   warrior_male.glb     (15-20 animações)
   mage_female.glb      (15-20 animações)
   archer_male.glb      (15-20 animações)
   ```

2. **Asset Dictionary como "Source of Truth"**:
   ```json
   {
     "animations": {
       "idle": "idle_combat",      ← Engine code usa "idle"
       "walk": "walk_forward",     ← Blender tem "walk_forward"
       "attack_1": "attack_sword"
     }
   }
   ```

3. **Código usa nomes genéricos**:
   ```cpp
   playAnimation(animComp, hashString("idle"), 0.2f);
   // Asset Dictionary resolve: idle → idle_combat
   ```

4. **Blender usa nomes descritivos**:
   ```
   idle_combat          (não apenas "idle")
   walk_forward         (não apenas "walk")
   attack_sword_slash   (descritivo!)
   ```

**Vantagens**:
- ✅ Código desacoplado de nomes específicos do Blender
- ✅ Fácil trocar assets (outro artista, outra nomenclatura)
- ✅ Editor visual pode editar dictionary sem recompilar
- ✅ Validação automática (warn se animation não existe)
- ✅ Metadata rico (blend times, priorities, tags)

---

## 🚀 **PRÓXIMOS PASSOS**

1. ✅ **Documentação** (este arquivo)
2. ⏳ **Implementar AnimationMetadata struct**
3. ⏳ **Estender asset_dictionary.json**
4. ⏳ **Adicionar Model::findAnimationByName()**
5. ⏳ **Testar com GLB contendo múltiplas animations**
6. ⏳ **Criar helper: debugPrintAnimations()**
7. 🔮 **Editor visual** (Fase 3, quando tiver ImGui avançado)

---

**Autor**: GitHub Copilot  
**Data**: 10 de Novembro de 2025  
**Status**: 📝 Design Document (padrão definido, aguardando implementação)
