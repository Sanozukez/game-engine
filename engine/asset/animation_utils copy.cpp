// // engine/asset/animation_utils.cpp

#include "animation_utils.h"
#include "asset_manager.h"
#include "../core/log.h"
#include "../ecs/components/animation_component.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <format>
#include <algorithm> // NOVO: Necessário para std::transform
#include <cctype>    // NOVO: Necessário para std::tolower

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
// IMPLEMENTAÇÃO DE IsRootBoneName (REGRA DE NEGÓCIO DO ROOT BONE)
// ----------------------------------------------------------------------
bool AnimationUtils::IsRootBoneName(const std::string &bone_name)
{
    if (bone_name.empty())
    {
        return false;
    }

    // 1. Converter o nome para minúsculas para pesquisa case-insensitive (como você pediu).
    std::string lower_name = bone_name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](unsigned char c)
                   { return static_cast<char>(std::tolower(c)); });

    // 2. Verificar se contém a substring "root".
    return lower_name.find("root") != std::string::npos;
}

// ----------------------------------------------------------------------
// FUNÇÃO AUXILIAR: LERP para glm::mat4
// ----------------------------------------------------------------------
// GLM não consegue interpolar glm::mat4 diretamente, então fazemos manualmente
// componente por componente, usando a função mix para glm::vec4 (que é o tipo interno).
static glm::mat4 lerp(const glm::mat4 &x, const glm::mat4 &y, float a)
{
    // Interpolação coluna por coluna
    return glm::mat4(
        glm::mix(x[0], y[0], a),
        glm::mix(x[1], y[1], a),
        glm::mix(x[2], y[2], a),
        glm::mix(x[3], y[3], a));
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

static glm::mat4 calculateBoneTransform(
    uint32_t currentClipID,
    const std::string &boneName,
    float currentTime,
    const std::shared_ptr<Model> &model);

// ----------------------------------------------------------------------
// FUNÇÃO AUXILIAR: Traversa a Hierarquia de Nodos (Cinemática Forward)
// ----------------------------------------------------------------------
static void readNodeHierarchy(
    const std::shared_ptr<Model> &model,
    uint32_t currentClipID,
    uint32_t previousClipID,
    float currentTime,
    float blendFactor,
    std::vector<glm::mat4> &finalBoneTransforms,
    const std::string &nodeName,
    const glm::mat4 &parentTransform)
{
    // 0. Verifica se o nó é um bone do esqueleto.
    bool isSkeletonBone = model->getBoneIndexByName(nodeName) != -1;

    // 1. Pega a transformação local estática (T, R, S do GLTF) como base.
    glm::mat4 nodeTransform = model->getNodeLocalTransform(nodeName);

    // CORREÇÃO CRÍTICA (Fixa o Colapso/Bind Pose):
    // Se for um bone, a transformação estática inicial DEVE ser Identity.
    // Isso anula a matriz de transformação do objeto Armature, que é a causa do colapso.
    if (isSkeletonBone)
    {
        nodeTransform = glm::mat4(1.0f);
    }

    // Variáveis auxiliares
    glm::mat4 animatedPose = glm::mat4(1.0f);
    glm::mat4 previousAnimatedPose = glm::mat4(1.0f);

    // 2. CÁLCULO DA POSE ANIMADA ATUAL
    if (currentClipID != 0)
    {
        // A animação só afeta os bones
        if (isSkeletonBone)
        {
            animatedPose = calculateBoneTransform(
                currentClipID, nodeName, currentTime, model);
        }

        // Se a pose animada foi calculada (não é Identity), aplique-a.
        if (animatedPose != glm::mat4(1.0f))
        {
            nodeTransform = animatedPose; // Aplica a pose animada (T, R, S keyframes)
        }
        // Se animatedPose for Identity: nodeTransform MANTÉM Identity (se for bone)
        // ou a Transformação Local Estática (se não for bone). CORRETO.
    }

    // 3. LÓGICA DE BLEND:
    if (previousClipID != 0 && blendFactor < 1.0f)
    {
        if (isSkeletonBone)
        {
            previousAnimatedPose = calculateBoneTransform(
                previousClipID, nodeName, currentTime, model);
        }

        if (previousAnimatedPose != glm::mat4(1.0f))
        {
            // O blend é feito entre a pose ANTERIOR e a pose ATUAL (nodeTransform)
            nodeTransform = lerp(previousAnimatedPose, nodeTransform, blendFactor);
        }
    }

    // 4. Calcula a Transformação Global (Model Space): Global = Pai Global * Local
    glm::mat4 globalTransform = parentTransform * nodeTransform;

    // 5. Armazena a Transformação Global do Nó (para Debug Lines)
    model->setNodeGlobalTransform(nodeName, globalTransform);

    // 6. Calcula a matriz final para o shader: Final = Global * IBM
    int boneIndex = model->getBoneIndexByName(nodeName);
    if (boneIndex != -1)
    {
        const auto &boneInfoMap = model->getBoneInfoMap();
        if (boneInfoMap.count(nodeName))
        {
            const BoneInfo &boneInfo = boneInfoMap.at(nodeName);

            // BoneFinal = GlobalTransform (Animada) * InverseBindMatrix (IBM)
            finalBoneTransforms[boneInfo.id] = globalTransform * boneInfo.offset;
        }
    }

    // 7. Continua recursivamente para os filhos
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
        previousAnimationID,
        currentTime,
        blendFactor,
        finalBoneTransforms,
        model->getSkeletonRootName(),
        glm::mat4(1.0f));
}

