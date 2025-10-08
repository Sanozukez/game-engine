Compilando o compilador MMAP

PS M:\Dev\game-engine\tools\mmap_compiler\build> cmake --build . --config Debug

Criando o arquivo mmap a partir de um arquivo GLB

PS M:\Dev\game-engine\tools\mmap_compiler\build> .\Debug\mmap_compiler.exe ../../../assets/models/test_scene_1.glb



Compilando o Dictionary Compiler

PS M:\Dev\game-engine\tools\dictionary_compiler\build> cmake --build . --config Debug

Criando o arquivo binário de dicionário

PS M:\Dev\game-engine\tools\dictionary_compiler\build> .\Debug\dictionary_compiler.exe


cmake .. -G "Visual Studio 17 2022" -A x64