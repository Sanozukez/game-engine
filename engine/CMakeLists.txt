# engine/CMakeLists.txt
# Este arquivo define a biblioteca 'engine' que agrupa todos os módulos internos.

add_library(engine STATIC)

# Adiciona subdiretórios.
# A ordem aqui é importante para dependências de targets (ex: core antes de quem o usa).
add_subdirectory(core)
add_subdirectory(input)
add_subdirectory(render)
add_subdirectory(ui)
add_subdirectory(window)
add_subdirectory(geometry)
add_subdirectory(asset)
add_subdirectory(deps)
add_subdirectory(game)

# se todas as fontes .cpp estão sendo adicionadas pelos subdiretórios
# (como engine/core/CMakeLists.txt e engine/input/CMakeLists.txt fazem).
# target_sources(engine
#     PRIVATE
#         # Nenhuma fonte direta da raiz da engine por enquanto.
# )

# Garante C++20 para a biblioteca 'engine'.
target_compile_features(engine PUBLIC cxx_std_20)

# Linka com as bibliotecas de terceiros.
target_link_libraries(engine PUBLIC
    glad   # AGORA É APENAS 'glad', não 'glad::glad'
    glm    # 'glm' é o nome do target, como antes
    imgui  # AGORA É APENAS 'imgui', não 'ImGui::ImGui'
    glfw   # Adicionado GLFW, pois ImGui liga com ele, e a engine pode precisar
)