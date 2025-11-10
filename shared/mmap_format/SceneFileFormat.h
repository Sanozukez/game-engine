// // shared/mmap_format/SceneFileFormat.h
//
// Este arquivo define a estrutura binária do formato de arquivo MMAP (MMORPG Map).
//
// A diretiva #pragma pack(push, 1) é usada para forçar o alinhamento de 1 byte,
// garantindo que as estruturas C++ correspondam exatamente ao layout do arquivo no disco.

#pragma once

#include <cstdint>

// Garante que o alinhamento seja de 1 byte (sem padding), crucial para formatos de arquivo binário
#pragma pack(push, 1)

// =========================================================================================
// 1. ENUMERAÇÃO DE SEÇÕES (TIPOS DE DADOS DO MAPA)
// Define os diferentes blocos de dados contidos no arquivo MMAP.
// =========================================================================================

enum class SceneSectionType : uint32_t
{
    SCENE_SECTION_NODES = 0,     // Entidades base (posição/transformação/referência de Asset)
    SCENE_SECTION_STATIC_MESHES, // Dados de Meshes Estáticas (Instâncias de Módulos Modulares)
    SCENE_SECTION_NPC_SPAWNS,    // Dados de NPCs e Spawns Points (Gameplay)
    SCENE_SECTION_TRIGGERS,      // Volumes de Eventos/Portais/Cutscenes
    SCENE_SECTION_LIGHTS,        // Dados de Iluminação (Luzes Dinâmicas e Probes)
    SCENE_SECTION_TERRAIN_DATA,  // Dados do Terreno/Heightmap/Textura
    SCENE_SECTION_NAVMESH,       // Dados de Navegação (Pathfinding)
    SCENE_SECTION_AUDIO_ZONES,   // Zonas de Áudio/Reverb
    SCENE_SECTION_VFX_SPAWNS,    // Spawns de Efeitos Visuais (Partículas, Decals)
    SCENE_SECTION_TERRAIN_METADATA, // Nome da Mesh de Terreno no Node TER_ TerrainMetaData

    SCENE_SECTION_COUNT // Total de Seções (Deve ser o último item)
};

// =========================================================================================
// 2. ESTRUTURAS DE INDEXAÇÃO
// =========================================================================================

// Estrutura que define a localização e quantidade de um bloco de dados no arquivo.
struct SceneSectionEntry
{
    uint32_t count;  // Quantidade de itens nesta seção (ex: 50 NPCs)
    uint64_t offset; // Posição (em bytes) onde a seção começa no arquivo
};

// =========================================================================================
// 3. CABEÇALHO DO ARQUIVO (SceneFileHeader)
// Contém os metadados do arquivo e a tabela de índices.
// =========================================================================================

struct SceneFileHeader
{
    // 1. Metadados do Arquivo (Foco na Integridade e Compatibilidade)
    uint32_t magic = 0x4D4D4150; // 'MMAP' (little-endian)
    uint16_t version = 100;      // Versão 1.00
    uint16_t flags = 0;          // Flags para otimizações (Ex: 1 = HasOcclusionData)

    uint64_t fileSize; // Tamanho total do arquivo no disco

    // 2. Integridade de Dados
    uint64_t data_checksum; // Hash (CRC64 ou CRC32) dos dados para verificar se o arquivo está corrompido.

    // 3. Informação de Rede e Localização (Essencial para MMORPG)
    uint32_t map_id;  // ID único global do mapa (Para o servidor saber qual mapa carregar)
    uint32_t chunk_x; // Coordenada de Chunk X (Para streaming/occlusion)
    uint32_t chunk_y; // Coordenada de Chunk Y

    uint32_t reserved_A; // Espaço reservado para expansão (Alinhamento)

    // NPC Spawns
    uint32_t npc_spawn_count;      // Quantidade de spawns
    uint64_t npc_spawn_data_offset; // Offset para o array de NPCSpawnData

    // 4. Tabela de Seções (Indexação Rápida)
    SceneSectionEntry sections[(int)SceneSectionType::SCENE_SECTION_COUNT];
};

// =========================================================================================
// 4. ESTRUTURA DA ENTIDADE BASE (SceneNode)
// Todo objeto posicionado no mundo herda esta estrutura.
// =========================================================================================

