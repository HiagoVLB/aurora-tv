# AGENTS.md

Orientações para trabalhar no Aurora. Priorize comportamento correto no webOS,
latência previsível e uma interface utilizável com o controle da TV. Implemente
o escopo pedido preservando o trabalho local e os contratos de sessão existentes.

## Projeto e ponto de partida

Aurora (`com.aurora.gamestream`) é um fork de moonlight-tv, em C11, para LG webOS.
O build Linux desktop serve ao desenvolvimento. A interface usa LVGL v8 e a API
SDL2; no webOS essa API roda sobre SDL3 via sdl2-compat, com shims próprios para
NDL e teclas. O build usa CMake e submódulos. Os alvos de CI são webOS e desktop;
a presença de scripts antigos de outras plataformas não significa suporte ativo.

Antes de alterar uma área, consulte o código e `git status --short`. Não reverta
mudanças que já estavam no checkout nem atualize submódulos incidentalmente.
HEAD sozinho não identifica uma versão com alterações locais.

- [Estado atual](docs/CURRENT_STATE.md): linha de base datada, referências,
  implementação, lacunas e validações. Consulte em planejamento, investigação
  e comparação; confirme no código as afirmações que afetarem a tarefa.
- [Histórico de streaming](docs/WEBOS_STREAMING_HISTORY.md): experimentos e áudio
  já avaliados na C5. Leia antes de mexer em pacing, PTS, AV1 120 ou mapeamento 5.1.
- [Integração VibeShine](docs/VIBESHINE_INTEGRATION.md) e
  [revisão Nonary/webOS](docs/NONARY_MOONLIGHT_WEBOS_REVIEW.md): contratos e análises.
  A revisão inclui correções posteriores; não trate diagnóstico histórico como
  defeito ainda presente sem conferir o working tree.

O alvo de interesse atual é a LG C2. Evidência de C5 + eARC deve continuar
identificada como tal. Versão declarada no CMake, IPK gerado e versão instalada
na TV são estados diferentes; não presuma que coincidem.

## Mapa do código

| Responsabilidade | Onde trabalhar |
| --- | --- |
| Inicialização e loop principal | `src/app/app.c`, `src/main.c` |
| Protocolo GameStream, XML, pareamento e launch | `core/libgamestream/`; transporte e callbacks Limelight em `core/moonlight-common-c/` |
| Descoberta, hosts e catálogo | `src/app/backend/pcmanager/`, `src/app/backend/apploader/` |
| Ciclo de sessão, configuração e ABR | `src/app/stream/session.c`, `session_worker.c`, `adaptive_bitrate.c` |
| Vídeo, áudio e entrada de streaming | `src/app/stream/video/`, `audio/`, `input/`; saída pelo SS4S em `third_party/ss4s/` |
| Launcher, configurações e overlay | `src/app/ui/launcher/`, `settings/`, `streaming/` |
| Preferências e perfis | `src/app/app_settings.{h,c}`, `src/app/profile/` |
| Integração webOS, SDL/NDL, Game Mode e prioridade | `src/app/platform/webos/`, `third_party/commons/cmake/ExternalSDL3ForWebOS.cmake` |
| Testes, simulador e empacotamento | `tests/`, `tools/ui-sim/`, `scripts/webos/`, `.github/workflows/` |

O SS4S escolhe módulos em runtime. Confirme no log o backend selecionado; não
atribua ao NDL o comportamento do Starfish/SMP ou do decoder dummy do simulador.

## Build e validação

Execute os comandos a partir da raiz do Aurora. Em clone novo, inicialize com
`git submodule update --init --recursive`; há dependências aninhadas no
moonlight-common-c. Em checkout existente, examine o estado dos submódulos antes.

### Linux desktop

