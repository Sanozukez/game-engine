# 🔍 Análise Técnica: Extensão do Asset Dictionary

## 📊 **FLUXO ATUAL MAPEADO**

### **Estrutura Binária Existente**
```
File: assets_dictionary.bin (Version implícita: 100)

[Header - 4 bytes]
uint32_t total_count;

[Body - 136 bytes × N]
AssetEntry[N]
```

### **AssetManager Load Flow**
```cpp
AssetManager::loadAssetDictionary()
├─ Abre assets_dictionary.bin
├─ Lê total_count (uint32_t)
├─ Lê AssetEntry[total_count] 
│  └─ sizeof(AssetEntry) = 136 bytes
├─ Popula m_assetIDToPathMap[hash] = path
└─ Log: "X assets carregados"

AssetManager::getModel(assetID)
├─ getAssetPathByID(assetID)
│  └─ m_assetIDToPathMap[assetID] → "character_test.glb"
├─ Cache check: m_modelCache["character_test.glb"]
├─ Se miss: GLTFLoader::loadGLTF()
│  └─ Carrega meshes, skeleton, animations
│  └─ AnimationDataMapper extrai animations do GLTF
│  └─ Model::m_animations[hash(name)] = AnimationAsset*
└─ Retorna shared_ptr<Model>
```

---

## 🎯 **PLANO: Fase 1.1 - Adicionar Structs**

### **Arquivo**: `shared/mmap_format/SceneFileFormat.h`

### **Modificações**:

#### **1. Nova Enum (Asset Types)**
```cpp
enum class AssetType : uint8_t {
    TYPE_UNKNOWN = 0,
    TYPE_MODEL = 1,        // Static mesh (sem animations)
    TYPE_TEXTURE = 2,
    TYPE_CHARACTER_MODEL = 3,  // NOVO: Model com animations
    TYPE_AUDIO = 4         // Futuro
};
```

#### **2. Nova Struct: AnimationMapping**
```cpp
#define MAX_ANIM_NAME_LENGTH 64

struct AnimationMapping {
    uint32_t engine_name_hash;          // hash("idle") - usado no código
    char source_name[MAX_ANIM_NAME_LENGTH]; // "idle_combat" - nome no GLTF
    
    float duration;                     // Duração (segundos) - validação
    float blend_in_time;                // Fade in (segundos)
    float blend_out_time;               // Fade out (segundos)
    float default_playback_speed;       // Multiplicador (1.0 = normal)
    float movement_speed;               // m/s (0 = static/idle)
    
    uint8_t looping;                    // 1 = loop, 0 = one-shot
    uint8_t priority;                   // 0-255 (maior = interrompe menor)
    uint16_t reserved;                  // Padding/futuro
};
// sizeof(AnimationMapping) = 88 bytes
```

#### **3. Estender AssetEntry**
```cpp
struct AssetEntry {
    // [CAMPOS EXISTENTES - NÃO MUDAR]
    uint32_t asset_id;                        // Hash do path
    char asset_path[MAX_ASSET_PATH_LENGTH];   // 128 bytes
    uint8_t asset_type;                       // Usar AssetType enum
    uint8_t reserved[3];                      // Padding
    
    // [NOVOS CAMPOS]
    uint32_t animation_count;                 // Quantas animations (0 se não tem)
    uint64_t animation_data_offset;           // Offset no arquivo (0 se count=0)
};
// sizeof(AssetEntry) antigo: 136 bytes
// sizeof(AssetEntry) novo: 152 bytes (diferença: +16 bytes)
```

#### **4. Novo Header do Dictionary**
```cpp
struct AssetDictionaryHeader {
    uint32_t magic;                     // 0x41535444 = "ASTD"
    uint16_t version;                   // 101 (era 100 implícito)
    uint16_t flags;                     // Reservado
    
    uint32_t asset_count;               // Número de AssetEntry
    uint64_t animation_section_offset;  // Offset pra AnimationMapping[] (0 se sem anims)
    
    uint32_t reserved[4];               // Padding/expansão futura
};
// sizeof(AssetDictionaryHeader) = 32 bytes
```

