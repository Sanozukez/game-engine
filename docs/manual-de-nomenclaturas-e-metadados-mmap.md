# Manual de Nomenclatura e Metadados MMAP
Este manual define a convenção de nomeação no Blender e o mapeamento para as estruturas binárias do seu arquivo de cena (.mmap).

## I. Estrutura de Nomenclatura (Regra Geral)  
Todo objeto de layout (Empty) no Blender deve seguir o formato, usando o ```_``` como separador:

```PREFIXO_[Nome Unico]_[Variante]_SUFIXO```

| **Elemento** | **Propósito** | **Exemplo** |
|---|---|---|
| PREFIXO | Obrigatório. Define o tipo de objeto para a Engine. | SM_, SPAWN_, ARRAY_ |
| Nome Único | Identifica o Asset/Módulo que será instanciado. | Wall_Module_A, Tree_Pine_01 |
| SUFIXO | Opcional. Define a função do objeto (ex: Start/End de um Array, Colisão). | _START, _COL, _VIS |

## II. Definição dos Prefixos (Tipos de Objeto)
Seu mmap_compiler deve processar os seguintes prefixos para atribuir o SceneNode::type.

| **Prefixo** | **Nome Completo** | **SceneNode::type (Engine)** | **Propósito e Uso** |
|---|---|---|---|
| TER_ | Terrain Meta | TYPE_TERRAIN_BASE | Define o ponto de origem e a geometria do terreno principal. Essencial para o carregamento base do mapa.  |
| SM_ | Static Mesh | TYPE_STATIC_MESH | Objeto estático único ou Módulo de Colisão. Não se repete por Array. (Ex: Cantos do muro, Pedras grandes, Cascalhos).  |
| ARRAY_ | Array Start | TYPE_ARRAY_START | Essencial para otimização. Marca o ponto inicial de uma seção modular que se repete (Muros, Grades, Cercas).  |
| SPAWN_ | Spawn Point | TYPE_NPC ou TYPE_PLAYER_START | Marcadores de posição para entidades dinâmicas (Ex: NPC, Player Start, Mobs).  |
| TRG_ | Trigger Volume | TYPE_TRIGGER_VOLUME | Marcas de área invisíveis para gameplay (Ex: Portais, Volumes de Áudio/Cutscene). |

## III. Regras Específicas para Nomenclatura de Arrays
Esta regra substitui o uso de múltiplos Empties para repetição.

| **Estrutura** | **Nomenclatura no Blender** | **Ação do Compiler** |
|---|---|---|
| Ponto Inicial | Um Empty nomeado ARRAY_Wall_Section_START | O compiler salva a posição e rotação deste Empty.  |
| Ponto Final | Um Empty nomeado ARRAY_Wall_Section_END | O compiler usa a posição deste Empty para calcular a distância total do Array.  |
| Módulo Base | Um GLB separado contendo a geometria e a colisão (Ex: SM_Wall_Module_A.glb). | O nome do Módulo (Wall_Module_A) será salvo no SceneNode como asset_reference_id. |

## IV. Uso de Metadados (Custom Properties)
Para adicionar dados de gameplay que o nome não pode expressar, você usará as Custom Properties do Blender.

O seu compiler deve ser instruído a buscar as seguintes propriedades no Empty:

| **Propriedade (Blender)** | **Tipo de Dados** | **Destino no MMAP** | **Uso de Gameplay** |
|---|---|---|---|
| ASSET_REF_ID | Texto/String | SceneNode::asset_reference_id | Obrigatório. O nome do arquivo GLB a ser instanciado. (Ex: Tree_Pine_LOD01.glb).  |
| SPAWN_MOB_ID | Inteiro | SCENE_SECTION_NPC_SPAWNS | Se for um SPAWN_, o ID do monstro a ser gerado (lido da sua base de dados).  |
| ARRAY_MODULE_LENGTH | Float | (Calculado pelo Compiler) | Obrigatório para ARRAY_. O comprimento exato do módulo a ser repetido (Ex: 4.0 metros).  |
| TERRAIN_COL_MESH | Texto/String | SCENE_SECTION_TERRAIN_DATA | O nome da Mesh de Colisão dentro do GLB principal (Ex: UCX_Terrain). |


Objetos por tipo e Aplicação

O Terreno é um dos principais objetos, ele sempre será único em cada cena. Para ser renderizado dentro do ambiente do jogo o terreno depende de 2 objetos, um Empty e a Mesh do terreno.
O Empty deve seguir o padrão de prefixo de nome: TER_Base_Mesh
A Mesh deve seguir o padrão de prefixo de novme: SM_Terrain_Mesh

O objeto Empty, define a posição exata onde o o terreno sera instanciado, será o ponto do centro da renderização. 0,0,0.
O objeto Mesh do terreno deve ter sua origem (pivot point) em seu centro geométrico, esse ponto ficará alinhado a posição do empty de terreno.

O arquivo de terreno geralmente é a base de toda cena. Todos os emptys de layout e mecanicas de jogo, vão no mesmo arquivo GLB do terreno, com isso cada Empty representa a posição exata para instanciação de assets, triggers e outras configurações em cima do terreno.