As dependências Ubuntu estão em
[build-desktop.yml](.github/workflows/build-desktop.yml); a imagem do simulador
em Ubuntu 24.04 contém o conjunto ampliado de desenvolvimento. `gettext`
(`msgfmt`, `xgettext`, `msgmerge`) é obrigatório no configure desktop.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DTARGET_DESKTOP=ON
cmake --build build
xvfb-run ctest --test-dir build --output-on-failure
```

`BUILD_TESTS` fica ON por padrão no desktop e OFF no webOS. Escolha o teste
pertinente, como `ctest --test-dir build -R test_settings --output-on-failure`,
quando a mudança tiver escopo específico. Os testes principais usam Unity;
os fixtures de protocolo são um executável isolado.

### macOS/Linux com Docker: interface e protocolo

```sh
./tools/ui-sim/run.sh
```

O [simulador](tools/ui-sim/README.md) compila a interface real SDL/LVGL com saída
SS4S dummy. O runtime fica sem rede e usa preferências isoladas. Os resultados
estão em `.cache/ui-sim-run-*/artifacts/` (`report.json`, `index.html`, PNGs e logs).
Use cenários para foco, navegação, popups, mouse/roda, controle direcional e resize.
Isso não emula o firmware LG nem valida decoder, áudio ou streaming real.

Para contratos VibeShine, em Linux com as dependências de desenvolvimento:

```sh
bash tests/core/run_vibeshine_protocol.sh
```

No Mac, depois de criar a imagem do simulador, execute o mesmo fixture em Linux:

```sh
docker run --rm --network none --volume "$PWD:/source:ro" --workdir /source aurora-ui-sim:local bash tests/core/run_vibeshine_protocol.sh
```

### webOS

Consulte [BUILD_WEBOS.md](docs/BUILD_WEBOS.md), os scripts em `scripts/webos/` e a
[action de CI](.github/actions/build-webos/action.yml). O SDK usado é buildroot-nc4
`webos-a38c582`, host x86_64, destino ARM. Em Mac ARM, ele exige ambiente Linux
x86_64 compatível; o build do simulador ARM64 não substitui esse cross-build.
Com SDK/ares-package preparados em Linux ou WSL:

```sh
CI=1 TOOLCHAIN_FILE=/caminho/do/sdk/share/buildroot/toolchainfile.cmake ./scripts/webos/easy_build.sh -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

`CI=1` evita a atualização automática dos submódulos nesse script; inicialize-os
antes. `TARGET_WEBOS` é detectado pelo toolchain/compiler e altera fontes,
bibliotecas e recursos. O build webOS converte `.po` em `cstrings.json` no pacote.

Os wrappers WSL contêm caminhos legados `/mnt/c/Projetos/...`: confira-os antes
de reutilizar; `run_wsl_build.py` não é um entrypoint portátil. Builds podem
produzir IPKs em diretórios temporários. Localize/copie o artefato para um caminho
persistente antes de encerrar o ambiente e informe o arquivo efetivamente gerado.
Build/empacotamento e instalação são etapas distintas; instalar não é consequência
automática de uma tarefa de documentação, revisão ou build.

## UI, preferências e tradução

- A UI é baseada em fragments LVGL v8. Preserve ownership, callbacks e grupos de
  foco. Cancele callbacks assíncronos que guardem um fragment ao destruí-lo.
  Valide Back/Escape e retorno de foco ao fechar dropdowns, popups e overlay.
- A lista `entries[]` de `settings.controller.c` determina os painéis acessíveis:
  Stream, Input, Host e Experimental. Compilar um pane não o torna visível.
  Preferências comuns pertencem ao painel correspondente; experimentos ficam em
  `experimental.pane.c`. Não recrie panes de vídeo/áudio inacessíveis.
- Uma preferência nova precisa de campo, default, leitura/escrita INI, UI acessível
  e consumidor real. Decida se pertence ao perfil ou é global; confira os dois
  sentidos de conversão em `profile_manager.c`. Checkbox salvo não prova efeito.
  `app_configuration` referencia `&app->settings`; a sessão recebe uma cópia efetiva.
- Distinga alteração ao vivo de configuração aplicada na próxima sessão. Atualize
  `needs_stream_reconnect` quando necessário e explique o valor efetivo após clamps.
  Não acrescente opções sem capacidade/efeito demonstrável no backend selecionado.
- Ao retirar uma chave, mantenha compatibilidade no bloco compartilhado de chaves
  legadas ignoradas em `app_settings.c`. Não apague preferências ou pareamentos
  como forma de contornar uma migração.
- Textos de UI usam `locstr`/`translatable` e catálogos `src/i18n/`. Atualize as
  traduções afetadas, especialmente pt-BR, preservando placeholders e o fluxo
  gettext desktop / cstrings webOS.

## Invariantes de sessão e latência

- Preserve a cadência atual do loop: `app_process_events`, `lv_task_handler` e
  `SDL_Delay(1)`, com `LV_INDEV_DEF_READ_PERIOD=1`. Aumentar o intervalo afeta entrada.
- A dispatch de eventos passa por `SDL_FilterEvents`, sob mutex da fila SDL.
  Não faça rede, disco, joins ou esperas longas nesse callback. Encaminhe trabalho
  entre threads pelo bus existente e respeite a vida útil de seus dados.
- Vídeo usa `CAPABILITY_DIRECT_SUBMIT`: feed bloqueante também bloqueia a recepção
  RTP. Áudio webOS não usa direct submit para evitar que transcode Opus 5.1
  monopolize a thread de recepção. Não altere isso como otimização sem medição.
- Timestamps de decode units e Limelight usam microssegundos com epoch comum na
  implementação atual de `PltGetMicroseconds`. Revalide unidades/epoch ao atualizar
  moonlight-common-c; NDL recebe PTS em milissegundos.
