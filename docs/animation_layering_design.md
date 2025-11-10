# Animation Layering System - Design Document

## Visão Geral

Sistema de **Animation Layering** permite executar múltiplas animações simultaneamente em diferentes partes do corpo. Essencial para MMORPGs onde o player precisa **andar enquanto ataca** ou **correr enquanto casta spell**.

**Status**: 📝 Planejado (implementar quando tiver sistema de combat/skills)

---

## Arquitetura

### **Layers (Camadas)**

```
Layer 0: FULL_BODY (prioridade base)
  └─ Idle, Walk, Run, Jump

Layer 1: UPPER_BODY (sobrescreve Layer 0 para bones superiores)
  └─ Attack, Cast_Spell, Drink_Potion, Wave

Layer 2: HEAD (sobrescreve Layers anteriores apenas para cabeça)
  └─ Look_Around, Emote_Nod, Emote_Shake

Layer 3: ADDITIVE (modifica layers anteriores, não substitui)
  └─ Breath, Wounded_Limp, Buff_Glow
```

### **Bone Masks (Máscaras)**

Define quais bones cada layer afeta:

```cpp
struct BoneMask {
    std::string name;
    std::vector<int> affectedBoneIds;
    float weight = 1.0f; // 0.0 = desabilitado, 1.0 = totalmente ativo
};

// Exemplo: Upper Body Mask
BoneMask upperBodyMask {
    .name = "UpperBody",
    .affectedBoneIds = {
        boneIds["Spine-01"],
        boneIds["Spine-02"],
        boneIds["Spine-03"],
        boneIds["Shoulder.L"],
        boneIds["Shoulder.R"],
        boneIds["Neck"],
        boneIds["Head"],
        // ... todos os bones de braços
    },
    .weight = 1.0f
};
```

---

## Estruturas de Dados

### **AnimationLayer**

```cpp
struct AnimationLayer {
    uint32_t layerID;
    std::string name;
    int priority = 0; // Maior prioridade = sobrescreve layers menores
    
    // Animação atual do layer
    uint32_t currentAnimationID = 0;
    float currentTime = 0.0f;
    
    // Blending dentro do layer
    uint32_t previousAnimationID = 0;
    float blendFactor = 1.0f;
    float blendSpeed = 5.0f;
    
    // Bone mask (quais bones este layer afeta)
    BoneMask boneMask;
    
    // Modo de blending
    enum class BlendMode {
        Override,   // Substitui completamente (padrão)
        Additive    // Adiciona à transformação existente
    };
    BlendMode blendMode = BlendMode::Override;
    
    // Controle geral do layer
    bool enabled = true;
    float layerWeight = 1.0f; // 0.0 = desabilitado, 1.0 = totalmente ativo
};
```

### **AnimationComponent (Atualizado)**

```cpp
struct AnimationComponent {
    // Asset reference
    uint32_t animationAssetID = 0;
    
    // NOVO: Múltiplos layers
    std::vector<AnimationLayer> layers;
    
    // Resultado final (já existe)
    std::vector<glm::mat4> finalBoneTransforms;
    
    // Helper methods
    AnimationLayer* getLayer(uint32_t layerID);
    AnimationLayer* getLayerByName(const std::string& name);
};
```

---

## Algoritmo de Blending Multi-Layer

### **Pseudo-código**

```cpp
void AnimationSystem::updateEntityWithLayers(World& world, EntityID entityID, float dt)
{
    auto& animComp = world.getComponent<AnimationComponent>(entityID);
    auto& meshComp = world.getComponent<Mesh>(entityID);
    
    std::shared_ptr<Model> model = assetManager.getModel(meshComp.assetID);
    Skeleton* skeleton = model->getSkeleton();
    
    // 1. Inicializar com rest pose
    std::map<int, glm::mat4> finalLocalTransforms;
    initializeNodeTransforms(skeleton, model, finalLocalTransforms);
    
    // 2. Processar cada layer em ordem de prioridade
    std::sort(animComp.layers.begin(), animComp.layers.end(), 
        [](const AnimationLayer& a, const AnimationLayer& b) {
            return a.priority < b.priority; // Menor primeiro
        });
    
    for (auto& layer : animComp.layers) {
        if (!layer.enabled || layer.layerWeight <= 0.0f) continue;
        
        // Atualizar tempo e blend do layer
        updateLayerAnimation(layer, dt);
        
        // Calcular transforms do layer
        std::map<int, glm::mat4> layerTransforms;
        calculateLayerTransforms(model, layer, layerTransforms);
        
        // Aplicar bone mask e blend mode
        applyLayerToBones(layer, layerTransforms, finalLocalTransforms);
    }
    
    // 3. Forward Kinematics e IBMs (como antes)
    updateBoneTransforms(skeleton, finalLocalTransforms, animComp);
}
```

### **Aplicar Layer com Bone Mask**

