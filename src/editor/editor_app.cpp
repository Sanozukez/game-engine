// src/editor/editor_app.cpp
// Implementação do Editor Visual - Primeira arquitetura da interface

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#endif

#include "editor_app.h"
#include "editor_sky_serialization.h"
#include "editor_theme.h"
#include "render/opengl_types.h"
#include "window/window.h"
#include "core/log.h"
#include "core/SceneLoader.h"
#include "shared/mmap_format/SceneFileFormat.h"
#include "world_partition/terrain_profile.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <iomanip>

#include <stb_image.h>

#include <glm/gtx/quaternion.hpp>

namespace {
    constexpr const char* kEditorTitle = "Game Engine Editor v0.1";

    std::string TrimCopy(const std::string& value) {
        auto begin = value.begin();
        while (begin != value.end() && std::isspace(static_cast<unsigned char>(*begin))) {
            ++begin;
        }

        auto end = value.end();
        while (end != begin && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
            --end;
        }

        return std::string(begin, end);
    }

    bool StartsWith(const std::string& value, const std::string& prefix) {
        return value.rfind(prefix, 0) == 0;
    }

    std::string ToGenericPathString(const std::filesystem::path& path) {
        return path.lexically_normal().generic_string();
    }

    std::string EscapeForCmd(const std::string& value) {
        std::string escaped = value;
        std::size_t pos = 0;
        while ((pos = escaped.find('"', pos)) != std::string::npos) {
            escaped.insert(pos, "\\");
            pos += 2;
        }
        return escaped;
    }

    int RunCommandAtRepoRoot(const std::string& repoRoot, const std::string& command) {
        const std::string cmdLine = "cmd /C \"cd /d \"" + EscapeForCmd(repoRoot) + "\" && " + command + "\"";
        return std::system(cmdLine.c_str());
    }

    std::string GetCMakeExecutable() {
        const std::filesystem::path cmakeFull = "C:/Program Files/CMake/bin/cmake.exe";
        if (std::filesystem::exists(cmakeFull)) {
            return "\"" + cmakeFull.generic_string() + "\"";
        }
        return "cmake";
    }

    bool IsMmapFileSignatureValid(const std::filesystem::path& path) {
        if (!std::filesystem::exists(path)) {
            return false;
        }

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        uint32_t magic = 0;
        file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        return file.good() && magic == 0x4D4D4150;
    }

    std::string ToIso8601UtcNow() {
        const auto now = std::chrono::system_clock::now();
        const std::time_t nowTimeT = std::chrono::system_clock::to_time_t(now);

        std::tm utcTm{};
#if defined(_WIN32)
        gmtime_s(&utcTm, &nowTimeT);
#else
        gmtime_r(&nowTimeT, &utcTm);
#endif

        std::ostringstream oss;
        oss << std::put_time(&utcTm, "%Y-%m-%dT%H:%M:%SZ");
        return oss.str();
    }

    std::string JsonEscape(const std::string& text) {
        std::string out;
        out.reserve(text.size());
        for (char ch : text) {
            switch (ch) {
                case '\\': out += "\\\\"; break;
                case '"': out += "\\\""; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default: out.push_back(ch); break;
            }
        }
        return out;
    }

    bool TryExtractDimensionsFromStem(const std::string& stem, int& outWidth, int& outHeight) {
        for (std::size_t i = 0; i < stem.size(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(stem[i]))) {
                continue;
            }

            std::size_t j = i;
            while (j < stem.size() && std::isdigit(static_cast<unsigned char>(stem[j]))) {
                ++j;
            }

            if (j >= stem.size() || (stem[j] != 'x' && stem[j] != 'X')) {
                i = j;
                continue;
            }

            std::size_t k = j + 1;
            if (k >= stem.size() || !std::isdigit(static_cast<unsigned char>(stem[k]))) {
                i = k;
                continue;
            }

            while (k < stem.size() && std::isdigit(static_cast<unsigned char>(stem[k]))) {
                ++k;
            }

            const int w = std::stoi(stem.substr(i, j - i));
            const int h = std::stoi(stem.substr(j + 1, k - (j + 1)));
            if (w > 0 && h > 0) {
                outWidth = w;
                outHeight = h;
                return true;
            }

            i = k;
        }

