

# ASSET MANAGER
Necessário remover IDS hardcoded e implementar função de busca

animation_sampler.h/.cpp	Lógica de sampling e interpolação.	findKeyframePair, calculateBoneTransform
skeleton_traverser.h/.cpp	Lógica de cinemática forward (hierarquia).	readNodeHierarchy
animation_utils.h/.cpp	Utilitários de alto nível/hashing/blending.	getAnimationHashID, calculateBoneTransforms, IsRootBoneName, lerp


Plano de Correção (Futuro):

Renomeie engine/ecs/components/animation_component.h -> struct Animation para struct AnimationComponent.

Renomeie engine/asset/animation.h -> struct Animation para struct AnimationAsset.

Renomeie engine/asset/skeleton.h -> struct Bone para struct BoneData (ou BoneInfo, como no seu código antigo).

Isso torna o código auto-explicativo e o erro C2039 que tivemos torna-se impossível de acontecer.


Para realmente seguir o seu G-guideline ("manter códigos o mais focados e SRP (single responsability principle) possivel"), este é o plano que devemos adotar:

Mapeadores de Dados (O que animation_data_mapper devia ser):

animation_data_mapper.cpp (O "Chefe"): Deve ter apenas 50 linhas. A sua única função (mapAnimationData) deve ser chamar os mappers abaixo.

NOVO: SkeletonMapper.cpp: Deve conter mapSkeleton e processBoneNode (a lógica de ossos, parentId e hierarquia).

NOVO: AnimationClipMapper.cpp: Deve conter mapAnimations (a lógica de ler keyframes T, R, S).

gltf_data_reader.cpp (Existente): Fica como está (lê malhas).

Sistemas de Runtime (O que AnimationSystem devia ser):

animation_system.cpp (O "Chefe"): Também deve ficar pequeno. A sua função update deve apenas orquestrar os passos.

keyframe_sampler.cpp (Existente): Perfeito. (SRP: "Como interpolar T, R, S").

skeleton_hierarchy.cpp (Existente): Perfeito. (SRP: "Como aplicar Cinemática Forward").

NOVO: PoseBlender.cpp: O seu animation_utils.cpp antigo tinha lógica de blend (SLERP/LERP entre currentLocal e previousLocal). O seu AnimationSystem atual não faz blending (ele só usa currentAnim). Quando formos adicionar blending, a lógica (decomposeTRS, composeTRS, slerp) deve ir para este novo ficheiro (SRP: "Como misturar duas poses").