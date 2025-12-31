# src/server/CMakeLists.txt
# Dedicated Server - No graphics

# ============================================================================
# GAME SERVER - Dedicated server (no graphics/UI)
# ============================================================================

# Uncomment when ready to implement server
# add_executable(game-server
#     server_main.cpp
#     server/game_server.cpp
#     server/network/udp_socket.cpp
#     server/network/packet_handler.cpp
#     server/game_logic/world_simulation.cpp
# )
# 
# target_link_libraries(game-server PRIVATE
#     engine  # Core only (no graphics)
#     # Network libs here (e.g., asio, enet)
# )
# 
# # Server does NOT link engine_client (no GLFW, OpenGL, etc)
