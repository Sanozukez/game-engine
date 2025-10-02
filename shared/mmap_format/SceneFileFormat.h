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

enum class SceneSectionType : uint32_t {
    SCENE_SECTION_NODES = 0,        // Entidades base (posição/transformação/referência de Asset)
    SCENE_SECTION_STATIC_MESHES,    // Dados de Meshes Estáticas (Instâncias de Módulos Modulares)
    SCENE_SECTION_NPC_SPAWNS,       // Dados de NPCs e Spawns Points (Gameplay)
    SCENE_SECTION_TRIGGERS,         // Volumes de Eventos/Portais/Cutscenes
    SCENE_SECTION_LIGHTS,           // Dados de Iluminação (Luzes Dinâmicas e Probes)
    SCENE_SECTION_TERRAIN_DATA,     // Dados do Terreno/Heightmap/Textura
    SCENE_SECTION_NAVMESH,          // Dados de Navegação (Pathfinding)
    SCENE_SECTION_AUDIO_ZONES,      // Zonas de Áudio/Reverb
    SCENE_SECTION_VFX_SPAWNS,       // Spawns de Efeitos Visuais (Partículas, Decals)
    
    SCENE_SECTION_COUNT             // Total de Seções (Deve ser o último item)
};

// =========================================================================================
// 2. ESTRUTURAS DE INDEXAÇÃO
// =========================================================================================

// Estrutura que define a localização e quantidade de um bloco de dados no arquivo.
struct SceneSectionEntry {
    uint32_t count;  // Quantidade de itens nesta seção (ex: 50 NPCs)
    uint64_t offset; // Posição (em bytes) onde a seção começa no arquivo
};


// =========================================================================================
// 3. CABEÇALHO DO ARQUIVO (SceneFileHeader)
// Contém os metadados do arquivo e a tabela de índices.
// =========================================================================================

struct SceneFileHeader {
    // 1. Metadados do Arquivo (Foco na Integridade e Compatibilidade)
    uint32_t magic = 0x4D4D4150;     // 'MMAP' (little-endian)
    uint16_t version = 100;          // Versão 1.00
    uint16_t flags = 0;              // Flags para otimizações (Ex: 1 = HasOcclusionData)

    uint64_t fileSize;               // Tamanho total do arquivo no disco

    // 2. Integridade de Dados
    uint64_t data_checksum;          // Hash (CRC64 ou CRC32) dos dados para verificar se o arquivo está corrompido.
    
    // 3. Informação de Rede e Localização (Essencial para MMORPG)
    uint32_t map_id;                 // ID único global do mapa (Para o servidor saber qual mapa carregar)
    uint32_t chunk_x;                // Coordenada de Chunk X (Para streaming/occlusion)
    uint32_t chunk_y;                // Coordenada de Chunk Y

    uint32_t reserved_A;             // Espaço reservado para expansão (Alinhamento)

    // 4. Tabela de Seções (Indexação Rápida)
    SceneSectionEntry sections[(int)SceneSectionType::SCENE_SECTION_COUNT]; 
};


// =========================================================================================
// 4. ESTRUTURA DA ENTIDADE BASE (SceneNode)
// Todo objeto posicionado no mundo herda esta estrutura.
// =========================================================================================

enum class EntityType : uint8_t {
    TYPE_UNKNOWN = 0,
    TYPE_STATIC_MESH = 1,
    TYPE_NPC = 2,
    TYPE_MOB_SPAWN = 3,
    TYPE_TRIGGER_VOLUME = 4,
    TYPE_LIGHT = 5,
    TYPE_AUDIO_SOURCE = 6,
    // Tipos de Assets que sua engine consegue renderizar
};

struct SceneNode {
    // 1. Identificação (Crucial para Rede/Editor)
    uint32_t entity_id;              // ID Único no mundo (fixo), para sincronização de rede
    uint16_t parent_index;           // Índice para o 'parent' na array de Nodes (Hierarquia), 0 = Root
    EntityType type : 8;             // Tipo da entidade (8 bits)
    uint8_t entity_flags;            // Flags da entidade (Ex: 1 = Replicável pela rede)

    // 2. Transformação (64 bytes)
    float position[3];               // Posição X, Y, Z
    float rotation_quat[4];          // Rotação em Quatérnios (W, X, Y, Z) - Excelente para interpolação
    float scale[3];                  // Escala X, Y, Z

    // 3. Referência (Instanciamento e Assets)
    uint32_t asset_reference_id;     // ID que aponta para o Asset real (GLB/Prefab na sua biblioteca)

    // 4. Lógica Específica
    // Offset que aponta para o bloco de dados específico desta entidade (dentro de uma das seções do Header).
    // Ex: Se type=NPC, aponta para os dados de Quest/Respawn na seção SCENE_SECTION_NPC_SPAWNS
    uint64_t specific_data_offset; 
};

// Remove o alinhamento de 1 byte e volta para o padrão do compilador
#pragma pack(pop)

// Opcional: Define um alias para o tipo de dado mais comum no mapa
using SceneNodeID = uint32_t;