        return false;
    }

    bool LoadHeightmapSamples16(
        const std::filesystem::path& sourcePath,
        std::vector<uint16_t>& outSamples,
        int& outWidth,
        int& outHeight,
        std::string& outError) {
        outSamples.clear();
        outWidth = 0;
        outHeight = 0;

        std::string ext = sourcePath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        if (ext == ".r16" || ext == ".raw") {
            std::ifstream in(sourcePath, std::ios::binary | std::ios::ate);
            if (!in.is_open()) {
                outError = "Nao foi possivel abrir arquivo RAW/R16.";
                return false;
            }

            const std::streamsize size = in.tellg();
            if (size <= 0 || (size % 2) != 0) {
                outError = "Arquivo RAW/R16 invalido: tamanho precisa ser multiplo de 2 bytes.";
                return false;
            }

            in.seekg(0, std::ios::beg);
            std::vector<unsigned char> bytes(static_cast<std::size_t>(size));
            if (!in.read(reinterpret_cast<char*>(bytes.data()), size)) {
                outError = "Falha ao ler bytes de RAW/R16.";
                return false;
            }

            const std::size_t sampleCount = bytes.size() / 2;
            int parsedW = 0;
            int parsedH = 0;
            const bool hasDimsInName = TryExtractDimensionsFromStem(sourcePath.stem().string(), parsedW, parsedH);

            if (hasDimsInName) {
                if (static_cast<std::size_t>(parsedW) * static_cast<std::size_t>(parsedH) != sampleCount) {
                    outError = "Dimensoes no nome do arquivo nao batem com o tamanho do RAW/R16.";
                    return false;
                }
                outWidth = parsedW;
                outHeight = parsedH;
            } else {
                const int side = static_cast<int>(std::sqrt(static_cast<double>(sampleCount)));
                if (side <= 0 || static_cast<std::size_t>(side) * static_cast<std::size_t>(side) != sampleCount) {
                    outError = "RAW/R16 sem dimensoes no nome precisa ser quadrado perfeito (ex.: map_1024x1024.r16).";
                    return false;
                }
                outWidth = side;
                outHeight = side;
            }

            outSamples.resize(sampleCount);
            for (std::size_t i = 0; i < sampleCount; ++i) {
                const uint16_t lo = static_cast<uint16_t>(bytes[i * 2]);
                const uint16_t hi = static_cast<uint16_t>(bytes[i * 2 + 1]);
                outSamples[i] = static_cast<uint16_t>((hi << 8) | lo);
            }

            return true;
        }

        int width = 0;
        int height = 0;
        int channels = 0;

        if (uint16_t* img16 = stbi_load_16(sourcePath.string().c_str(), &width, &height, &channels, 1)) {
            if (width <= 0 || height <= 0) {
                stbi_image_free(img16);
                outError = "Imagem invalida para heightmap.";
                return false;
            }

            const std::size_t count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
            outSamples.assign(img16, img16 + count);
            outWidth = width;
            outHeight = height;
            stbi_image_free(img16);
            return true;
        }

        if (unsigned char* img8 = stbi_load(sourcePath.string().c_str(), &width, &height, &channels, 1)) {
            if (width <= 0 || height <= 0) {
                stbi_image_free(img8);
                outError = "Imagem 8-bit invalida para heightmap.";
                return false;
            }

            const std::size_t count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
            outSamples.resize(count);
            for (std::size_t i = 0; i < count; ++i) {
                outSamples[i] = static_cast<uint16_t>(static_cast<uint16_t>(img8[i]) * 257u);
            }

            outWidth = width;
            outHeight = height;
            stbi_image_free(img8);
            return true;
        }

        outError = "Formato de imagem nao suportado para leitura de heightmap.";
        return false;
    }

    bool BuildTerrainMmapFromHeightmap(
        const std::filesystem::path& sourceHeightmap,
        const std::filesystem::path& outputMmap,
        int chunkX,
        int chunkZ,
        float minHeightMeters,
        float maxHeightMeters,
        float zeroLevelMeters,
        int bitDepth,
        const std::string& normalizationMode,
        std::string& outError,
        int& outWidth,
        int& outHeight) {
        std::vector<uint16_t> samples;
        if (!LoadHeightmapSamples16(sourceHeightmap, samples, outWidth, outHeight, outError)) {
            return false;
        }

        if (samples.empty()) {
            outError = "Heightmap sem amostras.";
            return false;
        }

        uint16_t minH = samples.front();
        uint16_t maxH = samples.front();
        for (uint16_t v : samples) {
            minH = std::min(minH, v);
            maxH = std::max(maxH, v);
        }

        SceneNode node{};
        node.entity_id = 1;
        node.parent_index = 0;
        node.type = EntityType::TYPE_TERRAIN_BASE;
        node.entity_flags = 0;
        node.position[0] = static_cast<float>(chunkX) * 250.0f;
        node.position[1] = 0.0f;
        node.position[2] = static_cast<float>(chunkZ) * 250.0f;
        node.rotation_quat[0] = 1.0f;
        node.rotation_quat[1] = 0.0f;
        node.rotation_quat[2] = 0.0f;
        node.rotation_quat[3] = 0.0f;
        node.scale[0] = 250.0f;
        node.scale[1] = 1.0f;
        node.scale[2] = 250.0f;
        node.specific_data_offset = 0;
        std::snprintf(node.name, sizeof(node.name), "TER_chunk_%d_%d", chunkX, chunkZ);

        const std::string sourceTag = sourceHeightmap.generic_string();
        node.asset_reference_id = static_cast<uint32_t>(std::hash<std::string>{}(sourceTag));

        TerrainMetaData terrainMeta{};
        terrainMeta.collision_mesh_id = static_cast<uint32_t>(std::hash<std::string>{}(sourceTag + "#collision"));
        terrainMeta.detail_mesh_id = static_cast<uint32_t>(std::hash<std::string>{}(sourceTag + "#detail"));
        terrainMeta.uv_scale = 1.0f;
        std::snprintf(terrainMeta.internal_mesh_name, sizeof(terrainMeta.internal_mesh_name), "HM_%dx%d_%u_%u",
                      outWidth,
                      outHeight,
                      static_cast<unsigned int>(minH),
                      static_cast<unsigned int>(maxH));

        SceneFileHeader header{};
        header.version = 100;
        header.flags = 0;
        header.data_checksum = 0;
        header.map_id = 1;
        header.chunk_x = static_cast<uint32_t>(chunkX);
        header.chunk_y = static_cast<uint32_t>(chunkZ);

        uint64_t currentOffset = sizeof(SceneFileHeader);
        const int nodesSection = static_cast<int>(SceneSectionType::SCENE_SECTION_NODES);
        const int terrainDataSection = static_cast<int>(SceneSectionType::SCENE_SECTION_TERRAIN_DATA);
        const int terrainMetaSection = static_cast<int>(SceneSectionType::SCENE_SECTION_TERRAIN_METADATA);

        header.sections[nodesSection].count = 1;
        header.sections[nodesSection].offset = currentOffset;
        currentOffset += sizeof(SceneNode);

        TerrainHeightmapDataHeader terrainData{};
        terrainData.width = static_cast<uint32_t>(outWidth);
        terrainData.height = static_cast<uint32_t>(outHeight);
        terrainData.bit_depth = static_cast<uint16_t>(bitDepth > 0 ? bitDepth : 16);
        terrainData.sample_format = 1;
        terrainData.min_height_m = minHeightMeters;
        terrainData.max_height_m = maxHeightMeters;
        terrainData.zero_level_m = zeroLevelMeters;
        terrainData.source_min_raw = minH;
        terrainData.source_max_raw = maxH;
        terrainData.sample_count = static_cast<uint32_t>(samples.size());
        std::snprintf(terrainData.normalization_mode, sizeof(terrainData.normalization_mode), "%s", normalizationMode.c_str());

        header.sections[terrainDataSection].count = 1;
        header.sections[terrainDataSection].offset = currentOffset;
        currentOffset += sizeof(TerrainHeightmapDataHeader);

        terrainData.samples_offset = currentOffset;
        currentOffset += samples.size() * sizeof(uint16_t);

        header.sections[terrainMetaSection].count = 1;
        header.sections[terrainMetaSection].offset = currentOffset;
        currentOffset += sizeof(TerrainMetaData);

        header.fileSize = currentOffset;

        std::ofstream out(outputMmap, std::ios::binary);
        if (!out.is_open()) {
            outError = "Nao foi possivel abrir arquivo de saida .mmap.";
            return false;
        }

        out.write(reinterpret_cast<const char*>(&header), sizeof(SceneFileHeader));
        out.write(reinterpret_cast<const char*>(&node), sizeof(SceneNode));
        out.write(reinterpret_cast<const char*>(&terrainData), sizeof(TerrainHeightmapDataHeader));
        out.write(reinterpret_cast<const char*>(samples.data()), static_cast<std::streamsize>(samples.size() * sizeof(uint16_t)));
        out.write(reinterpret_cast<const char*>(&terrainMeta), sizeof(TerrainMetaData));
        out.close();

        if (!IsMmapFileSignatureValid(outputMmap)) {
            outError = "Arquivo .mmap gerado sem assinatura valida.";
            return false;
        }

        return true;
    }

    bool PersistChunkMetadata(
        const std::filesystem::path& metadataPath,
        int chunkX,
        int chunkZ,
        const std::string& sourcePath,
        const std::string& sourceType,
        const std::string& conversionVersion,
        const std::string& mmapPath,
        int heightmapWidth,
        int heightmapHeight,
        float minHeightMeters,
        float maxHeightMeters,
        float zeroLevelMeters,
        int bitDepth,
        const std::string& normalizationMode,
        const std::string& notes) {
        std::ofstream out(metadataPath, std::ios::trunc);
        if (!out.is_open()) {
            return false;
        }

        out << "{\n";
        out << "  \"chunk\": { \"x\": " << chunkX << ", \"z\": " << chunkZ << " },\n";
        out << "  \"source_path\": \"" << JsonEscape(sourcePath) << "\",\n";
        out << "  \"source_type\": \"" << JsonEscape(sourceType) << "\",\n";
        out << "  \"imported_at_utc\": \"" << ToIso8601UtcNow() << "\",\n";
        out << "  \"conversion_version\": \"" << JsonEscape(conversionVersion) << "\",\n";
        out << "  \"mmap_path\": \"" << JsonEscape(mmapPath) << "\",\n";
        out << "  \"terrain_vertical\": { \"min_height_m\": " << minHeightMeters
            << ", \"max_height_m\": " << maxHeightMeters
            << ", \"zero_level_m\": " << zeroLevelMeters
            << ", \"height_range_m\": " << (maxHeightMeters - minHeightMeters)
            << " },\n";
        out << "  \"height_encoding\": { \"bit_depth\": " << bitDepth
            << ", \"normalization_mode\": \"" << JsonEscape(normalizationMode) << "\" },\n";
        out << "  \"heightmap\": { \"width\": " << heightmapWidth << ", \"height\": " << heightmapHeight << " },\n";
        out << "  \"notes\": \"" << JsonEscape(notes) << "\"\n";
        out << "}\n";

        return true;
    }
}

EditorApp::EditorApp() {
    Engine::Core::Log::Info("[Editor] Construtor chamado");
    std::strncpy(m_sceneNameBuffer, m_sceneName.c_str(), sizeof(m_sceneNameBuffer));
    m_sceneNameBuffer[sizeof(m_sceneNameBuffer) - 1] = '\0';
    ensureProjectStructure();
    const std::filesystem::path terrainProfilePath = Engine::WorldPartition::ResolveTerrainProfilePath(std::filesystem::current_path());
    m_terrainProfile = Engine::WorldPartition::LoadTerrainProfileFromFile(terrainProfilePath);
    m_worldPartition.setGridConfig(m_terrainProfile.grid);
    loadSceneDefinition();
    initWindow();
    initImGui();
    m_isInitialized = true;
}

EditorApp::~EditorApp() {
    shutdownImGui();
    Engine::Core::Log::Info("[Editor] Destrutor chamado");
}