enum class EntityType : uint8_t
{
    TYPE_UNKNOWN = 0,
    TYPE_STATIC_MESH = 1,
    TYPE_NPC = 2,
    TYPE_MOB_SPAWN = 3,
    TYPE_TERRAIN_BASE = 4, // <-- NOVO: Para o Terreno Base (Prefixos: TER_)
    TYPE_ARRAY_START = 5,  // <-- NOVO: Ponto inicial de um Array (Prefixos: ARRAY_)
    TYPE_TRIGGER_VOLUME = 6,
    TYPE_LIGHT = 7,
    TYPE_AUDIO_SOURCE = 8,
    // Tipos de Assets que sua engine consegue renderizar
};

// =========================================================================================
// 5. ESTRUTURA DE DICIONÁRIO DE ASSETS
// =========================================================================================

#define MAX_ASSET_PATH_LENGTH 128
#define MAX_ANIM_NAME_LENGTH 60  // 60 bytes para nome da animação no GLTF

// -----------------------------------------------------------------------------------------
// 5.1 ASSET DICTIONARY HEADER (Version 101)
// -----------------------------------------------------------------------------------------
struct AssetDictionaryHeader {
    uint32_t magic;                     // 0x41535444 = "ASTD" (Asset Dictionary)
    uint16_t version;                   // 101 (formato atual)
    uint16_t padding;                   // Padding para alinhamento de 4 bytes
    
    uint32_t asset_count;               // Número total de AssetEntry
    uint64_t animation_section_offset;  // Offset para AnimationMapping[] (0 se sem animations)
    
    uint32_t reserved[2];               // Reservado para expansão futura
};
// sizeof(AssetDictionaryHeader) = 28 bytes (4+2+2+4+8+8 = 28)

// -----------------------------------------------------------------------------------------
// 5.2 ASSET ENTRY (Entrada Individual do Dicionário)
// -----------------------------------------------------------------------------------------
struct AssetEntry {
    uint32_t asset_id;                        // Hash do asset path (usado no MMAP)
    char asset_path[MAX_ASSET_PATH_LENGTH];   // Caminho do arquivo GLB/Texture
    uint8_t asset_type;                       // 1=Model, 2=Texture, 3=CharacterModel, 4=Audio
    uint8_t reserved[7];                      // Padding para alinhamento (8 bytes total)
    
    // NOVO (v101): Dados de Animação (só usado se asset_type == 3 - CharacterModel)
    uint32_t animation_count;                 // Quantas AnimationMapping tem (0 se não é character)
    uint64_t animation_data_offset;           // Offset no arquivo para AnimationMapping[] (0 se count=0)
};
// sizeof(AssetEntry) v100: 136 bytes
// sizeof(AssetEntry) v101: 152 bytes (4 + 128 + 8 + 4 + 8 = 152)

// -----------------------------------------------------------------------------------------
// 5.3 ANIMATION MAPPING (Metadados de Animação - Version 101)
// -----------------------------------------------------------------------------------------
struct AnimationMapping {
    uint32_t engine_name_hash;          // Hash do nome usado no código (ex: hash("idle"))
    char source_name[MAX_ANIM_NAME_LENGTH]; // Nome no arquivo GLTF (ex: "idle_combat")
    
    // Metadata de Reprodução
    float duration;                     // Duração em segundos (para validação)
    float blend_in_time;                // Tempo de fade in (segundos)
    float blend_out_time;               // Tempo de fade out (segundos)
    float default_playback_speed;       // Multiplicador de velocidade (1.0 = normal)
    float movement_speed;               // Velocidade de movimento (m/s, 0 = estático)
    
    // Flags de Comportamento
    uint8_t looping;                    // 1 = loop, 0 = one-shot
    uint8_t priority;                   // 0-255 (maior valor = maior prioridade, interrompe menor)
    uint8_t reserved[2];                // Padding para alinhamento de 4 bytes
};
// sizeof(AnimationMapping) = 88 bytes (4 + 60 + 20 + 4 = 88)

// =========================================================================================
// 6. BLOCOS DE DADOS ESPECÍFICOS (APONTADOS PELO SceneNode::specific_data_offset)
// =========================================================================================

