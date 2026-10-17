# Proximos Passos - Backlog Atual

Este arquivo foi atualizado para manter somente o que ainda falta.
Tudo que foi concluido hoje foi removido do backlog.

## 1. Proximo passo imediato (prioridade maxima)

Integrar leitura do payload completo de terreno no runtime, consumindo a secao SCENE_SECTION_TERRAIN_DATA do MMAP.

Objetivo:
- usar TerrainHeightmapDataHeader + samples 16-bit no carregamento;
- reconstruir altura de terreno de forma deterministica com min/max/zero do proprio arquivo;
- preparar base para colisao, navmesh e validacao de costura entre chunks.

Entregaveis do passo:
1. Expandir loader para ler secao de terreno (header + amostras).
2. Expor dados de terreno carregados em estrutura reutilizavel de engine.
3. Conectar no fluxo de world/runtime sem dependencia de UI/editor.
4. Logar diagnosticos basicos (width, height, range, sample min/max).
5. Build + smoke test de game-engine e game-engine-editor.

## 2. Pendencias de mundo/chunks

### 2.1 Streaming e dados de chunk
- Conectar carregamento real de chunk no backend de stream (sair do Noop para backend real).
- Definir contrato de entrada/saida de chunk para client e server (API compartilhada).
- Integrar validacao de continuidade de borda entre chunks vizinhos no import/load.

### 2.2 Terreno e gameplay
- Integrar terreno carregado por payload ao sistema de tracking de altura do player.
- Integrar colisao de terreno baseada no height payload.
- Planejar bake incremental de navmesh por chunk.

### 2.3 Ferramentas de debug
- Overlay de debug em runtime: chunk ativo, chunks carregados, fila de load/unload.
- Ferramenta de verificacao de seam entre chunks (altura e normal).

## 3. Pendencias de sky e assets (ainda abertas)

### 3.1 Sky profile na cena
- Garantir fluxo completo de referencia sky.asset_profile na cena com auto-load robusto.
- Melhorar UX do browser de sky com indicador de perfil ativo e acao unica de aplicar + persistir.

### 3.2 Catalogo de assets
- Criar catalogo unico de assets do projeto (assetdb).
- Integrar importador generico com registro automatico no catalogo.
- Expor picker por tipo no Inspector e validacao de referencia quebrada.

## 4. Criterios de pronto do proximo ciclo

- Runtime le e interpreta SCENE_SECTION_TERRAIN_DATA de MMAP gerado pelo editor.
- Alturas reconstruidas batem com a regra global de export definida no terrain profile.
- Sem conflito de configuracao entre engine_settings e terrain_profile_path.
- Build verde para game-engine e game-engine-editor.