void EditorApp::initWindow() {
    Engine::WindowConfig config;
    config.width = 1600;
    config.height = 900;
    config.title = kEditorTitle;
    config.resizable = true;
    config.fullscreen = false;
    config.maximized = true;

    m_window = std::make_unique<Engine::Window>(config);
    Engine::Core::Log::Info("[Editor] Janela criada");
}

void EditorApp::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    setupStyle();

    ImGui_ImplGlfw_InitForOpenGL(m_window->getGLFWWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 450");

    Engine::Core::Log::Info("[Editor] ImGui inicializado");
}

void EditorApp::setupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ApplyEditorTheme(style, m_visualStyle);
    Engine::Core::Log::Info("[Editor] Estilo visual do editor configurado");
}

void EditorApp::shutdownImGui() {
    if (!m_isInitialized) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    Engine::Core::Log::Info("[Editor] ImGui finalizado");
}

void EditorApp::run() {
    Engine::Core::Log::Info("[Editor] Iniciando loop principal");

    while (!m_window->shouldClose()) {
        if (m_themeDirty) {
            setupStyle();
            m_themeDirty = false;
        }

        glClearColor(0.07f, 0.08f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderMenuBar();
        renderMainContent();
        renderStatusBar();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        m_window->swapBuffersAndPollEvents();
    }

    Engine::Core::Log::Info("[Editor] Loop principal encerrado");
}

void EditorApp::updateWorldPartitionFromCamera(const glm::vec3& cameraWorldPosition) {
    m_worldPartition.updateFromCamera(cameraWorldPosition);
}

std::string EditorApp::getActiveChunkLabel() const {
    return m_worldPartition.activeChunkLabel();
}

int EditorApp::getLoadedChunkCount() const {
    return m_worldPartition.loadedChunkCount();
}

bool EditorApp::isObjectVisibleInActiveChunk(const std::string& objectId) const {
    const auto posIt = m_objectPositions.find(objectId);
    if (posIt == m_objectPositions.end()) {
        return true;
    }

    const Engine::WorldPartition::ChunkCoord objectChunk =
        m_worldPartition.resolveChunkFromWorldPosition(posIt->second);
    return m_worldPartition.isObjectInsideActiveChunk(objectChunk);
}

float EditorApp::terrainMinHeightMeters() const {
    return m_terrainProfile.minHeightMeters;
}

float EditorApp::terrainMaxHeightMeters() const {
    return m_terrainProfile.maxHeightMeters;
}

float EditorApp::terrainZeroLevelMeters() const {
    return m_terrainProfile.zeroLevelMeters;
}

int EditorApp::terrainHeightBitDepth() const {
    return m_terrainProfile.bitDepth;
}

std::string EditorApp::terrainNormalizationMode() const {
    return m_terrainProfile.normalizationMode;
}

bool EditorApp::doesChunkHaveCompiledMap(int chunkX, int chunkZ) const {
    const std::filesystem::path chunkPath = std::filesystem::current_path() / m_projectRootPath / "terrains" / "chunks" /
        std::format("chunk_{}_{}.mmap", chunkX, chunkZ);
    return std::filesystem::exists(chunkPath) && IsMmapFileSignatureValid(chunkPath);
}

bool EditorApp::setActiveChunkScene(int chunkX, int chunkZ) {
    const std::filesystem::path chunkPath = std::filesystem::current_path() / m_projectRootPath / "terrains" / "chunks" /
        std::format("chunk_{}_{}.mmap", chunkX, chunkZ);
    if (!std::filesystem::exists(chunkPath) || !IsMmapFileSignatureValid(chunkPath)) {
        m_worldEditorStatus = std::format("Celula ({},{}) ainda nao possui .mmap valido.", chunkX, chunkZ);
        return false;
    }

    std::error_code ec;
    const std::filesystem::path rel = std::filesystem::relative(chunkPath, std::filesystem::current_path(), ec);
    m_sceneAssetPath = ec ? chunkPath.generic_string() : rel.generic_string();
    m_sceneLoaded = true;
    m_terrainReady = true;
    m_isEmptyScene = false;
    m_sceneName = std::format("chunk_{}_{}", chunkX, chunkZ);
    std::strncpy(m_sceneNameBuffer, m_sceneName.c_str(), sizeof(m_sceneNameBuffer));
    m_sceneNameBuffer[sizeof(m_sceneNameBuffer) - 1] = '\0';

    const bool loaded = rebuildSceneObjectsFromCurrentMap();
    m_worldEditorStatus = loaded
        ? std::format("Cena ativa alterada para chunk ({},{}).", chunkX, chunkZ)
        : std::format("Falha ao carregar cena do chunk ({},{}).", chunkX, chunkZ);
    return loaded;
}

void EditorApp::importChunkMapForCell(int chunkX, int chunkZ) {
#if defined(_WIN32)
    char fileBuffer[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter =
        "Map Sources (*.mmap;*.glb;*.gltf;*.obj;*.raw;*.png;*.jpg;*.jpeg;*.r16;*.tif;*.tiff)\0*.mmap;*.glb;*.gltf;*.obj;*.raw;*.png;*.jpg;*.jpeg;*.r16;*.tif;*.tiff\0"
        "All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameA(&ofn)) {
        m_worldEditorStatus = "Import de mapa cancelado pelo usuario.";
        return;
    }

    const std::filesystem::path sourcePath = fileBuffer;
    if (!std::filesystem::exists(sourcePath)) {
        m_worldEditorStatus = "Arquivo selecionado nao existe.";
        return;
    }

    const std::filesystem::path chunkDir = std::filesystem::current_path() / m_projectRootPath / "terrains" / "chunks";
    const std::filesystem::path heightmapDir = std::filesystem::current_path() / m_projectRootPath / "assets" / "maps" / "heightmaps";
    const std::filesystem::path modelDir = std::filesystem::current_path() / m_projectRootPath / "assets" / "models";
    std::error_code ec;
    std::filesystem::create_directories(chunkDir, ec);
    std::filesystem::create_directories(heightmapDir, ec);
    std::filesystem::create_directories(modelDir, ec);

    const std::string chunkStem = std::format("chunk_{}_{}", chunkX, chunkZ);
    const std::string ext = sourcePath.extension().string();
    std::string extLower = ext;
    std::transform(extLower.begin(), extLower.end(), extLower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    const std::filesystem::path chunkMmapPath = chunkDir / (chunkStem + ".mmap");
    const std::filesystem::path chunkMetaPath = chunkDir / (chunkStem + ".meta.json");

    if (extLower == ".mmap") {
        std::filesystem::copy_file(sourcePath, chunkMmapPath, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec || !IsMmapFileSignatureValid(chunkMmapPath)) {
            m_worldEditorStatus = "Falha ao importar .mmap para a celula.";
            return;
        }
        PersistChunkMetadata(
            chunkMetaPath,
            chunkX,
            chunkZ,
            sourcePath.generic_string(),
            "mmap",
            "passthrough_mmap_v1",
            chunkMmapPath.generic_string(),
            0,
            0,
            terrainMinHeightMeters(),
            terrainMaxHeightMeters(),
            terrainZeroLevelMeters(),
            terrainHeightBitDepth(),
            terrainNormalizationMode(),
            "Arquivo .mmap importado diretamente para o chunk.");
        setActiveChunkScene(chunkX, chunkZ);
        return;
    }

    if (extLower == ".glb" || extLower == ".gltf" || extLower == ".obj") {
        const std::filesystem::path importedModel = modelDir / (chunkStem + extLower);
        std::filesystem::copy_file(sourcePath, importedModel, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            m_worldEditorStatus = "Falha ao copiar modelo fonte para project/assets/models.";
            return;
        }

        std::error_code relEc;
        const std::filesystem::path importedRel = std::filesystem::relative(importedModel, std::filesystem::current_path(), relEc);
        const std::string importedDisplay = relEc ? importedModel.generic_string() : importedRel.generic_string();
        std::strncpy(m_mmapSourceGlbPathBuffer, importedDisplay.c_str(), sizeof(m_mmapSourceGlbPathBuffer));
        m_mmapSourceGlbPathBuffer[sizeof(m_mmapSourceGlbPathBuffer) - 1] = '\0';

        buildMmapCompiler();
        runMmapCompilerFromSource();

        std::filesystem::path generatedMmap = importedModel;
        generatedMmap.replace_extension(".mmap");
        if (!std::filesystem::exists(generatedMmap) || !IsMmapFileSignatureValid(generatedMmap)) {
            m_worldEditorStatus = "Falha ao converter modelo para .mmap.";
            return;
        }

        std::filesystem::copy_file(generatedMmap, chunkMmapPath, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            m_worldEditorStatus = "Falha ao salvar .mmap convertido no chunk.";
            return;
        }

        PersistChunkMetadata(
            chunkMetaPath,
            chunkX,
            chunkZ,
            sourcePath.generic_string(),
            "model",
            "mmap_compiler_v1",
            chunkMmapPath.generic_string(),
            0,
            0,
            terrainMinHeightMeters(),
            terrainMaxHeightMeters(),
            terrainZeroLevelMeters(),
            terrainHeightBitDepth(),
            terrainNormalizationMode(),
            std::format("Fonte {} convertida via mmap_compiler.", extLower));

        setActiveChunkScene(chunkX, chunkZ);
        return;
    }

    const std::filesystem::path importedHeightmap = heightmapDir / (chunkStem + extLower);
    std::filesystem::copy_file(sourcePath, importedHeightmap, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        m_worldEditorStatus = "Falha ao importar heightmap para project/assets/maps/heightmaps.";
        return;
    }

    std::string conversionError;
    int hmWidth = 0;
    int hmHeight = 0;
    if (!BuildTerrainMmapFromHeightmap(
            importedHeightmap,
            chunkMmapPath,
            chunkX,
            chunkZ,
            terrainMinHeightMeters(),
            terrainMaxHeightMeters(),
            terrainZeroLevelMeters(),
            terrainHeightBitDepth(),
            terrainNormalizationMode(),
            conversionError,
            hmWidth,
            hmHeight)) {
        m_worldEditorStatus = std::format("Falha na conversao de heightmap para .mmap: {}", conversionError);
        return;
    }

    PersistChunkMetadata(
        chunkMetaPath,
        chunkX,
        chunkZ,
        sourcePath.generic_string(),
        "heightmap",
        "heightmap_to_mmap_v1",
        chunkMmapPath.generic_string(),
        hmWidth,
        hmHeight,
        terrainMinHeightMeters(),
        terrainMaxHeightMeters(),
        terrainZeroLevelMeters(),
        terrainHeightBitDepth(),
        terrainNormalizationMode(),
        std::format("Conversao real de heightmap {} para MMAP.", extLower));

    setActiveChunkScene(chunkX, chunkZ);
    m_worldEditorStatus = std::format("Heightmap convertido com sucesso para chunk ({},{}): {}x{}.", chunkX, chunkZ, hmWidth, hmHeight);
#else
    (void)chunkX;
    (void)chunkZ;
    m_worldEditorStatus = "Import de mapa via dialogo disponivel apenas no Windows.";
#endif
}

void EditorApp::ensureProjectStructure() {
    const std::filesystem::path projectRoot = std::filesystem::current_path() / m_projectRootPath;
    std::filesystem::create_directories(projectRoot / "scenes");
    std::filesystem::create_directories(projectRoot / "terrains");
    std::filesystem::create_directories(projectRoot / "assets");
    std::filesystem::create_directories(projectRoot / "assets" / "sky");
    std::filesystem::create_directories(projectRoot / "config");
    ensureProjectAssetFolders();
    Engine::WorldPartition::EnsureTerrainProfileFileExists(
        Engine::WorldPartition::ResolveTerrainProfilePath(std::filesystem::current_path()));

    const std::filesystem::path settingsFile = projectRoot / "config" / "project_settings.json";
    if (!std::filesystem::exists(settingsFile)) {
        std::ofstream out(settingsFile);
        if (out.is_open()) {
            out << "{\n"
                << "  \"project\": {\n"
                << "    \"name\": \"StarterGameProject\",\n"
                << "    \"default_scene\": \"scenes/starter_scene.scene\",\n"
                << "    \"terrains_folder\": \"terrains\",\n"
                << "    \"assets_folder\": \"assets\",\n"
                << "    \"scenes_folder\": \"scenes\",\n"
                << "    \"startup_terrain_size\": 50.0\n"
                << "  },\n"
                << "  \"scene\": {\n"
                << "    \"type\": \"starter\",\n"
                << "    \"ground_size\": 50.0,\n"
                << "    \"ground_height\": 0.0,\n"
                << "    \"player_spawn\": [0.0, 1.0, 0.0],\n"
                << "    \"npc_spawn\": [12.0, 1.0, 6.0]\n"
                << "  }\n"
                << "}\n";
            out.close();
        }
    }

    const std::filesystem::path starterSceneFile = projectRoot / "scenes" / "starter_scene.scene";
    if (!std::filesystem::exists(starterSceneFile)) {
        std::ofstream out(starterSceneFile);
        if (out.is_open()) {
            out << "scene_name: StarterScene\n"
                << "scene_type: starter\n"
                << "ground_size: 50\n"
                << "ground_center: 0,0,0\n"
                << "player_spawn: 0,1,0\n"
                << "npc_spawn: 12,1,6\n"
                << "terrain_source: project/terrains/default_terrain\n"
                << "sky.procedural_enabled: 1\n"
                << "sky.sun_yaw_deg: 35.0\n"
                << "sky.sun_pitch_deg: 22.0\n"
                << "sky.cloud_speed: 0.0035\n"
                << "sky.cloud_direction_deg: 18.0\n"
                << "sky.cloud_coverage: 0.54\n"
                << "sky.cloud_softness: 0.08\n"
                << "sky.cloud_scale: 1.75\n"
                << "sky.cloud_opacity: 0.72\n"
                << "sky.zenith_color: 0.20,0.39,0.70\n"
                << "sky.horizon_color: 0.70,0.78,0.90\n"
                << "sky.cloud_color: 0.92,0.93,0.95\n";
            out.close();
        }
    }

    m_projectReady = true;
    Engine::Core::Log::Info("[Editor] Estrutura mínima do projeto inicializada em: project/");
}

void EditorApp::ensureProjectAssetFolders() {
    const std::filesystem::path assetsRoot = std::filesystem::current_path() / m_projectRootPath / "assets";
    std::filesystem::create_directories(assetsRoot / "audio");
    std::filesystem::create_directories(assetsRoot / "scripts");
    std::filesystem::create_directories(assetsRoot / "maps");
    std::filesystem::create_directories(assetsRoot / "maps" / "heightmaps");
    std::filesystem::create_directories(assetsRoot / "materials");
    std::filesystem::create_directories(assetsRoot / "textures");
    std::filesystem::create_directories(assetsRoot / "particles");
    std::filesystem::create_directories(assetsRoot / "models");
    std::filesystem::create_directories(assetsRoot / "animations");
    std::filesystem::create_directories(assetsRoot / "animations" / "skeletal");
    std::filesystem::create_directories(assetsRoot / "prefabs");
    std::filesystem::create_directories(assetsRoot / "prefabs" / "archetypes");
    std::filesystem::create_directories(assetsRoot / "shaders");
    std::filesystem::create_directories(assetsRoot / "assets");
    std::filesystem::create_directories(assetsRoot / "assets" / "characters");
    std::filesystem::create_directories(assetsRoot / "assets" / "objects");
    std::filesystem::create_directories(assetsRoot / "assets" / "environments");
    std::filesystem::create_directories(assetsRoot / "ui");
    std::filesystem::create_directories(assetsRoot / "ui" / "hud");
    std::filesystem::create_directories(assetsRoot / "misc");

    const std::filesystem::path dataRoot = std::filesystem::current_path() / m_projectRootPath / "data";
    std::filesystem::create_directories(dataRoot / "database");
}

bool EditorApp::rebuildSceneObjectsFromCurrentMap() {
    m_objectPositions.clear();
    m_objectScales.clear();
    m_objectRotationsDeg.clear();

    m_objectPositions["Ground"] = glm::vec3(0.0f, -0.05f, 0.0f);
    m_objectScales["Ground"] = glm::vec3(12.0f, 0.1f, 12.0f);
    m_objectRotationsDeg["Ground"] = glm::vec3(0.0f);

    const std::filesystem::path sceneAsset = std::filesystem::current_path() / m_sceneAssetPath;
    if (!std::filesystem::exists(sceneAsset) || sceneAsset.extension() != ".mmap") {
        if (m_selectedObject.empty() || m_objectPositions.find(m_selectedObject) == m_objectPositions.end()) {
            m_selectedObject = "Ground";
        }
        return false;
    }

    if (!IsMmapFileSignatureValid(sceneAsset)) {
        if (m_selectedObject.empty() || m_objectPositions.find(m_selectedObject) == m_objectPositions.end()) {
            m_selectedObject = "Ground";
        }
        return false;
    }

    Engine::SceneLoader loader;
    if (!loader.loadMapBinary(m_sceneAssetPath)) {
        if (m_selectedObject.empty() || m_objectPositions.find(m_selectedObject) == m_objectPositions.end()) {
            m_selectedObject = "Ground";
        }
        return false;
    }

    std::unordered_map<std::string, int> duplicateCount;
    for (const SceneNode& node : loader.nodes) {
        std::string name = node.name;
        if (name.empty()) {
            name = "Node_" + std::to_string(node.entity_id);
        }

        int& count = duplicateCount[name];
        if (count > 0) {
            name += "_" + std::to_string(count);
        }
        ++count;

        glm::vec3 pos(node.position[0], node.position[1], node.position[2]);
        glm::vec3 scale(node.scale[0], node.scale[1], node.scale[2]);
        if (glm::length(scale) < 0.001f) {
            scale = glm::vec3(1.0f);
        }

        glm::quat q(node.rotation_quat[0], node.rotation_quat[1], node.rotation_quat[2], node.rotation_quat[3]);
        glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(q));

        if (node.type == EntityType::TYPE_TERRAIN_BASE) {
            m_objectPositions["Ground"] = pos;
            m_objectScales["Ground"] = glm::max(glm::vec3(0.1f), scale);
            m_objectRotationsDeg["Ground"] = eulerDeg;
            continue;
        }

        m_objectPositions[name] = pos;
        m_objectScales[name] = glm::max(glm::vec3(0.05f), scale);
        m_objectRotationsDeg[name] = eulerDeg;
    }

    if (m_selectedObject.empty() || m_objectPositions.find(m_selectedObject) == m_objectPositions.end()) {
        auto it = m_objectPositions.begin();
        if (it != m_objectPositions.end()) {
            m_selectedObject = it->first;
        }
    }

    return true;
}

void EditorApp::loadSceneDefinition() {
    const std::filesystem::path projectRoot = std::filesystem::current_path() / m_projectRootPath;
    const std::filesystem::path terrainPath = projectRoot / "terrains" / "default_terrain.mmap";
    const std::filesystem::path fallbackPath = std::filesystem::current_path() / "assets" / "models" / "test_scene_1.mmap";
    const std::filesystem::path sceneFilePath = projectRoot / "scenes" / "starter_scene.scene";

    if (IsMmapFileSignatureValid(terrainPath)) {
        m_sceneAssetPath = "project/terrains/default_terrain.mmap";
        m_sceneDefinitionPath = "project/scenes/starter_scene.scene";
        m_sceneLoaded = true;
        m_terrainReady = true;
        m_isEmptyScene = false;
        m_sceneName = "StarterScene";
        std::strncpy(m_sceneNameBuffer, m_sceneName.c_str(), sizeof(m_sceneNameBuffer));
        m_sceneNameBuffer[sizeof(m_sceneNameBuffer) - 1] = '\0';
        if (rebuildSceneObjectsFromCurrentMap()) {
            loadSkySettingsFromSceneFile(sceneFilePath);
            Engine::Core::Log::Info(std::string("[Editor] Cena carregada a partir do projeto: ") + terrainPath.string());
            return;
        }
        Engine::Core::Log::Warn("[Editor] Arquivo de terreno do projeto existe, mas nao e um MMAP valido. Tentando fallback.");
    }

    if (IsMmapFileSignatureValid(fallbackPath)) {
        m_sceneAssetPath = "assets/models/test_scene_1.mmap";
        m_sceneLoaded = true;
        m_terrainReady = true;
        m_isEmptyScene = false;
        m_sceneName = "TestScene_1";
        std::strncpy(m_sceneNameBuffer, m_sceneName.c_str(), sizeof(m_sceneNameBuffer));
        m_sceneNameBuffer[sizeof(m_sceneNameBuffer) - 1] = '\0';
        rebuildSceneObjectsFromCurrentMap();
        Engine::Core::Log::Info(std::string("[Editor] Cena carregada do fallback de assets: ") + fallbackPath.string());
        return;
    }

    loadStarterScene();
}

void EditorApp::loadStarterScene() {
    m_sceneAssetPath = "project/terrains/default_terrain.mmap";
    m_sceneDefinitionPath = "project/scenes/starter_scene.scene";
    m_sceneLoaded = false;
    m_terrainReady = false;
    m_isEmptyScene = true;
    m_sceneName = "StarterScene";
    std::strncpy(m_sceneNameBuffer, m_sceneName.c_str(), sizeof(m_sceneNameBuffer));
    m_sceneNameBuffer[sizeof(m_sceneNameBuffer) - 1] = '\0';
    rebuildSceneObjectsFromCurrentMap();
    loadSkySettingsFromSceneFile(std::filesystem::current_path() / m_sceneDefinitionPath);
    Engine::Core::Log::Warn("[Editor] Nenhuma cena pronta foi encontrada. Iniciando cena mínima padrão do projeto.");
}

void EditorApp::importTerrainIntoProject() {
    const std::filesystem::path projectRoot = std::filesystem::current_path() / m_projectRootPath;
    const std::filesystem::path terrainFolder = projectRoot / "terrains";
    std::filesystem::create_directories(terrainFolder);

    const std::filesystem::path importedTerrain = terrainFolder / "default_terrain.mmap";
    const std::filesystem::path fallbackBinary = std::filesystem::current_path() / "assets" / "models" / "test_scene_1.mmap";

    std::error_code copyEc;
    if (std::filesystem::exists(fallbackBinary)) {
        std::filesystem::copy_file(fallbackBinary, importedTerrain, std::filesystem::copy_options::overwrite_existing, copyEc);
    }

    if (!std::filesystem::exists(importedTerrain)) {
        Engine::Core::Log::Warn("[Editor] Nao foi possivel preparar default_terrain.mmap a partir do fallback binario.");
    }

    m_sceneAssetPath = "project/terrains/default_terrain.mmap";
    m_sceneLoaded = true;
    m_terrainReady = true;
    m_isEmptyScene = false;
    m_sceneName = "StarterScene";
    std::strncpy(m_sceneNameBuffer, m_sceneName.c_str(), sizeof(m_sceneNameBuffer));
    m_sceneNameBuffer[sizeof(m_sceneNameBuffer) - 1] = '\0';
    rebuildSceneObjectsFromCurrentMap();
    Engine::Core::Log::Info("[Editor] Terreno importado para a estrutura do projeto.");
}

void EditorApp::pickTerrainFromFileBrowser() {
#if defined(_WIN32)
    char fileBuffer[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = "Terrain and Scene Files (*.mmap;*.gltf;*.glb;*.obj)\0*.mmap;*.gltf;*.glb;*.obj\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameA(&ofn)) {
        Engine::Core::Log::Warn("[Editor] Seleção de terreno cancelada pelo usuário.");
        return;
    }

    const std::filesystem::path selectedFile = fileBuffer;
    const std::filesystem::path projectRoot = std::filesystem::current_path() / m_projectRootPath;
    const std::filesystem::path targetFolder = projectRoot / "terrains";
    std::filesystem::create_directories(targetFolder);

    const std::filesystem::path targetFile = targetFolder / selectedFile.filename();
    std::filesystem::copy_file(selectedFile, targetFile, std::filesystem::copy_options::overwrite_existing);

    m_sceneAssetPath = std::string("project/terrains/") + selectedFile.filename().string();
    m_sceneLoaded = true;
    m_terrainReady = true;
    m_isEmptyScene = false;
    m_sceneName = selectedFile.stem().string();
    std::strncpy(m_sceneNameBuffer, m_sceneName.c_str(), sizeof(m_sceneNameBuffer));
    m_sceneNameBuffer[sizeof(m_sceneNameBuffer) - 1] = '\0';
    rebuildSceneObjectsFromCurrentMap();
    Engine::Core::Log::Info(std::string("[Editor] Terreno importado do arquivo: ") + selectedFile.string());
#else
    Engine::Core::Log::Warn("[Editor] File browser de terreno suportado apenas no Windows.");
#endif
}

void EditorApp::pickAndImportAssetFromFileBrowser() {
#if defined(_WIN32)
    char fileBuffer[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter =
        "Supported Assets (*.glb;*.gltf;*.fbx;*.obj;*.png;*.jpg;*.jpeg;*.tga;*.wav;*.ogg;*.mp3;*.mmap;*.raw)\0*.glb;*.gltf;*.fbx;*.obj;*.png;*.jpg;*.jpeg;*.tga;*.wav;*.ogg;*.mp3;*.mmap;*.raw\0"
        "All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameA(&ofn)) {
        m_contentBrowserStatus = "Import cancelado pelo usuario.";
        return;
    }

    importAssetFileToProject(std::filesystem::path(fileBuffer));
#else
    m_contentBrowserStatus = "Import via file dialog disponivel apenas no Windows.";
#endif
}

void EditorApp::importAssetFileToProject(const std::filesystem::path& sourceFile) {
    if (!std::filesystem::exists(sourceFile)) {
        m_contentBrowserStatus = std::string("Erro: arquivo nao encontrado: ") + sourceFile.string();
        return;
    }

    auto toLower = [](std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    };

    const std::string ext = toLower(sourceFile.extension().string());
    const std::string stemLower = toLower(sourceFile.stem().string());

    std::string relativeTargetDir = "project/assets/misc";

    if (ext == ".wav" || ext == ".ogg" || ext == ".mp3" || ext == ".flac") {
        relativeTargetDir = "project/assets/audio";
    } else if (ext == ".lua" || ext == ".py" || ext == ".cs" || ext == ".js") {
        relativeTargetDir = "project/assets/scripts";
    } else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp" || ext == ".dds" || ext == ".hdr" || ext == ".exr") {
        if (stemLower.find("height") != std::string::npos) {
            relativeTargetDir = "project/assets/maps/heightmaps";
        } else {
            relativeTargetDir = "project/assets/textures";
        }
    } else if (ext == ".raw") {
        relativeTargetDir = "project/assets/maps/heightmaps";
    } else if (ext == ".mat" || ext == ".mtl") {
        relativeTargetDir = "project/assets/materials";
    } else if (ext == ".prefab" || ext == ".archetype") {
        relativeTargetDir = "project/assets/prefabs/archetypes";
    } else if (ext == ".anim" || ext == ".bvh" || ext == ".smd") {
        relativeTargetDir = "project/assets/animations/skeletal";
    } else if (ext == ".vert" || ext == ".frag" || ext == ".geom" || ext == ".comp" || ext == ".glsl") {
        relativeTargetDir = "project/assets/shaders";
    } else if (ext == ".particle" || ext == ".pfx" || ext == ".vfx") {
        relativeTargetDir = "project/assets/particles";
    } else if (ext == ".glb" || ext == ".gltf" || ext == ".fbx" || ext == ".obj") {
        relativeTargetDir = "project/assets/assets/objects";
    } else if (ext == ".json" || ext == ".xml" || ext == ".csv") {
        if (stemLower.find("hud") != std::string::npos || stemLower.find("ui") != std::string::npos || stemLower.find("inventory") != std::string::npos || stemLower.find("skill") != std::string::npos || stemLower.find("chat") != std::string::npos) {
            relativeTargetDir = "project/assets/ui/hud";
        } else {
            relativeTargetDir = "project/data/database";
        }
    } else if (ext == ".mmap" || ext == ".scene") {
        relativeTargetDir = "project/assets/maps";
    }

    const std::filesystem::path targetDir = std::filesystem::current_path() / relativeTargetDir;
    std::error_code ec;
    std::filesystem::create_directories(targetDir, ec);
    if (ec) {
        m_contentBrowserStatus = std::string("Erro: falha ao criar pasta destino: ") + targetDir.string();
        return;
    }

    const std::filesystem::path copiedPath = targetDir / sourceFile.filename();
    std::filesystem::copy_file(sourceFile, copiedPath, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        m_contentBrowserStatus = std::string("Erro: falha ao importar asset: ") + ec.message();
        return;
    }

    std::error_code relEc;
    const std::filesystem::path relativeImported = std::filesystem::relative(copiedPath, std::filesystem::current_path(), relEc);
    const std::string relativeImportedText = relEc ? copiedPath.generic_string() : relativeImported.generic_string();

    if (ext == ".glb" || ext == ".gltf" || ext == ".obj") {
        std::strncpy(m_mmapSourceGlbPathBuffer, relativeImportedText.c_str(), sizeof(m_mmapSourceGlbPathBuffer));
        m_mmapSourceGlbPathBuffer[sizeof(m_mmapSourceGlbPathBuffer) - 1] = '\0';
        m_assetPipelineStatus = std::string("Fonte MMAP atualizada para: ") + relativeImportedText;
    }

    m_contentBrowserCurrentRoot = relativeTargetDir;
    m_contentBrowserStatus = std::string("Importado com sucesso para ") + relativeTargetDir;
    Engine::Core::Log::Info(std::string("[Editor][Content] Asset importado: ") + relativeImportedText);
}

