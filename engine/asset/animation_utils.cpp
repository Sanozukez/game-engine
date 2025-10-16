// // engine/asset/animation_utils.cpp

#include "animation_utils.h"
#include "asset_manager.h"
#include "../core/log.h"
#include "../ecs/components/animation_component.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <format>

using namespace Engine::Asset;
using namespace Engine::ECS::Component;

// ----------------------------------------------------------------------
// 1. Implementação do Hash ID
// ----------------------------------------------------------------------
uint32_t AnimationUtils::getAnimationHashID(const std::string &name)
{
    return AssetManager::getAssetIDByName(name);
}

// ----------------------------------------------------------------------
// FUNÇÃO AUXILIAR: LERP para glm::mat4 (RESOLVE O ERRO C2440)
// ----------------------------------------------------------------------
// GLM não consegue interpolar glm::mat4 diretamente, então fazemos manualmente
// componente por componente, usando a função mix para glm::vec4 (que é o tipo interno).
static glm::mat4 lerp(const glm::mat4& x, const glm::mat4& y, float a) {
    // Interpolação coluna por coluna
    return glm::mat4(
        glm::mix(x[0], y[0], a),
        glm::mix(x[1], y[1], a),
        glm::mix(x[2], y[2], a),
        glm::mix(x[3], y[3], a)
    );
}

// NOVO: Declaração antecipada com os novos 4 argumentos de animação
static void readNodeHierarchy(
    const std::shared_ptr<Model> &model,
    uint32_t currentClipID,
    uint32_t previousClipID,
    float currentTime,
    float blendFactor,
    std::vector<glm::mat4> &finalBoneTransforms,
    const std::string &nodeName,
    const glm::mat4 &parentTransform);

// ----------------------------------------------------------------------
// FUNÇÕES AUXILIARES DE ANIMAÇÃO (DECLARAÇÕES ANTECIPADAS)
// ----------------------------------------------------------------------
// É essencial que a assinatura de 'calculateBoneTransform' esteja visível
// antes de 'readNodeHierarchy' usá-la.

static glm::mat4 calculateBoneTransform( // <--- ADICIONE ESTA DECLARAÇÃO
    uint32_t currentClipID,
    const std::string &boneName,
    float currentTime,
    const std::shared_ptr<Model> &model);

// ----------------------------------------------------------------------
// FUNÇÃO AUXILIAR: Traversa a Hierarquia de Nodos (Cinemática Forward)
// ----------------------------------------------------------------------
// Esta função é a única responsável por calcular a Transformação Global de cada osso.
static void readNodeHierarchy(
    const std::shared_ptr<Model> &model,
    uint32_t currentClipID,
    uint32_t previousClipID, // <--- NOVO: ID do clipe anterior
    float currentTime,
    float blendFactor, // <--- NOVO: Fator de 0.0 a 1.0
    std::vector<glm::mat4> &finalBoneTransforms,
    const std::string &nodeName,
    const glm::mat4 &parentTransform)
{
    // A lógica original de transformação local é o Identity/Transform estático.
    glm::mat4 nodeTransform = model->getNodeLocalTransform(nodeName);

    // NOVO: 1. Cálculo da Pose Animada Local
    glm::mat4 pose1 = glm::mat4(1.0f);
    glm::mat4 pose2 = glm::mat4(1.0f);

    // 2. Por enquanto, apenas calculamos a pose local do clipe atual:
    if (currentClipID != 0) // Assume 0 é ID inválido
    {
        // Usa a lógica de interpolação implementada na última etapa
        glm::mat4 animatedLocalTransform = calculateBoneTransform(
            currentClipID,
            nodeName,
            currentTime,
            model);
        nodeTransform = animatedLocalTransform; // Aplica a pose animada

        // Futuro: Blend de poses (nodeTransform = glm::mix(pose1, pose2, blendFactor);)
    }

    // LÓGICA DE BLEND: Se estiver em transição
    if (previousClipID != 0 && blendFactor < 1.0f)
    {
        pose2 = calculateBoneTransform(previousClipID, nodeName, currentTime, model);

        // Interpola a matriz: MatrizFinal = (pose1 * blendFactor) + (pose2 * (1 - blendFactor))
        // Nota: A interpolação de matrizes (LERP) não é ideal, mas é o suficiente para o shell de blend.
        // O ideal seria interpolar T, R, S individualmente.
        nodeTransform = lerp(pose2, pose1, blendFactor);
    }

    // 2. Calcula a Transformação Global: Global = Pai Global * Local
    glm::mat4 globalTransform = parentTransform * nodeTransform;

    // 3. Verifica se o nodo é um osso (BoneFinal = Global * BoneOffset)
    int boneIndex = model->getBoneIndexByName(nodeName);
    if (boneIndex != -1)
    {
        const auto &boneInfoMap = model->getBoneInfoMap();
        if (boneInfoMap.count(nodeName))
        {
            const BoneInfo &boneInfo = boneInfoMap.at(nodeName);
            finalBoneTransforms[boneInfo.id] = globalTransform * boneInfo.offset;
        }
    }

    // 4. Continua recursivamente para os filhos
    const std::vector<std::string> children = model->getNodeChildren(nodeName);
    for (const auto &childName : children)
    {
        readNodeHierarchy(model, currentClipID, previousClipID, currentTime, blendFactor, finalBoneTransforms, childName, globalTransform);
    }
}

