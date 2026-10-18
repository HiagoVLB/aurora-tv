---
name: aurora-compare-vibeshine
description: "Compara o Aurora atual com o host Nonary/VibeShine para revisar contratos de sessão, catálogo, vídeo, áudio, entrada e bitrate; registra diferenças e atualiza a linha de base do projeto. Use em revisões de compatibilidade ou integração com VibeShine."
---

# Comparar Aurora com VibeShine

O resultado é um diagnóstico entre cliente e host, com versões reproduzíveis,
evidências dos dois lados e próximos passos para webOS. Não comparar os projetos
como se fossem dois clientes equivalentes.

## Encontrar o projeto e a referência

Localize a raiz Aurora pelo `AGENTS.md` e `CMakeLists.txt`; não dependa do cwd.
Leia [AGENTS.md](../../../AGENTS.md) e [CURRENT_STATE.md](../../../docs/CURRENT_STATE.md).
Use [VIBESHINE_INTEGRATION.md](../../../docs/VIBESHINE_INTEGRATION.md) para o
histórico da integração e a seção VibeShine de
[NONARY_MOONLIGHT_WEBOS_REVIEW.md](../../../docs/NONARY_MOONLIGHT_WEBOS_REVIEW.md)
quando o tema envolver HDR, sessões remotas ou captura.

A referência usual é `../vibeshine`, repositório
`https://github.com/Nonary/vibeshine`. Aceite outro checkout indicado pelo usuário.
Se ausente, use um clone isolado em `.cache/references/vibeshine`; não substitua
um checkout existente. Quando o pedido exigir a versão remota mais recente,
verifique a branch padrão/SHA no remoto e obtenha esse commit em um checkout
isolado. Caso contrário, compare o checkout disponível e informe que não houve
verificação de atualização remota. Uma referência antiga registrada no estado
atual não significa que continua sendo o HEAD remoto.

Na raiz Aurora, capture o estado real, incluindo mudanças locais:

```sh
python3 scripts/dev/comparison_snapshot.py --reference vibeshine=../vibeshine --output .cache/comparisons/vibeshine-state.json
```

Ajuste o caminho do checkout. O helper é local, não acessa a rede e registra
HEAD, hashes de diffs, arquivos novos e submódulos. Omitir arquivos sem commit
produziria uma descrição incorreta da versão atual.

## Rastrear o comportamento

Selecione os fluxos pedidos e siga UI/configuração → launch/resume/SDP → decisão
do host → encoder/captura → resposta → efeito no cliente. Use `rg` por símbolos
e parâmetros; um diff textual entre C e C++ não demonstra compatibilidade.

| Tema | Aurora | VibeShine |
| --- | --- | --- |
| Catálogo, UUID, XML e 410 | `core/libgamestream/src/{xml,client}.c`, `src/app/ui/launcher/apps.controller.c` | `src/nvhttp.cpp`, `src/remote_session.{h,cpp}`, `src/process.cpp` |
| Desconectar, encerrar, monitor retido | `src/app/stream/session{,_worker}.c`, `src/app/ui/streaming/streaming.controller.c` | `src/stream.cpp`, `src/rtsp.cpp`, `src/remote_display_topology.cpp` |
| HDR, Main10, cor e modo do host | `src/app/stream/session.c`, `src/app/app_settings.c` | `src/hdr_request_policy.h`, `src/rtsp.{h,cpp}`, `src/video_colorspace.cpp`, preferências de dispositivo/aplicação em `nvhttp.cpp`/`process.cpp` |
| FPS, captura e filas | `src/app/stream/video/`, `third_party/ss4s/modules/webos/` | `src/framegen_policy.h`, `src/config.h`, `src/platform/windows/display_wgc.cpp`, `wgc_capture_policy.h`, `display_vram.cpp` |
| ABR | `src/app/stream/adaptive_bitrate.c`, `core/libgamestream/src/client.c` | `/bitrate` e `/api/abr/capabilities` em `src/nvhttp.cpp` |

Verifique defaults, precedência de aplicação/dispositivo/global, permissões,
casos de erro e retomada. Estes pontos já causaram conclusões incompletas:

- `hdrMode=1` também pode negociar Main10 SDR; siga `prefer_10bit_sdr`,
  `effective_hdr_requested` e `effective_10bit_sdr_requested` até a cor final.
- `currentgame=0` pode ser uma projeção para cliente secundário. Títulos iguais
  podem ter UUIDs/papéis distintos. Remote Input e Remote Monitor têm ciclos,
  captura e áudio diferentes; verifique a versão examinada.
- HTTP 200 não garante sucesso XML. XML 410 pode indicar conclusão ou pedido
  de confirmação. Não repetir uma ação destrutiva para "testar" uma hipótese.
- Refresh de monitor virtual/captura do host, FPS do stream e refresh do painel
  LG são grandezas diferentes. `clientVrrRequested` não prova VRR na TV.
- Preferências administrativas do host não usam automaticamente a autenticação
  de pareamento do GameStream.

## Registrar conclusão e validação

Para cada achado, apresente comportamento Aurora, regra do host, evidência por
arquivo/símbolo/commit, impacto no webOS e ação proposta. Identifique se é bug
demonstrado, diferença intencional, configuração do host ou hipótese pendente.
Confirme no código as premissas dos documentos anteriores; corrija conclusões
superadas e mantenha a referência ao que mudou.

Escolha testes existentes pertinentes: fixtures do Aurora em
`tests/core/run_vibeshine_protocol.sh`; componentes VibeShine em
`tests/unit/test_remote_session.cpp`, `test_hdr_request_policy.cpp`,
`test_framegen_policy.cpp` e `test_video_colorspace.cpp`, conforme o tema.
Compilação/fixtures/replay não comprovam execução WGC real ou scanout na C2.
Registre comando, ambiente, resultado e limites. Comparação não implica deploy.

Atualize `docs/CURRENT_STATE.md` apenas nas áreas examinadas: data, HEADs e
estado sujo, diferenças desde a linha de base, evidências e pendências. Não
marque uma correção apenas proposta como implementada. Preserve as conclusões
da outra comparação e evidências de hardware já registradas. Para uma revisão
extensa, use `docs/comparisons/AAAA-MM-DD-vibeshine.md` e deixe um resumo/link no
estado atual; em revisão pequena, atualizar a linha de base basta.