void EditorApp::saveAllEditorChanges() {
    saveSkySettingsToSceneFile(std::filesystem::current_path() / m_sceneDefinitionPath);
    m_contentBrowserStatus = "Save All concluido: cena e sky persistidos.";
    Engine::Core::Log::Info("[Editor][Content] Save All concluido");
}

void EditorApp::buildDictionaryCompiler() {
    const std::string repoRoot = std::filesystem::current_path().string();
    const std::string cmakeExe = GetCMakeExecutable();

    const int configureCode = RunCommandAtRepoRoot(
        repoRoot,
        cmakeExe + " -S tools/dictionary_compiler -B tools/dictionary_compiler/build -G \"Visual Studio 17 2022\"");
    if (configureCode != 0) {
        setAssetPipelineStatus("Erro: falha ao configurar dictionary_compiler.");
        Engine::Core::Log::Error("[Editor] Falha ao configurar dictionary_compiler");
        return;
    }

    const int buildCode = RunCommandAtRepoRoot(
        repoRoot,
        cmakeExe + " --build tools/dictionary_compiler/build --config Debug");
    if (buildCode != 0) {
        setAssetPipelineStatus("Erro: falha ao compilar dictionary_compiler.");
        Engine::Core::Log::Error("[Editor] Falha ao compilar dictionary_compiler");
        return;
    }

    setAssetPipelineStatus("Dictionary compiler compilado com sucesso.");
    Engine::Core::Log::Info("[Editor] Dictionary compiler compilado com sucesso");
}

