// docs/asset_pipeline_anim.md

# Pipeline de Animação e Nomenclatura de Assets

Este documento define os padrões de nomenclatura e regras para exportação de modelos animados (.GLB) do Blender/Ferramenta 3D para o Engine.

## 1. Padrões de Nomenclatura de Clipes (Clips)

O Engine usa o nome do clipe de animação como a chave para o seu Hash ID interno. É crucial que a nomenclatura no Blender corresponda ao esperado no código.

| Categoria | Nome do Clipe (Blender/GLTF) | Descrição |
| :--- | :--- | :--- |
| **Base** | `Idle` | Animação padrão (personagem parado). |
| **Movimento** | `Walk`, `Run` | Usados pelo `PlayerSystem` e `AnimationSystem` para movimentação. |
| **Combate** | `Attack_01`, `Defend`, `Cast` | Animações de interação do jogador. |
| **Outros** | `Jump`, `Fall`, `Death` | Animações de estado. |

## 2. Estrutura do Arquivo GLB

1. **Skeleton (Armature):** Deve ser exportado com o nome do esqueleto raiz. A Engine assume que o primeiro `skin` do GLTF é o principal.
2. **Keyframes:** Utilize interpolação Linear ou Cúbica no Blender. O `AnimationLoader` processa os *keyframe* por amostragem.
3. **Múltiplos Clipes:** **Atenção:** Atualmente, o `AnimationLoader` só processa o **primeiro** clipe encontrado no array `animations` do GLTF. Para usar múltiplos clipes, será necessário modificar o `AnimationLoader` para iterar sobre todos eles e nomeá-los corretamente no Blender.

## 3. O Conceito de Blend

O Engine suporta o *blend* (mistura suave) entre duas poses.

* **Interpolação de Keyframe:** A Engine faz `SLERP` (Quaternions) e `LERP` (Posição/Escala) automaticamente dentro do `AnimationUtils`.
* **Transição de Clipes:** O `AnimationSystem` gerencia o `blendFactor` (de 0.0 a 1.0) para misturar as matrizes de `Clip A` e `Clip B`. Para que a transição ocorra, a lógica do `AnimationSystem` deve ser completada.