---

## 📐 **LAYOUT DO ARQUIVO BINÁRIO (Versão 101)**

```
Offset  | Size | Content
--------|------|------------------------------------------
0x0000  | 32   | AssetDictionaryHeader
        |      |   - magic = 0x41535444
        |      |   - version = 101
        |      |   - asset_count = N
        |      |   - animation_section_offset = X
--------|------|------------------------------------------
0x0020  | 152N | AssetEntry[N]
        |      |   [0] wall.glb (animation_count=0)
        |      |   [1] character.glb (animation_count=5, offset=Y)
        |      |   [2] terrain.glb (animation_count=0)
--------|------|------------------------------------------
0xXXXX  | 88M  | AnimationMapping[] (seção contígua)
        |      |   Character #1: 5 mappings
        |      |     - hash("idle") → "idle_combat"
        |      |     - hash("walk") → "walk_forward"
        |      |     - ...
        |      |   Character #2: 8 mappings
        |      |     - ...
--------|------|------------------------------------------
EOF
```

---

## ⚙️ **IMPACTO NOS SISTEMAS**

### **dictionary_compiler.cpp** (Precisa Mudar)
```cpp
// ANTES (version 100):
[write uint32_t: total_count]
[write AssetEntry[N]: 136 bytes cada]

// DEPOIS (version 101):
[write AssetDictionaryHeader: 32 bytes]
[write AssetEntry[N]: 152 bytes cada]
[write AnimationMapping[M]: 88 bytes cada]
```

### **asset_manager.cpp** (Precisa Adaptar)
```cpp
// loadAssetDictionary() - ANTES:
file.read(&total_count, 4);
file.read(entries.data(), total_count * 136);

// loadAssetDictionary() - DEPOIS:
AssetDictionaryHeader header;
file.read(&header, sizeof(header));

if (header.version < 101) {
    // Lógica antiga (backward compatibility)
    readOldFormat(file);
} else {
    // Nova lógica
    file.read(entries.data(), header.asset_count * 152);
    
    if (header.animation_section_offset > 0) {
        file.seekg(header.animation_section_offset);
        // Ler AnimationMapping[]
    }
}
```

---

## ✅ **CHECKLIST - FASE 1.1 (APENAS STRUCTS)**

### **Objetivo**: Adicionar structs sem quebrar compilação

- [ ] Abrir `shared/mmap_format/SceneFileFormat.h`
- [ ] Adicionar `enum class AssetType`
- [ ] Adicionar `struct AnimationMapping` (após AssetEntry)
- [ ] Adicionar `struct AssetDictionaryHeader` (antes de AssetEntry)
- [ ] Modificar `struct AssetEntry` (adicionar 2 campos)
- [ ] Compilar engine: `cmake --build build`
- [ ] Verificar: 0 erros de compilação
- [ ] Commit: `feat: add animation metadata structures (v101 format)`

**Tempo estimado**: 10 minutos  
**Risk**: Baixo (só adiciona structs, não muda lógica)

---

## 🚦 **TESTE DE FASE 1.1**

### **Validação Rápida**:
```cpp
// Em qualquer .cpp temporário
#include "shared/mmap_format/SceneFileFormat.h"
#include <iostream>

int main() {
    std::cout << "sizeof(AssetDictionaryHeader): " 
              << sizeof(AssetDictionaryHeader) << "\n";
    std::cout << "sizeof(AssetEntry): " 
              << sizeof(AssetEntry) << "\n";
    std::cout << "sizeof(AnimationMapping): " 
              << sizeof(AnimationMapping) << "\n";
    return 0;
}
```

**Output esperado**:
```
sizeof(AssetDictionaryHeader): 32
sizeof(AssetEntry): 152
sizeof(AnimationMapping): 88
```

---

**Pronto para começar?** Aguardando seu commit atual antes de modificar! 🚀
