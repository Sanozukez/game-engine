// engine/animation/animation_config.h

#pragma once
#include <cstdint>

namespace Engine::Animation {

// Flags para controle de debug por categoria
struct AnimationDebugFlags {
    bool showBoneTransforms = false;      // Mostra transformações dos ossos
    bool showKeyframeInfo = false;        // Mostra informações de keyframes
    bool showFKDebug = false;            // Mostra debug do Forward Kinematics
    bool showTimingInfo = false;         // Mostra informações de timing
};

// Configurações de otimização
struct AnimationOptimizationSettings {
    bool enableParallelUpdate = false;    // Habilita atualização paralela de ossos
    bool cacheTransforms = true;          // Cache de transformações
    uint32_t maxBonesPerBatch = 100;      // Máximo de ossos por batch
};

struct AnimationConfig {
    // Flags de teste e comportamento
    bool useRestPoseOnly = false;         // Usa apenas pose de repouso
    bool disableScaleKeys = true;         // Desabilita keys de escala
    
    // Tolerâncias e limites
    static constexpr float SCALE_EPSILON = 1e-4f;
    static constexpr float ROTATION_EPSILON = 1e-5f;
    static constexpr float TIME_EPSILON = 1e-6f;
    
    // Configurações de debug
    AnimationDebugFlags debug;
    
    // Configurações de otimização
    AnimationOptimizationSettings optimization;
    
    // Configurações de validação
    bool validateInputs = true;           // Valida inputs antes de processar
    bool sanitizeTransforms = true;       // Sanitiza transformações
};

} // namespace Engine::Animation