// ----------------------------------------------------------------------
// FUNÇÕES AUXILIARES DE ANIMAÇÃO (SRP: Cálculo de Pose)
// ----------------------------------------------------------------------

// Encontra o par de keyframes (anterior e próximo) para um determinado tempo.
template <typename T>
static bool findKeyframePair(float time, const std::vector<KeyFrame<T>> &keys, int &prevIndex, int &nextIndex, float &factor)
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
            if (totalTime > 0.0001f)
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
    if (!clip)
    {
        Engine::Core::Log::Warn(std::format("[ANIM_DEBUG] Falha! Clipe ID {} não encontrado para o osso {}.", currentClipID, boneName));
        return glm::mat4(1.0f);
    }

    // NOVO: Verifica a duração
    if (clip->duration <= 0.0001f)
    {
        Engine::Core::Log::Error(std::format("[ANIM_DEBUG] DURAÇÃO ZERO! Clipe: {}. O Keyframe está vazio ou malformado.", currentClipID));
        return glm::mat4(1.0f);
    }
    if (!clip || !clip->boneChannels.count(boneName))
    {
        return glm::mat4(1.0f); // Retorna Identity se o canal não for encontrado
    }

    const BoneChannel &channel = clip->boneChannels.at(boneName);

    // NOTA: Para ciclos, o tempo deve ser ajustado: currentTime = fmod(currentTime, clip->duration);
    float animTime = 0.0f;
    if (clip->duration > 0.0001f)
    {
        animTime = std::fmod(currentTime, clip->duration); // CORREÇÃO: Removido o 'float'
    }

    // Log de Verificação (ADICIONAR PARA DEBUG)
    if (boneName == model->getSkeletonRootName())
    {
        Engine::Core::Log::Info(std::format("[ANIM_DEBUG] ROOT: {}, Time: {:.2f}/{:.2f}",
                                            boneName,
                                            animTime,
                                            clip->duration));
    }

    // 1. Extrair T, R, S da Transformação Local Estática (Rest Pose)
    glm::mat4 localRestTransform = model->getNodeLocalTransform(boneName);

    // Inicialização da Pose (Componentes)
    glm::vec3 position = glm::vec3(localRestTransform[3]);                                                     // Translação
    glm::quat defaultR = glm::quat_cast(localRestTransform);                                                   // Rotação (glm::quat)
    glm::vec3 scale = glm::vec3(localRestTransform[0][0], localRestTransform[1][1], localRestTransform[2][2]); // Escala (simplificada)

    // A rotação da Engine (Engine::Math::Quat) deve ser inicializada com a Rest Pose.
    Engine::Math::Quat rotation = Engine::Math::Quat(defaultR);

    // Declaração e inicialização segura das variáveis de índice.
    int prev = 0;
    int next = 0;
    float factor = 0.0f;

    // 2. Posição (Translation) - Substitui o valor da Rest Pose se existir
    if (findKeyframePair(animTime, channel.positionKeys, prev, next, factor))
    {
        position = glm::mix(channel.positionKeys[prev].value, channel.positionKeys[next].value, factor);
    }

    // 3. Rotação (Rotation - SLERP) - Substitui o valor da Rest Pose se existir
    // Reinicia indices para o próximo Keyframe
    prev = 0;
    next = 0;
    factor = 0.0f;
    if (findKeyframePair(animTime, channel.rotationKeys, prev, next, factor))
    {
        Engine::Math::Quat q1 = channel.rotationKeys[prev].value;
        Engine::Math::Quat q2 = channel.rotationKeys[next].value;

        // CORREÇÃO C2039: O resultado é Engine::Math::Quat. Atribui diretamente.
        rotation = Engine::Math::Quat::slerp(q1, q2, factor);
    }

    // 4. Escala (Scale) - Substitui o valor da Rest Pose se existir
    // Reinicia indices para o próximo Keyframe
    prev = 0;
    next = 0;
    factor = 0.0f;
    if (findKeyframePair(animTime, channel.scaleKeys, prev, next, factor))
    {
        scale = glm::mix(channel.scaleKeys[prev].value, channel.scaleKeys[next].value, factor);
    }

    // 5. Constrói a Matriz Local: Translação * Rotação * Escala
    glm::mat4 localTransform = glm::mat4(1.0f);
    localTransform = glm::translate(localTransform, position);
    // O método toMat4() é usado no lugar de glm::mat4_cast(rotation), pois rotation é Engine::Math::Quat.
    localTransform = localTransform * rotation.toMat4();
    localTransform = glm::scale(localTransform, scale);

    return localTransform;
}