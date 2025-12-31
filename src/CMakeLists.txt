# src/CMakeLists.txt
# Executables: Game Client, Editor, Server

# ============================================================================
# SUB-PROJECTS
# ============================================================================

# Game Client + Editor (uses graphics)
add_subdirectory(client)

# Dedicated Server (no graphics) - FUTURE
add_subdirectory(server)