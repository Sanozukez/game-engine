// engine/ecs/systems/animation_system.h

#pragma once
#include "base_system.h"
#include "../../animation/animation_config.h"
#include "../../asset/asset_manager.h"
#include "../../asset/skeleton.h"
#include "../../asset/animation.h"
#include "../components/animation_component.h"
#include "../../animation/keyframe_sampler.h"  // Adicionando include necessário
#include "../world.h"
#include <glm/glm.hpp>
#include <map>

namespace Engine::ECS::System {

class AnimationSystem : public BaseSystem {
public:
    explicit AnimationSystem(Engine::Asset::AssetManager& assetManager);
    void update(World& world, float dt) override;
    
    // Método público para trocar animações com blending
    void playAnimation(Component::AnimationComponent& animComp, uint32_t newAnimationID, float blendDuration = 0.3f);

    // NOVO v101: Trocar animação usando engine_name (ex: "idle", "walk") com metadata do AssetManager
    void playAnimationByName(Component::AnimationComponent& animComp, 
                            const std::shared_ptr<Engine::Asset::Model>& model,
                            const std::string& engineName);

private:
    Engine::Asset::AssetManager& m_assetManager;
    Engine::Animation::AnimationConfig m_config;

    void updateEntityAnimation(World& world, EntityID entityID, float dt);

    void updateAnimationTime(
        Component::AnimationComponent& animComp,
        const Engine::Asset::AnimationAsset* currentAnim,
        float dt);

    void initializeNodeTransforms(
        const Engine::Skeleton* skeleton,
        const std::shared_ptr<Engine::Asset::Model>& model,
        std::map<int, glm::mat4>& outLocalTransforms);

    void applyAnimationChannel(
        const Engine::Asset::AnimationChannel& channel,
        const glm::mat4& nodeTransform,
        float currentTime,
        glm::mat4& outLocalTransform);

    void updateBoneTransforms(
        const Engine::Skeleton* skeleton,
        const std::map<int, glm::mat4>& localTransforms,
        Component::AnimationComponent& animComp);
    
    // Blending entre animações
    void updateBlendFactor(Component::AnimationComponent& animComp, float dt);
    
    void blendAnimations(
        const Engine::Skeleton* skeleton,
        const std::shared_ptr<Engine::Asset::Model>& model,
        Component::AnimationComponent& animComp,
        const Engine::Asset::AnimationAsset* currentAnim,
        const Engine::Asset::AnimationAsset* previousAnim,
        std::map<int, glm::mat4>& outBlendedTransforms);

    bool validateAnimationInputs(
        const Engine::Skeleton* skeleton,
        const Engine::Asset::AnimationAsset* currentAnim) const;
};

} // namespace Engine::ECS::System