void EditorApp::runDictionaryCompiler() {
    const std::filesystem::path debugExe = std::filesystem::current_path() / "tools" / "dictionary_compiler" / "build" / "Debug" / "dictionary_compiler.exe";
    const std::filesystem::path releaseExe = std::filesystem::current_path() / "tools" / "dictionary_compiler" / "build" / "Release" / "dictionary_compiler.exe";

    std::filesystem::path exePath;
    if (std::filesystem::exists(debugExe)) {
        exePath = debugExe;
    } else if (std::filesystem::exists(releaseExe)) {
        exePath = releaseExe;
    }

    if (exePath.empty()) {
        setAssetPipelineStatus("Aviso: dictionary_compiler.exe nao encontrado. Compile o tool primeiro.");
        Engine::Core::Log::Warn("[Editor] dictionary_compiler.exe nao encontrado. Compile o tool primeiro.");
        return;
    }

    const int runCode = RunCommandAtRepoRoot(std::filesystem::current_path().string(), "\"" + exePath.generic_string() + "\"");
    if (runCode != 0) {
        setAssetPipelineStatus("Erro: falha ao executar dictionary_compiler.");
        Engine::Core::Log::Error("[Editor] Falha ao executar dictionary_compiler");
        return;
    }

    setAssetPipelineStatus("Dictionary gerado em data/assets_dictionary.bin.");
    Engine::Core::Log::Info("[Editor] Dictionary binario atualizado em data/assets_dictionary.bin");
}

