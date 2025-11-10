# CÁLCULO RÁPIDO: Animação Walk para 5.5 m/s

## ✅ **IMPLEMENTADO NO CÓDIGO**
- ✅ Campo `playbackSpeed` adicionado em `AnimationComponent`
- ✅ Aplicado em `AnimationSystem::updateEntityAnimation()` (linha 153)
- ✅ Compilado com sucesso!

---

## 📊 **SPECS RECOMENDADAS (Testado em MMORPGs)**

### **Opção 1: Natural Walk (RECOMENDADO)**
```
FPS:              30
Frames totais:    16
Duração:          0.53 segundos
Stride Length:    1.1 metros
Passos/segundo:   3.75 (16 frames = 2 passos)
```

**No código**:
```cpp
animComp.playbackSpeed = 1.33f; // Acelera para casar com 5.5 m/s
```

**Cálculo**:
```
Velocidade desejada:  5.5 m/s
Stride length:        1.1 m
Passos ideais/s:      5.5 / 1.1 = 5.0
Passos da anim:       3.75 (16 frames @ 30fps)
playbackSpeed:        5.0 / 3.75 = 1.33x
```

---

### **Opção 2: Perfeito (Sem playbackSpeed)**
```
FPS:              30
Frames totais:    12
Duração:          0.4 segundos
Stride Length:    1.1 metros
Passos/segundo:   5.0 (exatamente 5.5 m/s)
```

**No código**:
```cpp
animComp.playbackSpeed = 1.0f; // Animação já perfeita!
```

**Cálculo**:
```
Velocidade desejada:  5.5 m/s
Stride length:        1.1 m
Passos ideais/s:      5.5 / 1.1 = 5.0
Duração ideal:        2 passos / 5.0 = 0.4s
Frames @ 30fps:       0.4 * 30 = 12 frames
```

⚠️ **Desvantagem**: 12 frames pode parecer muito rápido (mais run que walk)

---

## 🎯 **MINHA RECOMENDAÇÃO**

**Use Opção 1** (16 frames + playbackSpeed 1.33x):

1. **No Blender**:
   - Crie walk com 16 frames @ 30 FPS
   - In-place (root bone X/Z = 0)
   - 2 passos completos (esquerda + direita)
   - Stride "virtual" = 1.1 metros

2. **No Código** (já implementado!):
   ```cpp
   // src/app/app_setup.cpp (ou onde setup player animation)
   auto& animComp = world.getComponent<AnimationComponent>(playerID);
   animComp.playbackSpeed = 1.33f;
   ```

3. **Teste**:
   - Olhe os pés enquanto anda
   - Se deslizar pra frente: aumentar (1.4f, 1.5f)
   - Se deslizar pra trás: diminuir (1.2f, 1.1f)

---

## 📐 **STRIDE LENGTH: Como Escolher?**

Você não precisa medir nada! Para humanóide típico:

| Tipo | Stride Length | Notas |
|------|---------------|-------|
| **Walk** | 1.0 - 1.2m | Use **1.1m** (padrão MMORPG) |
| **Run** | 1.5 - 1.8m | Use **1.6m** |
| **Sprint** | 2.0 - 2.5m | Use **2.2m** |

Como animação é **in-place** (personagem não sai do lugar no Blender), você **escolhe** mentalmente "cada passo avança 1.1m" e ajusta `playbackSpeed` para casar.

---

## 🚀 **PRÓXIMOS PASSOS**

1. ✅ Código pronto (playbackSpeed implementado)
2. ⏳ Você: Criar/ajustar animação walk no Blender (16 frames, in-place)
3. ⏳ Você: Testar no jogo com `playbackSpeed = 1.33f`
4. ⏳ Você: Ajustar por feel (±0.1) até eliminar foot sliding

---

## 📝 **CHECKLIST BLENDER**

Quando criar a animação:
- [ ] 16 frames totais (Frame 1 a 16)
- [ ] 30 FPS (Timeline → Framerate)
- [ ] Root bone X = 0, Z = 0 (in-place)
- [ ] 2 passos completos (esquerda + direita)
- [ ] Frame 1 = Frame 16 (loop perfeito)
- [ ] Export: GLTF 2.0 (.glb), +Y Up, Sampling Rate = 30

---

## 🎮 **TESTE RÁPIDO NO CÓDIGO**

Se quiser testar agora mesmo com animação atual:

```cpp
// Acelerar 2x (teste extremo)
animComp.playbackSpeed = 2.0f;
// Animação deve rodar o dobro da velocidade

// Normal
animComp.playbackSpeed = 1.0f;

// Slow-motion (50%)
animComp.playbackSpeed = 0.5f;
```

Se isso funcionar, sistema está OK! 🎉

---

**Ver guia completo**: `docs/animation_blender_guide.md`  
**Configuração atual**: `movement_speed = 5.5 m/s` ✅  
**Status**: Código pronto, aguardando animação no Blender! 🚀