```cpp
void AnimationSystem::applyLayerToBones(
    const AnimationLayer& layer,
    const std::map<int, glm::mat4>& layerTransforms,
    std::map<int, glm::mat4>& outFinalTransforms)
{
    for (int boneId : layer.boneMask.affectedBoneIds) {
        if (layerTransforms.find(boneId) == layerTransforms.end()) continue;
        
        const glm::mat4& layerTransform = layerTransforms.at(boneId);
        
        if (layer.blendMode == AnimationLayer::BlendMode::Override) {
            // Interpolar entre transform atual e layer transform
            glm::vec3 currentT, currentS, layerT, layerS;
            glm::quat currentR, layerR;
            
            decomposeTRS(outFinalTransforms[boneId], currentT, currentR, currentS);
            decomposeTRS(layerTransform, layerT, layerR, layerS);
            
            float weight = layer.layerWeight * layer.boneMask.weight;
            
            glm::vec3 blendedT = glm::mix(currentT, layerT, weight);
            glm::quat blendedR = glm::slerp(currentR, layerR, weight);
            glm::vec3 blendedS = glm::mix(currentS, layerS, weight);
            
            outFinalTransforms[boneId] = composeTRS(blendedT, blendedR, blendedS);
            
        } else if (layer.blendMode == AnimationLayer::BlendMode::Additive) {
            // Adicionar transformação (útil para breath, wounded limp)
            glm::vec3 currentT, currentS, additiveT, additiveS;
            glm::quat currentR, additiveR;
            
            decomposeTRS(outFinalTransforms[boneId], currentT, currentR, currentS);
            decomposeTRS(layerTransform, additiveT, additiveR, additiveS);
            
            float weight = layer.layerWeight * layer.boneMask.weight;
            
            // Adicionar (não interpolar)
            glm::vec3 resultT = currentT + (additiveT * weight);
            glm::quat resultR = currentR * glm::slerp(glm::quat(1,0,0,0), additiveR, weight);
            glm::vec3 resultS = currentS * glm::mix(glm::vec3(1), additiveS, weight);
            
            outFinalTransforms[boneId] = composeTRS(resultT, resultR, resultS);
        }
    }
}
```

---

## Exemplos de Uso

### **Exemplo 1: Walk + Attack**

```cpp
// Setup inicial (quando player spawna)
AnimationComponent& animComp = ...;

// Layer 0: Full Body (andar)
AnimationLayer fullBodyLayer;
fullBodyLayer.layerID = 0;
fullBodyLayer.name = "FullBody";
fullBodyLayer.priority = 0;
fullBodyLayer.boneMask = createFullBodyMask(skeleton); // Todos os bones
animComp.layers.push_back(fullBodyLayer);

// Layer 1: Upper Body (atacar)
AnimationLayer upperBodyLayer;
upperBodyLayer.layerID = 1;
upperBodyLayer.name = "UpperBody";
upperBodyLayer.priority = 1; // Sobrescreve FullBody
upperBodyLayer.boneMask = createUpperBodyMask(skeleton); // Apenas torso/braços
upperBodyLayer.enabled = false; // Desabilitado até atacar
animComp.layers.push_back(upperBodyLayer);

// Durante gameplay: Player aperta botão de ataque enquanto anda
auto* fullBody = animComp.getLayer(0);
auto* upperBody = animComp.getLayer(1);

// Manter walk nas pernas
animationSystem.playAnimationOnLayer(fullBody, walkAnimID, 0.2f);

// Ativar attack no upper body
upperBody->enabled = true;
animationSystem.playAnimationOnLayer(upperBody, attackAnimID, 0.1f);

// Resultado: Pernas andando, torso/braços atacando!
```

### **Exemplo 2: Run + Cast Spell + Breath**

```cpp
// Layer 0: Run (corpo inteiro)
animationSystem.playAnimationOnLayer(fullBodyLayer, runAnimID, 0.3f);

// Layer 1: Cast Spell (upper body)
upperBodyLayer->enabled = true;
animationSystem.playAnimationOnLayer(upperBodyLayer, castSpellAnimID, 0.2f);

// Layer 2: Breath (additive, sutil)
AnimationLayer breathLayer;
breathLayer.name = "Breath";
breathLayer.priority = 2;
breathLayer.blendMode = AnimationLayer::BlendMode::Additive;
breathLayer.layerWeight = 0.3f; // Sutil
breathLayer.boneMask = createTorsoMask(skeleton); // Apenas torso
breathLayer.enabled = true;
animComp.layers.push_back(breathLayer);
animationSystem.playAnimationOnLayer(&breathLayer, breathAnimID, 0.5f);

// Resultado: 
// - Pernas correndo (Layer 0)
// - Braços castando spell (Layer 1)
// - Torso respirando suavemente (Layer 2 additive)
```

---

## Bone Masks Pré-definidos (MMORPG Típico)