void EditorApp::buildMmapCompiler() {
    const std::string repoRoot = std::filesystem::current_path().string();
    const std::string cmakeExe = GetCMakeExecutable();

    const int configureCode = RunCommandAtRepoRoot(
        repoRoot,
        cmakeExe + " -S tools/mmap_compiler -B tools/mmap_compiler/build -G \"Visual Studio 17 2022\"");
    if (configureCode != 0) {
        setAssetPipelineStatus("Erro: falha ao configurar mmap_compiler.");
        Engine::Core::Log::Error("[Editor] Falha ao configurar mmap_compiler");
        return;
    }

    const int buildCode = RunCommandAtRepoRoot(
        repoRoot,
        cmakeExe + " --build tools/mmap_compiler/build --config Debug");
    if (buildCode != 0) {
        setAssetPipelineStatus("Erro: falha ao compilar mmap_compiler.");
        Engine::Core::Log::Error("[Editor] Falha ao compilar mmap_compiler");
        return;
    }

    setAssetPipelineStatus("MMAP compiler compilado com sucesso.");
    Engine::Core::Log::Info("[Editor] MMAP compiler compilado com sucesso");
}

void EditorApp::runMmapCompilerFromSource() {
    const std::filesystem::path sourceGlbPath = std::filesystem::current_path() / std::filesystem::path(m_mmapSourceGlbPathBuffer);
    if (!std::filesystem::exists(sourceGlbPath)) {
        setAssetPipelineStatus(std::string("Erro: GLB de origem nao encontrado: ") + sourceGlbPath.string());
        Engine::Core::Log::Warn(std::string("[Editor] GLB de origem nao encontrado: ") + sourceGlbPath.string());
        return;
    }

    const std::filesystem::path debugExe = std::filesystem::current_path() / "tools" / "mmap_compiler" / "build" / "Debug" / "mmap_compiler.exe";
    const std::filesystem::path releaseExe = std::filesystem::current_path() / "tools" / "mmap_compiler" / "build" / "Release" / "mmap_compiler.exe";

    std::filesystem::path exePath;
    if (std::filesystem::exists(debugExe)) {
        exePath = debugExe;
    } else if (std::filesystem::exists(releaseExe)) {
        exePath = releaseExe;
    }

    if (exePath.empty()) {
        setAssetPipelineStatus("Aviso: mmap_compiler.exe nao encontrado. Compile o tool primeiro.");
        Engine::Core::Log::Warn("[Editor] mmap_compiler.exe nao encontrado. Compile o tool primeiro.");
        return;
    }

    const std::string command = "\"" + exePath.generic_string() + "\" \"" + sourceGlbPath.generic_string() + "\"";
    const int runCode = RunCommandAtRepoRoot(std::filesystem::current_path().string(), command);
    if (runCode != 0) {
        setAssetPipelineStatus("Erro: falha ao executar mmap_compiler.");
        Engine::Core::Log::Error("[Editor] Falha ao executar mmap_compiler");
        return;
    }

    std::filesystem::path outputMmap = sourceGlbPath;
    outputMmap.replace_extension(".mmap");
    if (std::filesystem::exists(outputMmap)) {
        setAssetPipelineStatus(std::string("MMAP gerado com sucesso: ") + outputMmap.string());
        Engine::Core::Log::Info(std::string("[Editor] MMAP gerado: ") + outputMmap.string());
    } else {
        setAssetPipelineStatus("Aviso: execucao concluida, mas .mmap esperado nao foi encontrado.");
        Engine::Core::Log::Warn("[Editor] Execucao concluida, mas nao foi possivel localizar o .mmap esperado.");
    }
}

