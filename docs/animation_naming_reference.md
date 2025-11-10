# Convenção de Nomenclatura - Referência Rápida

## 🎯 **PADRÃO RECOMENDADO**

### **Formato**
```
[category]_[action]_[variation]_[direction]
```

---

## 📋 **LISTA DE NOMES PADRÃO (MMORPG)**

### **🚶 Locomotion (Movimento)**
```
idle                    - Parado, alerta
idle_combat            - Parado, pronto para combate
idle_relaxed           - Parado, relaxado (fora de combate)
idle_crouch            - Agachado parado

walk_forward           - Andar pra frente (5.5 m/s)
walk_backward          - Andar pra trás
walk_left              - Andar lateral esquerda
walk_right             - Andar lateral direita

run_forward            - Correr (8.0 m/s)
run_sprint             - Sprint (11.0 m/s)
run_combat             - Correr em posição de combate

jump_start             - Início do pulo
jump_loop              - No ar (loop)
jump_land              - Aterrissagem

fall_loop              - Caindo (loop)
fall_land              - Aterrissar após queda
```

### **⚔️ Combat (Combate)**
```
attack_1               - Ataque básico #1
attack_2               - Ataque básico #2
attack_3               - Ataque básico #3 (combo)

attack_sword_slash     - Espada: corte horizontal
attack_sword_thrust    - Espada: estocada
attack_sword_overhead  - Espada: ataque de cima

attack_bow_draw        - Arco: puxar corda
attack_bow_hold        - Arco: segurar (aim)
attack_bow_release     - Arco: soltar flecha

block_start            - Começar a bloquear
block_loop             - Bloqueando (loop)
block_end              - Terminar bloqueio
block_impact           - Recebeu ataque (bloqueado)

parry                  - Aparar
dodge_left             - Esquiva esquerda
dodge_right            - Esquiva direita
dodge_backward         - Esquiva pra trás
dodge_roll             - Rolamento
```

### **🔮 Skills (Habilidades)**
```
cast_start             - Começar a castar
cast_loop              - Castando (channeling)
cast_release           - Soltar spell

skill_fireball_cast    - Lançar bola de fogo
skill_heal_cast        - Curar
skill_buff_cast        - Aplicar buff

skill_whirlwind_start  - Redemoinho: início
skill_whirlwind_loop   - Redemoinho: girando (loop)
skill_whirlwind_end    - Redemoinho: fim

skill_dash             - Dash/teleport rápido
skill_ground_slam      - Bater no chão (AOE)
```

### **😊 Emotes**
```
emote_wave             - Acenar
emote_clap             - Aplaudir
emote_dance            - Dançar
emote_sit              - Sentar
emote_laugh            - Rir
emote_cry              - Chorar
emote_point            - Apontar
emote_salute           - Saudar
emote_sleep            - Dormir
```

### **💀 States (Estados)**
```
stun_start             - Começar atordoado
stun_loop              - Atordoado (loop)
stun_end               - Sair do atordoamento

death_forward          - Morte caindo pra frente
death_backward         - Morte caindo pra trás
death_explosion        - Morte explosiva (desmaterializar)

knockback_light        - Empurrão leve
knockback_heavy        - Empurrão forte
knockdown              - Derrubado no chão
getup                  - Levantar do chão

wounded_loop           - Ferido (loop - idle machucado)
```

### **🏃 Transitions (Transições)**
```
run_start              - Walk → Run
run_stop               - Run → Idle

turn_left_90           - Virar 90° esquerda (in-place)
turn_right_90          - Virar 90° direita
turn_180               - Virar 180° (meia-volta)

equip_weapon           - Equipar arma
unequip_weapon         - Desequipar arma
switch_weapon          - Trocar arma (rápido)
```

### **🛡️ Interact (Interação)**
```
interact_pickup        - Pegar item do chão
interact_open          - Abrir baú/porta
interact_lever         - Puxar alavanca
interact_drink         - Beber poção
interact_eat           - Comer comida
```

---

## 🎨 **BLENDER: Naming no NLA**

### **✅ CORRETO**
```
NLA Editor:
├─ idle_combat          ✅ Lowercase, underscore
├─ walk_forward         ✅ Descritivo
├─ attack_sword_slash   ✅ Categoria_ação_variação
└─ skill_whirlwind_loop ✅ Indica que loopa
```

### **❌ EVITAR**
```
├─ Idle Combat          ❌ Espaços (GLTF não gosta)
├─ IdleCombat           ❌ CamelCase (inconsistente)
├─ idle-combat          ❌ Hífen (prefira underscore)
├─ atk1                 ❌ Abreviado demais
├─ idle                 ❌ Genérico (melhor: idle_combat ou idle_relaxed)
```