- Pare/junte ABR antes de `LiStopConnection`. Não restaure bitrate de uma sessão
  encerrada nem envie ajustes após `/cancel`. Preserve teto do backend, clamp do
  host e configuração efetiva ao renegociar.
- Disconnect deixa a aplicação no host; End session solicita `/cancel` com a
  confirmação existente. Remote Monitor e Remote Input não são jogos comuns:
  use UUID/papel e regras do host, não nomes de cartões. HTTP 200 e XML 410
  precisam de interpretação de protocolo; não repita Terminate automaticamente.
- Preserve `surroundParams=642014523` e o passthrough Opus 5.1 correspondente.
  Não acrescente `surround-params` do Sunshine nem force PCM/RemapPcm51 como correção
  genérica. O histórico C5 + eARC e as convenções de canais estão no documento
  de streaming; valide outro modelo/caminho de áudio separadamente.
- Game Mode e ajustes de prioridade/root têm ciclo de entrada/restauração de
  sessão. Preserve o cleanup nos caminhos de erro. Não trate isso como HDMI ALLM.

## Diagnóstico e compatibilidade webOS

Registre modelo, firmware, backend, codec efetivo, resolução, FPS, HDR,
bitrate solicitado/aplicado, conexão e configuração do host em experimentos.
Um relato de C5 não valida C2; uma capacidade HDMI não prova suporte do decoder
interno. VRR do host/display virtual, pacing, V-Sync e VRR físico são distintos.

O FPS atual do overlay mede submit, não scanout. Tempo de `Play` não é tempo de
decode, e fila estimada não é latência física. Compare movimento com uma carga
reproduzível e isole variáveis. Leia o
[histórico de pacing/AV1](docs/WEBOS_STREAMING_HISTORY.md) antes de testar outra
variação de PTS; confira env e condições de ativação no código atual.

Logs persistentes usam a infraestrutura em `src/app/util/file_log.*`, com filas
limitadas e escrita fora do caminho de feed. Não registre chaves de entrada,
credenciais ou URLs completas de launch com segredos. Preferências/log ficam em
`$HOME/conf` quando gravável, com fallback temporário em `/tmp/moonlight-tv/conf`;
confirme o caminho emitido no startup. Logs não substituem stack trace nem provam,
sozinhos, a causa de um host ter ficado offline.

## Dependências e comparação com outros projetos

LVGL é o fork v8 de mariotaku; atualizar para v9 não é manutenção rotineira.
SS4S/commons e moonlight-common-c têm contratos com a aplicação; alterações devem
identificar commits e patches locais. Não aumente caps de vídeo SS4S com base
apenas nas especificações do painel. O parser de dimensões em commons suporta
H.264/HEVC; ausência de dimensões AV1 nas estatísticas não prova falha do stream.

Use a skill correspondente quando a tarefa pedir comparação ou investigação
substancial de compatibilidade:

- [aurora-compare-vibeshine](.agents/skills/aurora-compare-vibeshine/SKILL.md)
  (`$aurora-compare-vibeshine`): contrato cliente/host, catálogo, sessões remotas,
  HDR/SDR, entrada/áudio e bitrate.
- [aurora-compare-nonary](.agents/skills/aurora-compare-nonary/SKILL.md)
  (`$aurora-compare-nonary`): diferenças do cliente Qt e adaptações viáveis para
  webOS/C2, incluindo apresentação, codecs e telemetria.

Leia o SKILL.md selecionado. Referências usuais são `../vibeshine` e um checkout
identificado de Nonary/moonlight-qt; não dependa de um clone antigo em `/tmp`.
A captura local de commits, diffs e submódulos pode ser feita com
`scripts/dev/comparison_snapshot.py`. Não é necessário comparar os três projetos
para uma edição rotineira. Não transforme conclusão de análise em feature validada.

## Encerramento do trabalho

Execute os checks adequados ao escopo: documentação/links para docs, simulador
para UI, fixtures para protocolo, build para integração de fontes e teste físico
para afirmações sobre streaming real. Registre o que passou e o que não foi
executado; compilação, testes de política e simulação não comprovam scanout,
áudio físico ou limpeza do host numa TV real.

Ao mudar comportamento, resolver uma pendência ou atualizar uma referência,
atualize a área correspondente de [CURRENT_STATE.md](docs/CURRENT_STATE.md).
Mantenha aqui os invariantes e a orientação de trabalho; deixe versões, resultados
mutáveis e pendências na linha de base, e experimentos detalhados nos relatórios.
A resposta final deve dizer o que mudou, por quê, como foi verificado e qualquer
limitação material. Não declare deploy, versão instalada ou compatibilidade de
hardware que não foram verificados.