void EditorApp::pickMmapSourceGlbFromFileBrowser() {
#if defined(_WIN32)
    char fileBuffer[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = "GLB Files (*.glb)\0*.glb\0GLTF/OBJ Files (*.gltf;*.obj)\0*.gltf;*.obj\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameA(&ofn)) {
        setAssetPipelineStatus("Selecao de GLB cancelada pelo usuario.");
        return;
    }

    const std::filesystem::path selectedPath = fileBuffer;
    std::error_code ec;
    const std::filesystem::path relativePath = std::filesystem::relative(selectedPath, std::filesystem::current_path(), ec);
    const std::string displayPath = ec ? selectedPath.generic_string() : relativePath.generic_string();

    std::strncpy(m_mmapSourceGlbPathBuffer, displayPath.c_str(), sizeof(m_mmapSourceGlbPathBuffer));
    m_mmapSourceGlbPathBuffer[sizeof(m_mmapSourceGlbPathBuffer) - 1] = '\0';
    setAssetPipelineStatus(std::string("GLB selecionado: ") + displayPath);
#else
    setAssetPipelineStatus("File picker de GLB disponivel apenas no Windows.");
#endif
}

bool EditorApp::copyFileToProjectModels(const std::filesystem::path& sourceFile, std::filesystem::path& importedFileOut) {
    const std::filesystem::path targetDir = std::filesystem::current_path() / m_projectRootPath / "assets" / "models";
    std::error_code ec;
    std::filesystem::create_directories(targetDir, ec);
    if (ec) {
        return false;
    }

    importedFileOut = targetDir / sourceFile.filename();
    std::filesystem::copy_file(sourceFile, importedFileOut, std::filesystem::copy_options::overwrite_existing, ec);
    return !ec;
}

bool EditorApp::copyGeneratedMmapToProjectTerrains(const std::filesystem::path& generatedMmapPath, std::filesystem::path& copiedMmapOut) {
    if (!std::filesystem::exists(generatedMmapPath)) {
        return false;
    }

    const std::filesystem::path targetDir = std::filesystem::current_path() / m_projectRootPath / "terrains";
    std::error_code ec;
    std::filesystem::create_directories(targetDir, ec);
    if (ec) {
        return false;
    }

    copiedMmapOut = targetDir / generatedMmapPath.filename();
    std::filesystem::copy_file(generatedMmapPath, copiedMmapOut, std::filesystem::copy_options::overwrite_existing, ec);
    return !ec;
}

void EditorApp::runMmapImportAndCompilePipeline() {
    const std::filesystem::path selectedPath = std::filesystem::current_path() / std::filesystem::path(m_mmapSourceGlbPathBuffer);
    if (!std::filesystem::exists(selectedPath)) {
        setAssetPipelineStatus("Erro: selecione um GLB valido antes de executar o pipeline.");
        return;
    }

    std::filesystem::path importedGlb;
    if (!copyFileToProjectModels(selectedPath, importedGlb)) {
        setAssetPipelineStatus("Erro: falha ao importar GLB para project/assets/models.");
        return;
    }

    std::error_code relEc;
    const std::filesystem::path importedRel = std::filesystem::relative(importedGlb, std::filesystem::current_path(), relEc);
    const std::string importedDisplay = relEc ? importedGlb.generic_string() : importedRel.generic_string();
    std::strncpy(m_mmapSourceGlbPathBuffer, importedDisplay.c_str(), sizeof(m_mmapSourceGlbPathBuffer));
    m_mmapSourceGlbPathBuffer[sizeof(m_mmapSourceGlbPathBuffer) - 1] = '\0';

    buildMmapCompiler();
    runMmapCompilerFromSource();

    std::filesystem::path generatedMmap = importedGlb;
    generatedMmap.replace_extension(".mmap");
    std::filesystem::path copiedMmap;
    if (!copyGeneratedMmapToProjectTerrains(generatedMmap, copiedMmap)) {
        setAssetPipelineStatus("Erro: MMAP compilado, mas falha ao copiar para project/terrains.");
        return;
    }

    const std::filesystem::path copiedRel = std::filesystem::relative(copiedMmap, std::filesystem::current_path());
    m_sceneAssetPath = copiedRel.generic_string();
    setAssetPipelineStatus(std::string("Pipeline concluido: GLB importado e MMAP disponivel em ") + copiedRel.generic_string());
}

