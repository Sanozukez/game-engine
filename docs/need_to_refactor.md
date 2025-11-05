

# ASSET MANAGER
Necessário remover IDS hardcoded e implementar função de busca

animation_sampler.h/.cpp	Lógica de sampling e interpolação.	findKeyframePair, calculateBoneTransform
skeleton_traverser.h/.cpp	Lógica de cinemática forward (hierarquia).	readNodeHierarchy
animation_utils.h/.cpp	Utilitários de alto nível/hashing/blending.	getAnimationHashID, calculateBoneTransforms, IsRootBoneName, lerp