---

## 📊 **METADATA (Asset Dictionary)**

### **Exemplo Completo**
```json
{
  "idle_combat": {
    "source_name": "idle_combat",
    "duration": 2.0,
    "looping": true,
    "blend_in": 0.2,
    "blend_out": 0.2,
    "priority": 0,
    "movement_speed": 0.0,
    "playback_speed": 1.0,
    "tags": ["locomotion", "combat_ready"]
  },
  "walk_forward": {
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
  "attack_sword_slash": {
    "source_name": "attack_sword_slash",
    "duration": 0.6,
    "looping": false,
    "blend_in": 0.05,
    "blend_out": 0.1,
    "priority": 10,
    "interrupt_window": [0.3, 0.6],
    "damage_frame": 0.25,
    "tags": ["combat", "melee", "sword"]
  }
}
```

---

## 🔍 **CÓDIGO: Uso no Engine**

### **Definir Constantes (Compile-time)**
```cpp
// src/game/animation_ids.h
namespace AnimID {
    constexpr uint32_t IDLE        = hashString("idle");
    constexpr uint32_t WALK        = hashString("walk");
    constexpr uint32_t RUN         = hashString("run");
    constexpr uint32_t ATTACK_1    = hashString("attack_1");
    constexpr uint32_t SKILL_DASH  = hashString("skill_dash");
    constexpr uint32_t EMOTE_WAVE  = hashString("emote_wave");
    constexpr uint32_t DEATH       = hashString("death_forward");
}
```

### **Usar no Gameplay**
```cpp
// PlayerSystem
if (velocity < 0.1f) {
    animSystem.playAnimation(animComp, AnimID::IDLE, 0.2f);
}
else if (velocity < 7.0f) {
    animSystem.playAnimation(animComp, AnimID::WALK, 0.2f);
}
else {
    animSystem.playAnimation(animComp, AnimID::RUN, 0.15f);
}

// Skill System
if (skillUsed == SkillType::Dash) {
    animSystem.playAnimation(animComp, AnimID::SKILL_DASH, 0.1f);
}

// Emote System
if (emoteTriggered == EmoteType::Wave) {
    animSystem.playAnimation(animComp, AnimID::EMOTE_WAVE, 0.3f);
}
```

---

## ✅ **CHECKLIST: Criar Nova Animação**

### **1. Blender (Criar)**
- [ ] Nomear Action no NLA: `[category]_[action]_[variation]`
- [ ] Exemplo: `attack_sword_slash`
- [ ] Verificar duração (Timeline)
- [ ] Exportar GLB com "Export All Actions" ✅

### **2. Asset Dictionary (Mapear)**
- [ ] Adicionar entry em `asset_dictionary.json`:
  ```json
  "attack_1": {
    "source_name": "attack_sword_slash",
    "duration": 0.6,
    "looping": false,
    "priority": 10
  }
  ```

### **3. Engine Code (Usar)**
- [ ] Adicionar constante em `animation_ids.h`:
  ```cpp
  constexpr uint32_t ATTACK_1 = hashString("attack_1");
  ```
- [ ] Usar no gameplay:
  ```cpp
  animSystem.playAnimation(animComp, AnimID::ATTACK_1, 0.1f);
  ```

### **4. Testar**
- [ ] Verificar log: animação encontrada no GLB
- [ ] Testar transição (blend in/out)
- [ ] Verificar duração correta
- [ ] Confirmar looping (ou one-shot)

---

## 🚀 **PRIORIDADES (Combat System)**

```
Priority 0:   Idle, Walk, Run (locomotion base)
Priority 1-5: Emotes (interrompíveis)
Priority 10:  Attacks (interrompe locomotion)
Priority 15:  Skills (interrompe attacks)
Priority 20:  Stun, Knockback (force override)
Priority 99:  Death (NUNCA interrompido)
```

**Regra**: Animação com priority MAIOR interrompe MENOR.

---

## 📚 **EXEMPLOS DE AAA GAMES**

### **Dark Souls 3**
```
chr_idle_normal
chr_walk_forward
chr_run_forward
chr_attack_sword_R1
chr_roll_forward
chr_death_A
```

### **Monster Hunter World**
```
idle_combat
walk_weapon_drawn
attack_greatsword_charge
skill_true_charge_slash
dodge_roll
```

### **World of Warcraft**
```
Stand (idle)
Walk
Run
Attack1H
Cast
EmoteWave
Death
```

---

**Ver documentação completa**: `docs/animation_naming_convention.md`  
**Status**: ✅ Padrão definido e pronto para uso!