void EditorApp::setAssetPipelineStatus(const std::string& statusMessage) {
    m_assetPipelineStatus = statusMessage;
    Engine::Core::Log::Info(std::string("[Editor][AssetPipeline] ") + statusMessage);
}

void EditorApp::loadSkySettingsFromSceneFile(const std::filesystem::path& sceneFilePath) {
    std::ifstream in(sceneFilePath);
    if (!in.is_open()) {
        Engine::Core::Log::Warn(std::string("[Editor] Nao foi possivel abrir arquivo de cena para carregar sky: ") + sceneFilePath.string());
        return;
    }

    const bool isSceneFile = (sceneFilePath.extension() == ".scene");
    Editor::SkySerialization::SkyData skyData;
    skyData.proceduralEnabled = m_sky.proceduralEnabled;
    skyData.sunYawDeg = m_sky.sunYawDeg;
    skyData.sunPitchDeg = m_sky.sunPitchDeg;
    skyData.cloudSpeed = m_sky.cloudSpeed;
    skyData.cloudDirectionDeg = m_sky.cloudDirectionDeg;
    skyData.cloudCoverage = m_sky.cloudCoverage;
    skyData.cloudSoftness = m_sky.cloudSoftness;
    skyData.cloudScale = m_sky.cloudScale;
    skyData.cloudOpacity = m_sky.cloudOpacity;
    skyData.zenithColor = {m_sky.zenithColor.r, m_sky.zenithColor.g, m_sky.zenithColor.b};
    skyData.horizonColor = {m_sky.horizonColor.r, m_sky.horizonColor.g, m_sky.horizonColor.b};
    skyData.cloudColor = {m_sky.cloudColor.r, m_sky.cloudColor.g, m_sky.cloudColor.b};
    skyData.assetProfile = m_activeSkyProfilePath;
    std::string referencedSkyProfilePath;

    std::string line;
    while (std::getline(in, line)) {
        std::string key;
        std::string value;
        if (!Editor::SkySerialization::ParseLineKeyValue(line, key, value)) {
            continue;
        }

        if (key == "sky.asset_profile") {
            referencedSkyProfilePath = value;
        }

        if (Editor::SkySerialization::IsSkyKey(key)) {
            Editor::SkySerialization::ApplySkyField(skyData, key, value);
        }
    }

    m_sky.proceduralEnabled = skyData.proceduralEnabled;
    m_sky.sunYawDeg = skyData.sunYawDeg;
    m_sky.sunPitchDeg = skyData.sunPitchDeg;
    m_sky.cloudSpeed = skyData.cloudSpeed;
    m_sky.cloudDirectionDeg = skyData.cloudDirectionDeg;
    m_sky.cloudCoverage = skyData.cloudCoverage;
    m_sky.cloudSoftness = skyData.cloudSoftness;
    m_sky.cloudScale = skyData.cloudScale;
    m_sky.cloudOpacity = skyData.cloudOpacity;
    m_sky.zenithColor = glm::vec3(skyData.zenithColor.r, skyData.zenithColor.g, skyData.zenithColor.b);
    m_sky.horizonColor = glm::vec3(skyData.horizonColor.r, skyData.horizonColor.g, skyData.horizonColor.b);
    m_sky.cloudColor = glm::vec3(skyData.cloudColor.r, skyData.cloudColor.g, skyData.cloudColor.b);

    if (isSceneFile) {
        if (!referencedSkyProfilePath.empty()) {
            const std::filesystem::path profilePath = std::filesystem::current_path() / referencedSkyProfilePath;
            if (std::filesystem::exists(profilePath)) {
                loadSkySettingsFromSceneFile(profilePath);
                m_activeSkyProfilePath = ToGenericPathString(std::filesystem::path(referencedSkyProfilePath));
            } else {
                Engine::Core::Log::Warn(std::string("[Editor] Sky asset_profile referenciado nao encontrado: ") + referencedSkyProfilePath);
                m_activeSkyProfilePath.clear();
            }
        } else {
            m_activeSkyProfilePath.clear();
        }
    }

    Engine::Core::Log::Info(std::string("[Editor] Sky carregado da cena: ") + sceneFilePath.string());
}

void EditorApp::saveSkySettingsToSceneFile(const std::filesystem::path& sceneFilePath) const {
    std::vector<std::string> preservedLines;

    {
        std::ifstream in(sceneFilePath);
        if (in.is_open()) {
            std::string line;
            while (std::getline(in, line)) {
                const std::string trimmed = TrimCopy(line);
                if (Editor::SkySerialization::IsSkyKey(trimmed)) {
                    continue;
                }
                preservedLines.push_back(line);
            }
        }
    }

    const std::filesystem::path parentPath = sceneFilePath.parent_path();
    if (!parentPath.empty()) {
        std::filesystem::create_directories(parentPath);
    }

    std::ofstream out(sceneFilePath, std::ios::trunc);
    if (!out.is_open()) {
        Engine::Core::Log::Error(std::string("[Editor] Falha ao salvar sky no arquivo de cena: ") + sceneFilePath.string());
        return;
    }

    for (const std::string& line : preservedLines) {
        out << line << "\n";
    }

    Editor::SkySerialization::SkyData skyData;
    skyData.proceduralEnabled = m_sky.proceduralEnabled;
    skyData.sunYawDeg = m_sky.sunYawDeg;
    skyData.sunPitchDeg = m_sky.sunPitchDeg;
    skyData.cloudSpeed = m_sky.cloudSpeed;
    skyData.cloudDirectionDeg = m_sky.cloudDirectionDeg;
    skyData.cloudCoverage = m_sky.cloudCoverage;
    skyData.cloudSoftness = m_sky.cloudSoftness;
    skyData.cloudScale = m_sky.cloudScale;
    skyData.cloudOpacity = m_sky.cloudOpacity;
    skyData.zenithColor = {m_sky.zenithColor.r, m_sky.zenithColor.g, m_sky.zenithColor.b};
    skyData.horizonColor = {m_sky.horizonColor.r, m_sky.horizonColor.g, m_sky.horizonColor.b};
    skyData.cloudColor = {m_sky.cloudColor.r, m_sky.cloudColor.g, m_sky.cloudColor.b};
    skyData.assetProfile = m_activeSkyProfilePath;
    const bool includeAssetProfile = (sceneFilePath.extension() == ".scene");
    const std::vector<std::string> skyLines = Editor::SkySerialization::BuildSkyLines(skyData, includeAssetProfile);
    for (const std::string& line : skyLines) {
        out << line << "\n";
    }

    Engine::Core::Log::Info(std::string("[Editor] Sky salvo no arquivo de cena: ") + sceneFilePath.string());
}

bool EditorApp::createRenderTexture(unsigned int& textureId, unsigned int& framebufferId, unsigned int& depthId, int width, int height) {
    if (width <= 0 || height <= 0) {
        return false;
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenRenderbuffers(1, &depthId);
    glBindRenderbuffer(GL_RENDERBUFFER, depthId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

    glGenFramebuffers(1, &framebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthId);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return status == GL_FRAMEBUFFER_COMPLETE;
}

void EditorApp::ensureSceneFramebuffer(int targetWidth, int targetHeight) {
    targetWidth = std::max(targetWidth, 64);
    targetHeight = std::max(targetHeight, 64);

    if (m_sceneFramebuffer == 0 || m_sceneFramebufferWidth != targetWidth || m_sceneFramebufferHeight != targetHeight) {
        if (m_sceneFramebuffer != 0) {
            glDeleteFramebuffers(1, &m_sceneFramebuffer);
            m_sceneFramebuffer = 0;
        }
        if (m_sceneColorTexture != 0) {
            glDeleteTextures(1, &m_sceneColorTexture);
            m_sceneColorTexture = 0;
        }
        if (m_sceneDepthRenderbuffer != 0) {
            glDeleteRenderbuffers(1, &m_sceneDepthRenderbuffer);
            m_sceneDepthRenderbuffer = 0;
        }

        const bool ok = createRenderTexture(m_sceneColorTexture, m_sceneFramebuffer, m_sceneDepthRenderbuffer, targetWidth, targetHeight);
        m_sceneFramebufferWidth = ok ? targetWidth : 0;
        m_sceneFramebufferHeight = ok ? targetHeight : 0;
    }
}