// ----------------------------------------------------------------------
// FUNÇÃO PRINCIPAL: calculateBoneTransforms (SRP da Amostragem)
// ----------------------------------------------------------------------
void AnimationUtils::calculateBoneTransforms(
    const std::shared_ptr<Model> &model,
    uint32_t currentAnimationID,
    uint32_t previousAnimationID,
    float currentTime,
    float blendFactor,
    std::vector<glm::mat4> &finalBoneTransforms)
{
    if (!model || model->getSkeletonRootName().empty())
    {
        return;
    }

    // Garante que o vetor de saída tenha o tamanho máximo (necessário para o shader)
    if (finalBoneTransforms.size() != Animation::MAX_BONES)
    {
        finalBoneTransforms.resize(Animation::MAX_BONES, glm::mat4(1.0f));
    }

    // ----------------------------------------------------------------------
    // LÓGICA DE BLEND E INTERPOLAÇÃO (PLACEHOLDER REAL)
    // A lógica real de amostragem de Keyframe e Blend de Poses entraria aqui.
    // Por enquanto, confiamos que os métodos getNodeLocalTransform e getNodeChildren
    // retornam transformações básicas, permitindo que a hierarquia seja percorrida.
    // ----------------------------------------------------------------------

    // 2. Inicia a Travessia da Hierarquia
    readNodeHierarchy(
        model,
        currentAnimationID,
        previousAnimationID, // <--- NOVO
        currentTime,
        blendFactor, // <--- NOVO
        finalBoneTransforms,
        model->getSkeletonRootName(),
        glm::mat4(1.0f));
}

// ----------------------------------------------------------------------
// FUNÇÕES AUXILIARES DE ANIMAÇÃO (SRP: Cálculo de Pose)
// ----------------------------------------------------------------------

// Encontra o par de keyframes (anterior e próximo) para um determinado tempo.
template <typename T>
static bool findKeyframePair(float time, const std::vector<KeyFrame<T>> &keys, int &prevIndex, int &nextIndex, float &factor) // <-- CORREÇÃO: Removido 'Model::'
{
    if (keys.empty())
        return false;
    if (keys.size() == 1)
    {
        prevIndex = nextIndex = 0;
        factor = 0.0f;
        return true;
    }

    // Busca binária ou linear para encontrar o frame
    for (size_t i = 0; i < keys.size() - 1; ++i)
    {
        if (time >= keys[i].time && time <= keys[i + 1].time)
        {
            prevIndex = static_cast<int>(i);
            nextIndex = static_cast<int>(i + 1);

            float totalTime = keys[nextIndex].time - keys[prevIndex].time;
            if (totalTime > 0.0f)
            {
                // Fator de interpolação (0.0 a 1.0)
                factor = (time - keys[prevIndex].time) / totalTime;
            }
            else
            {
                factor = 0.0f;
            }
            return true;
        }
    }

    // Se o tempo for maior que a duração, usa o último frame
    if (time >= keys.back().time)
    {
        prevIndex = nextIndex = static_cast<int>(keys.size() - 1);
        factor = 0.0f;
    }

    return true;
}

// Calcula o valor interpolado para um osso em um dado tempo
static glm::mat4 calculateBoneTransform(
    uint32_t currentClipID,
    const std::string &boneName,
    float currentTime,
    const std::shared_ptr<Model> &model)
{
    // Assume-se que o clipe e o canal existem (validação feita no loop principal)
    const AnimationClip *clip = model->getAnimationClip(currentClipID);
    if (!clip || !clip->boneChannels.count(boneName))
    {
        return glm::mat4(1.0f); // Retorna Identity se o canal não for encontrado
    }

    const BoneChannel &channel = clip->boneChannels.at(boneName);

    // NOTA: Para ciclos, o tempo deve ser ajustado: currentTime = fmod(currentTime, clip->duration);
    float animTime = fmod(currentTime, clip->duration);

    int prev, next;
    float factor;

    // Posição (Translation)
    glm::vec3 position = glm::vec3(0.0f);
    if (findKeyframePair(animTime, channel.positionKeys, prev, next, factor))
    {
        position = glm::mix(channel.positionKeys[prev].value, channel.positionKeys[next].value, factor);
    }

    // Rotação (Rotation - SLERP)
    Engine::Math::Quat rotation;
    if (findKeyframePair(animTime, channel.rotationKeys, prev, next, factor))
    {
        rotation = Engine::Math::Quat::slerp(channel.rotationKeys[prev].value, channel.rotationKeys[next].value, factor);
    }
    else
    {
        rotation = Engine::Math::Quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity
    }

    // Escala (Scale)
    glm::vec3 scale = glm::vec3(1.0f);
    if (findKeyframePair(animTime, channel.scaleKeys, prev, next, factor))
    {
        scale = glm::mix(channel.scaleKeys[prev].value, channel.scaleKeys[next].value, factor);
    }

    // Constrói a Matriz Local: Translação * Rotação * Escala
    glm::mat4 localTransform = glm::mat4(1.0f);
    localTransform = glm::translate(localTransform, position);
    localTransform = localTransform * rotation.toMat4();
    localTransform = glm::scale(localTransform, scale);

    return localTransform;
}