```cpp
BoneMask createFullBodyMask(Skeleton* skeleton) {
    BoneMask mask;
    mask.name = "FullBody";
    for (const auto& bone : skeleton->bones) {
        mask.affectedBoneIds.push_back(bone.id);
    }
    return mask;
}

BoneMask createUpperBodyMask(Skeleton* skeleton) {
    BoneMask mask;
    mask.name = "UpperBody";
    
    // Spine, shoulders, arms, hands
    std::vector<std::string> upperBones = {
        "Spine-01", "Spine-02", "Spine-03",
        "Shoulder.L", "Shoulder.R",
        "Upper_arm_3.rotation.L", "Upper_arm_3.rotation.R",
        "Forearm.L", "Forearm.R",
        "Hand.L", "Hand.R",
        "Neck", "Head"
        // ... adicionar todos os dedos se necessário
    };
    
    for (const auto& boneName : upperBones) {
        int boneId = skeleton->getBoneIdByName(boneName);
        if (boneId != -1) {
            mask.affectedBoneIds.push_back(boneId);
        }
    }
    
    return mask;
}

BoneMask createLowerBodyMask(Skeleton* skeleton) {
    BoneMask mask;
    mask.name = "LowerBody";
    
    std::vector<std::string> lowerBones = {
        "Hips-Main",
        "Thigh.L", "Thigh.R",
        "Shin.L", "Shin.R",
        "Foot.L", "Foot.R",
        "Toe.L", "Toe.R"
    };
    
    for (const auto& boneName : lowerBones) {
        int boneId = skeleton->getBoneIdByName(boneName);
        if (boneId != -1) {
            mask.affectedBoneIds.push_back(boneId);
        }
    }
    
    return mask;
}

BoneMask createHeadMask(Skeleton* skeleton) {
    BoneMask mask;
    mask.name = "Head";
    
    std::vector<std::string> headBones = {"Neck", "Neck_1", "Head"};
    
    for (const auto& boneName : headBones) {
        int boneId = skeleton->getBoneIdByName(boneName);
        if (boneId != -1) {
            mask.affectedBoneIds.push_back(boneId);
        }
    }
    
    return mask;
}
```

---

## Performance Considerations

### **Otimizações para MMORPG (50 players)**

1. **Cache de Bone Masks**: Criar uma vez, reusar para todos os players
2. **Layer Culling**: Desabilitar layers distantes (LOD)
3. **Layer Pooling**: Reutilizar objetos `AnimationLayer` em vez de criar novos
4. **Sparse Updates**: Atualizar layers menos prioritários em frames alternados
5. **SIMD**: Usar GLM SIMD para interpolações TRS em lote

```cpp
// Exemplo de LOD para layers
void AnimationSystem::updateEntityWithLayers(World& world, EntityID entityID, float dt) {
    float distanceToCamera = calculateDistance(entityID, camera);
    
    // Muito longe: apenas Layer 0 (full body)
    if (distanceToCamera > 50.0f) {
        updateOnlyLayer(entityID, 0, dt);
        return;
    }
    
    // Médio: Layers 0 e 1
    if (distanceToCamera > 20.0f) {
        updateOnlyLayers(entityID, {0, 1}, dt);
        return;
    }
    
    // Perto: Todos os layers
    updateAllLayers(entityID, dt);
}
```

---

## Quando Implementar

✅ **Agora**: Cross-fade blending (IMPLEMENTADO)  
⏳ **Quando tiver Skills**: Animation Layering (upper/lower split)  
⏳ **Quando tiver Combat**: Additive layers (flinch, wounded)  
⏳ **Polish Final**: IK (Inverse Kinematics) para feet placement, look-at

---

## Alternativas (Se Layering for Complexo Demais)

### **Opção B: Baked Animations** (Mais Simples)

Criar animações combinadas no Blender:
- `idle`
- `walk`
- `walk_attack` (pré-combinado)
- `run`
- `run_cast` (pré-combinado)
- `jump`
- `jump_attack` (pré-combinado)

**Vantagens**:
- Zero código extra
- Performance perfeita
- Artista tem controle total

**Desvantagens**:
- **Explosão combinatória**: 10 movimentos × 20 skills = 200 animações!
- Difícil manter consistência
- Muito espaço em disco

### **Recomendação para MMORPG**

**Híbrido**:
1. **Base**: Baked animations para combinações comuns (`walk_attack`, `run_cast`)
2. **Layers**: Usar para situações específicas (emotes, buffs visuais, flinch)
3. **Additive**: Breath, wounded limp, buff glow (sempre additive)

Isso dá **80% dos resultados com 20% da complexidade**!

---

## Referências

- **Unity Animator**: Inspiração para layers e blend trees
- **Unreal Engine Animation Blueprints**: Conceito de blend por bone mask
- **DOOM (2016)**: Additive animations para weapon recoil
- **World of Warcraft**: Baked animations + alguns layers para emotes

---

**Autor**: GitHub Copilot  
**Data**: 10 de Novembro de 2025  
**Status**: 📝 Design Document (não implementado)