// Dados para a Seção: SCENE_SECTION_NPC_SPAWNS (TYPE_NPC, TYPE_MOB_SPAWN)
struct NPCSpawnData
{    
   // A malha do Asset (modelo 3D) vem do SceneNode::asset_reference_id
    uint32_t unit_db_id;        // ID do Mob/NPC no Database de Unidades (ex: 1 = Lobo)
    float position[3];         // Posição de spawn (Copia a posição do Empty)
    float respawn_time_sec;    // Tempo de respawn em segundos (0 para NPCs estáticos)
    uint16_t patrol_route_id;  // ID que referencia a rota de patrulha (NavMesh)
    uint16_t max_mobs_in_area; // Quantidade máxima de mobs ativos (para spawns de monstro)
    uint32_t loot_table_id;    // ID que referencia a tabela de itens

    // Podemos adicionar mais flags e referências a logic scripts aqui no futuro.
};

// Dados para a Seção: SCENE_SECTION_TRIGGERS (TYPE_TRIGGER_VOLUME)
struct TriggerVolumeData
{
    uint32_t trigger_id;    // ID Único para o sistema de Eventos/Scripts
    uint16_t trigger_type;  // Tipo de trigger (1=Portal, 2=Cutscene, 3=QuestArea)
    uint16_t area_shape;    // 1=Box, 2=Sphere, 3=Capsule
    float extents[3];       // Dimensões do volume (XYZ para Box, Raio para Sphere)
    uint32_t target_map_id; // Destino (usado para portais)

    // Flags de rede e interação podem ser salvas no SceneNode::node_flags.
};

// Dados para a Seção: SCENE_SECTION_LIGHTS (TYPE_LIGHT)
struct LightData
{
    uint32_t light_id;
    uint8_t light_type;    // 1=Point, 2=Spot, 3=Directional
    float color[3];        // RGB
    float intensity;       // Intensidade da luz
    float range;           // Alcance da luz (apenas para Point/Spot)
    uint8_t casts_shadows; // 1=Verdadeiro, 0=Falso
};

// Dados para a Seção: SCENE_SECTION_TERRAIN_DATA (TYPE_TERRAIN_BASE)
// Esta struct é pequena, mas crucial para o streaming futuro
#define MAX_MESH_NAME_LENGTH 64

struct TerrainMetaData
{
    uint32_t collision_mesh_id; // ID de referência para a malha de colisão (UCX_)
    uint32_t detail_mesh_id;    // ID de referência para a malha de grama/folhagem
    float uv_scale;             // Escala da textura do terreno (para evitar repetição visual)
    char internal_mesh_name[MAX_MESH_NAME_LENGTH];  
};

struct SceneNode
{
    // 1. Identificação (Crucial para Rede/Editor)
    uint32_t entity_id;    // ID Único no mundo (fixo), para sincronização de rede
    uint16_t parent_index; // Índice para o 'parent' na array de Nodes (Hierarquia), 0 = Root
    EntityType type : 8;   // Tipo da entidade (8 bits)
    uint8_t entity_flags;  // Flags da entidade (Ex: 1 = Replicável pela rede)

    // 2. Transformação (64 bytes)
    float position[3];      // Posição X, Y, Z
    float rotation_quat[4]; // Rotação em Quatérnios (W, X, Y, Z) - Excelente para interpolação
    float scale[3];         // Escala X, Y, Z

    // 3. Referência (Instanciamento e Assets)
    uint32_t asset_reference_id; // ID que aponta para o Asset real (GLB/Prefab na sua biblioteca)

    // 4. Lógica Específica
    // Offset que aponta para o bloco de dados específico desta entidade (dentro de uma das seções do Header).
    // Ex: Se type=NPC, aponta para os dados de Quest/Respawn na seção SCENE_SECTION_NPC_SPAWNS
    uint64_t specific_data_offset;
    char name[64];          // Permite um nome de até 63 caracteres
};

// Remove o alinhamento de 1 byte e volta para o padrão do compilador
#pragma pack(pop)

// Opcional: Define um alias para o tipo de dado mais comum no mapa
using SceneNodeID = uint32_t;