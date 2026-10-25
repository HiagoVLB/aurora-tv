# Estado atual do Aurora

Linha de base revisada em **25/10/2026**, horário de São Paulo. Este é o ponto de
entrada mantido para saber onde estamos e comparar futuras versões. Os relatórios
detalhados continuam em [integração VibeShine](VIBESHINE_INTEGRATION.md),
[comparação Nonary/webOS](NONARY_MOONLIGHT_WEBOS_REVIEW.md) e
[investigação de logs](DIAGNOSTIC_LOG_REVIEW.md).

## Identidade da versão examinada

| Projeto | Estado local usado |
| --- | --- |
| Aurora | `d208961e43e76a7cabebad3ab8cfd1f1a443c88a`, **com alterações locais de automação de IPK/versionamento ainda sem commit**. O SHA sozinho não representa a implementação descrita aqui. |
| Nonary/VibeShine | `97835dbfb14faafd8b4f13d98bc00cc921d45296`, checkout `../vibeshine`, limpo na inspeção. Origem: `https://github.com/Nonary/vibeshine`. |
| Nonary/moonlight-qt | `3ae5f3b0e8dfb7538422f2920d38126385d9efb4`, checkout limpo consultado nesta sessão. Origem: `https://github.com/Nonary/moonlight-qt`. A cópia usada estava em `/tmp`; seu caminho não é uma dependência permanente. |

Os SHAs das referências identificam o código lido. Não houve nova verificação de
HEAD remoto nesta criação de skills; não interpretá-los como versões mais recentes
garantidas. O `CMakeLists.txt` declara versão **1.3.3**; isso não prova qual IPK
está instalado na TV. O alvo de interesse do usuário é **LG C2**; evidências
históricas de **C5 + eARC** no [histórico de streaming](WEBOS_STREAMING_HISTORY.md)
não são validação da C2.

Submódulos diretos relevantes no checkout:

| Dependência | Commit |
| --- | --- |
| moonlight-common-c | `9ae7cbb05f6f0fbed516043c9a0a3231b217d8ab` |
| SS4S | `49918fc58f0f78fd342ed6e891172a2b401a9a2c` |
| commons | `e778249c460bbf52f943e41c7e505823d9932508` |
| LVGL | `185ea1fc61dd01fac61867d2d6b56892e80c6058` |

A base inclui protocolo VibeShine, ABR/teardown, catálogo e overlay,
navegação/configurações, descoberta e testes/simulador de UI. As novas alterações
locais automatizam IPK/versionamento e fazem a marca do ELF derivar de APP_VERSION.
Nem todas as áreas foram validadas em hardware; não considerar tudo pronto para
release. As revisões comparativas anteriores não alteraram o streaming.

## Usabilidade das opções e controle da TV (25/10/2026)

- Stream mantém as opções condicionais visíveis e explica os requisitos junto
  aos controles: exclusão HEVC/AV1, HDR/10-bit SDR, YUV SDR, slices e refresh
  exclusivos de HEVC, intervalo de refresh e perfil de bitrate adaptativo.
  AV1/HDR também indicam indisponibilidade pelo backend. Os valores salvos e
  a política efetiva da sessão permanecem os mesmos.
- O menu da sessão destaca a ação em foco com aumento de 6,25%, fundo mais
  claro e contorno branco, inclusive por ponteiro. Ao reabrir, começa em
  Teclado completo. As quatro cores de atalho e a confirmação de End session
  continuam correspondendo às ações existentes.
- Home do controle envia Windows esquerdo (pressionar/soltar) durante uma
  sessão interativa, abrindo o menu Iniciar do host. Idle/view-only mantêm o
  Home da TV; Home físico/virtual conserva sua função de navegação.
  Números 0–9 recebidos pelo caminho SDL webOS viram Numpad 0–9. O teclado USB
  capturado via evdev conserva o mapeamento físico; o fallback SDL não distingue
  números do controle e de teclados sem captura. O resultado depende também do
  estado de Num Lock no host. Ajuda atualizada para ambos os atalhos.
- Dez textos novos traduzidos nos 16 catálogos existentes, sem habilitar idiomas.
  Auditoria gettext/cstrings aprovada, sem ausentes, vazios ou fuzzy. Build Linux
  e CTests `test_settings`, `test_session_options` e `test_remote_home` aprovados.
  UI aprovada em 34 execuções/986 etapas: inglês + 16 catálogos, em 1080p/720p,
  com inspeção das capturas de textos e menu. `uk`/`zh-TW` continuam inativos no
  produto; foram carregados apenas no simulador. Também aprovados foco por
  ponteiro e notas de backend sem AV1/HDR em inglês/pt-BR. Evidências em
  `.cache/usability/`, matriz `ui/matrix-all.json`, capturas `ui/contacts/`.
  Nenhum IPK foi gerado/instalado nesta alteração. Home, Numpad e a apresentação
  física na C2 ainda precisam ser conferidos na TV.
- README ampliado com tutorial de pareamento, primeira sessão, navegação,
  aplicação de ajustes/perfis e tabelas das opções acessíveis em Stream, Input,
  Host e Other. Explica dependências, uso prático e comparação de problemas,
  sem desenvolver teoria de HDR/codecs. Conferidos os nomes e consumidores no
  código, links locais, âncoras e estrutura Markdown.
- Pendência documentada no guia: `quitappafter` é exibido e persistido, mas não
  tem consumidor no encerramento da sessão. A opção **Quit the game when the
  stream ends** não implementa fechamento automático; a ação explícita
  **End session** continua sendo o caminho para solicitar `/cancel` ao host.

## Revisão das opções experimentais e menus (24/10/2026)

- Todas as opções do antigo Experimental foram rastreadas até persistência e
  consumidor. Vídeo/refresh HEVC e ABR ficam em **Stream**; idioma, logs,
  Game Mode e reset ficam em **Other / Outros**, traduzido nos 16 catálogos.
- Corrigidos ownership/limpeza de fragments, memória não inicializada de FPS,
  rebuild de UI no reset/idioma, liberação de preferências no reset, snapshots
  de sessão, ABR com amostras repetidas e rollback/HDR do Game Mode. Detalhes e
  limites em [EXPERIMENTAL_FEATURE_REVIEW.md](EXPERIMENTAL_FEATURE_REVIEW.md).
- CTest e fixtures com ASan/UBSan aprovados; UI localizada em inglês + 16 idiomas
  em 1080p/720p, roteiro de opções/reset real com sanitizers nas duas resoluções,
  gettext/cstrings dos 16 catálogos e compilação ARM aprovados. O IPK de teste foi
  gerado e auditado; instalado na C2 e startup conferido às 19:57, com hashes de
  pareamento preservados e vídeo/áudio NDL. Casos de streaming ainda pendentes.
  Critérios por opção em [C2_EXPERIMENTAL_VALIDATION.md](C2_EXPERIMENTAL_VALIDATION.md).
- Rodada real abriu Desktop em AV1 e HEVC, SDR 2560×1440/90 solicitados,
  estéreo NDL, teto aplicado de 95 Mbps. Três modos ABR ajustaram bitrate aceito
  pelo host e completaram a parada; slices HEVC alterou o anúncio de 1 para 6.
  Perdas de pacotes e lacunas de áudio observadas; sem crash/erro de feed nos
  trechos coletados. Isso ainda não valida apresentação física.
- Encontrado e corrigido refresh HEVC baseado em contagem de frames: agora usa
  tempo monotônico, inclusive com FPS variável/pausas, sem overflow por
  multiplicação de FPS. Seis CTests pertinentes e novo fixture ASan/UBSan
  aprovados; novo IPK gerado e instalado às 20:08. Na C2, pedidos periódicos
  observaram o intervalo em tempo; desligado e AV1 não geraram esses pedidos.
  SDR full range e HEVC HDR abriram sem erro de feed/crash; HDR sinalizado pelo
  host e metadados PQ/BT.2020 no NDL. Cores/áudio físicos ainda pendentes.
  Preferências temporárias restauradas, perfis/hosts/pareamento preservados.
- Estabilidade física permanece pendente: decoder, cores/HDR e permissões e
  restauração do firmware não são demonstrados pelo backend dummy. O refresh
  periódico continua opcional/desligado por padrão e pode causar hitches de IDR.

## Geração de IPK e versionamento

- Em **24/10/2026, 20:08 São Paulo**, IPK **1.3.3** regenerado com a correção
  do temporizador HEVC, **2.385.978 bytes**, SHA-256
  `dc2e20ccf82a56d462c980937983dcfab8c83da4e2783d26539ce5dfca584c51`.
  Recibo `.cache/ipk/run-hfluctai/`. Instalado na C2; ELF conferido:
  `d1c5e52a694d7089824a4f1f4629444f87bf24b053ef8455df06ddf96a2bb548`.
  Novo reset de logs por instalação observado; cópia anterior preservada.
  Menu Outros e 14 bundles iguais aos já auditados. Refresh, SDR/HDR e gating
  AV1 exercitados no cliente; observação física de imagem/áudio/controle e
  UI real continuam pendentes. Evidências em
  `.cache/experimental-physical/timer-fix-installation-receipt.json` e
  `timer-package-runtime-review.json`. Configuração da rodada restaurada.

- Em **24/10/2026, 19:42 São Paulo**, IPK **1.3.3** regenerado com as correções
  da revisão Experimental/Outros, mantendo a versão central. Entrega `aurora.ipk`,
  **2.386.028 bytes**, SHA-256
  `94dc5af224cf1e2d0744f47e6396ac7f394e888b627734b907f5bf9fcf016506`.
  Recibo `.cache/ipk/run-2k2ho7b_/` (`latest.json` aponta para essa execução).
  ELF ARM, identidade, versão e marcador de logs aprovados; auditoria adicional
  confirmou o menu Other/Outros, ausência do antigo menu Experimental, código
  de rollback novo e os 14 bundles empacotados. Após a confirmação de disponibilidade
  pelo usuário, instalado via SSH/Docker e reaberto às 19:57 na **OLED42C2PSA**,
  firmware **33.31.68**, webOS **10.3.1**. ELF confere com o pacote; configurações
  e pareamento preservados, logs da instalação anterior salvos antes do reset.
  Startup pt-BR com NDL vídeo/áudio e sem crash na coleta inicial. Serviço
  Homebrew inexistente; Game Mode root indisponível. Streaming/scanout/HDR ainda
  pendentes; recibo `.cache/experimental-physical/installation-receipt.json`.

- Em **24/10/2026, 11:35 São Paulo**, IPK **1.3.3** recompilado com proteção
  da vida útil do contexto nos callbacks NDL e correções HTTP, sem incremento.
  ARM/RelWithDebInfo, metadados/ELF e reset por instalação aprovados. SHA-256
  `7f2b74c21926432ba35b7ef04d81bb3b2f6ec7caff47fdfa7f69fa2f194111a1`;
  recibo, log e símbolos em `.cache/ipk/run-16ak3gfd/`. Símbolos do novo pacote
  confirmam `CallbackContextLock` e `http_init`; módulo NDL com SHA-256
  `b264106440c7f8cb223841df721518075c330a5b341a6ebdec867769f45b3817`.
  Build desktop e quatro testes CTest pertinentes passaram, além dos fixtures
  HTTP/NDL/decode com ASan/UBSan. O caso novo de callback falhava antes do patch.
  Pacote não instalado e aplicativo não executado, conforme restrição do usuário.

- Em **22/10/2026, 21:36 São Paulo**, IPK **1.3.3** recompilado com correção
  local da concorrência NDL e validação de fragmentos de vídeo, sem incrementar
  a versão. ARM/RelWithDebInfo, metadados/ELF e reset por instalação aprovados.
  SHA-256 `7d40301f90207ecab6971cf32c49277e017f9d33509210c628c9a5e326b0de0d`;
  recibo e símbolos em `.cache/ipk/run-s4iicwal/`. Build desktop completo e os
  dois testes CTest de segurança de vídeo passaram; fixture ASan/UBSan também.
  Instalação e validação física deste pacote ainda pendentes.

- Em **22/10/2026**, versão central alterada de **1.3.2** para **1.3.3**,
  interpretando o pedido `1.333` como o próximo patch no formato `X.Y.Z`.
  Helper conferiu a derivação de appinfo/APP_VERSION/marca do ELF. Em seguida,
  IPK **1.3.3**, ARM/RelWithDebInfo via Docker, gerado e validado com diagnóstico
  v2 e limpeza de logs por instalação. SHA-256:
  `74129148808e5a3ed2ca9cede0eb408ff4ec2568647fcca56cbca1cc7b382e32`.
  Fontes do commit `454e6b25888201816fa52ffb4720c67bd92cbdfb` com alterações
  locais de versão/documentação. Metadados, ELF ARM e marcador de reset
  aprovados; recibo/log/símbolos em `.cache/ipk/run-wyjwcgps/`.
  Pacote não instalado nem validado em execução na C2 nesta tarefa.
- Em **22/10/2026**, versão central incrementada por solicitação de **1.3.1**
  para **1.3.2**. Contrato de derivação appinfo/APP_VERSION/marca do ELF conferido
  pelo helper. Depois, IPK **1.3.2** recompilado e validado em ARM/RelWithDebInfo,
  com captura fatal e marcador de limpeza por instalação. Fontes do commit
  `7a9e9f76b52d362d9ad1ec260863b02f353045cd`, limpas durante o build. SHA-256 desse build:
  `b4eb9eb6b36c59af0bde4f9faa5ea7b6d42dede6067622df3181a7858b61e3ae`.
  Recibo/log/símbolos em `.cache/ipk/run-vyajzlsw/`; nenhuma instalação ou
  validação física na C2 foi realizada. Os builds anteriores seguem abaixo.
- Novo IPK **1.3.2** com diagnóstico **v2**, gerado em **22/10/2026**, destino
  ARM/RelWithDebInfo, com fontes locais alteradas. SHA-256 desse build:
  `fd15de7413a23f921b79ea01848dbd5b17225f5de08ea43ac38fe092fc0f2c64`.
  Metadados/ELF, strings v2, marcador de reset e arquivo de símbolos conferidos;
  tabelas ARM de unwind presentes e nenhuma dependência obrigatória de libunwind.
  Recibo/log/símbolos em `.cache/ipk/run-2wcjcrr9/`. Nova captura não instalada
  nem validada fisicamente na C2 nesta tarefa.
- Skills: `aurora-build-ipk` e `aurora-bump-version`, referenciadas no AGENTS.md.
- Comando de build: `python3 scripts/webos/build_ipk.py`. Saída única de entrega:
  **`aurora.ipk` na raiz do Aurora**, ignorada pelo Git. O pacote anterior só é
  substituído após validação do ID, versão, controle, arquitetura e marca ARM ELF.
- Logs/recibo: `.cache/ipk/run-*/build.log`, `source.json` e
  `.cache/ipk/latest.json`; não registrar o binário gerado como fonte versionada.
- Fonte única da versão: `MOONLIGHT_VERSION` em CMakeLists.txt. Manifesto,
  APP_VERSION e marca `aurora-v` do ELF derivam dela.
- Incremento padrão: `python3 scripts/dev/aurora_version.py --part patch --apply`.
  Sem `--apply`, apenas simula. Na criação destas skills a versão era **1.3.1**;
  a versão central atual é **1.3.3**. Build e incremento são operações separadas.
- Gerar um pacote não instala na TV nem comprova funcionamento na C2.
- Validação em 19/10/2026: build real Linux/amd64 via Docker, destino ARM,
  RelWithDebInfo, concluído pelo helper. `aurora.ipk` entregue com versão
  **1.3.1**, metadados/ELF/marca de versão conferidos. SHA-256:
  `b34761ff1ec175094bfa8976b8f42517b317728e3be933c3dd70f2c2462df9b4`.
  O recibo local de cada build fica em `.cache/ipk/latest.json`; o pacote não
  foi instalado na TV nesta tarefa.
- Novo build em **22/10/2026**, RelWithDebInfo ARM via Docker, com a captura fatal
  textual descrita abaixo: `aurora.ipk` **1.3.1**, metadados/ELF/marca de versão e
  strings da captura conferidos. SHA-256:
  `33c0f87dbfb376ff59134e9f92fe580a331b5b2cc34381aa13690664dc62e1d4`.
  Recibo e símbolos completos de `bin/aurora` preservados em
  `.cache/ipk/run-nsvt2f47/` (arquivo `_dbgsym.tar.gz` em `packages/`).
  Build identifica fontes locais alteradas; não houve incremento nem instalação.
- IPK regenerado em **22/10/2026** com limpeza de logs por instalação, mesma
  versão **1.3.1**, ARM/RelWithDebInfo. Marcador e código de reset conferidos no
  pacote; recibo inclui `installation_log_reset_verified=true`. SHA-256 desse build:
  `23a867609d39371718cad6532b445a51f14fe6805a7f6c530af71100f9efa1a1`.
  Recibo/log/símbolos em `.cache/ipk/run-eu54hyum/`; não instalado na TV.

## SIGABRT confirmado na C2 e captura v2 (22/10/2026)

- Revisão de **24/10/2026** dos três arquivos fornecidos: fatal de **11:18:29
  São Paulo**, PID **16316**, `moonlight-io`, `munmap_chunk(): invalid pointer`.
  `aurora-run.state` corresponde ao fatal. O log completo (8967 linhas) contém
  cinco startups que identificam execuções anteriores como `crashed`; o último
  fatal vem após cleanup da sessão. O frame próprio `0x4fe40` resolve para
  `http_destroy` nos símbolos do IPK local de 22/10; o restante passa por
  libc/c-ares/libcurl. Isso identifica onde a corrupção foi detectada, sem
  provar a escrita original nem a identidade binária da instalação atual.
  Um novo teste isolado reproduziu escrita inválida no callback NDL
  `LOADCOMPLETED` durante destruição do player. Patch local SS4S protege a
  publicação/retirada do contexto com um mutex separado dos calls de firmware,
  evitando acesso após destruição e deadlock em callback síncrono.
  A rede agora inicializa curl antes de iniciar threads, protege tamanhos/OOM
  do buffer HTTP, usa os tipos corretos da API e limpa referências temporárias
  após requisições. Evidência, validação e limites em
  [DIAGNOSTIC_LOG_REVIEW.md](DIAGNOSTIC_LOG_REVIEW.md). Nenhum aplicativo ou
  sessão de streaming foi executado nesta investigação. Testes isolados HTTP
  e NDL/decode com ASan/UBSan passaram; build desktop do aplicativo e quatro
  testes CTest pertinentes passaram, incluindo os 20000 ciclos com callbacks.

- Nova captura v2, às **21:18:36 de 22/10**, confirma **corrupção do heap**:
  `corrupted size vs. prev_size`, PID 5052, thread VideoRecv, AV1 1440p90.
  Bibliotecas exatas lidas da C2 resolvem a pilha em `g_malloc` chamado pelo
  feed do pipeline LG, via NDL_DirectVideoPlay. Isso localiza a detecção, não a
  escrita original. Fontes locais agora serializam feed de vídeo/áudio e unload
  do mesmo player NDL, evitam deadlock em callbacks/estatísticas e protegem a
  cópia dos fragmentos contra tamanhos inválidos. A sobreposição foi reproduzida
  no driver anterior; testes de concorrência, callback síncrono, prime tardio,
  erros e limites de buffers passam com ASan/UBSan. Correção física do crash e
  efeito sobre latência na C2 ainda pendentes. Evidência detalhada em
  [DIAGNOSTIC_LOG_REVIEW.md](DIAGNOSTIC_LOG_REVIEW.md). Patches locais em SS4S
  sobre `49918fc58f0f78fd342ed6e891172a2b401a9a2c`; sem atualização do submódulo.
  Uma consulta seguinte encontrou outro fatal do módulo antigo ainda instalado,
  PID 7197 às **21:30:25**, `double free or corruption (out)` em `video-src:src`.
  A libc exata aponta a detecção ao liberar o tcache na saída da thread; isso
  não identifica a escrita original nem testa o pacote corrigido. Relatório
  e hashes antigo/novo registrados no mesmo documento de diagnóstico.

- Log da instalação **1.3.3**, às **20:17:46**, confirma limpeza de logs e
  `CrashLog v2 ... persistent=1 backtrace=1` na C2. Inicialização da v2 validada;
  a captura de pilha fatal real seguinte está descrita acima. A ocorrência seguinte tem
  intervalos/overflow de áudio, perda de novos submits e desconexão de controle
  às **20:19:43**, com cleanup e app ainda ativo; não registra novo SIGABRT.
  O usuário esclareceu que o Windows respondia e só a tela do jogo ficou preta.
  Há defeito independente confirmado no ABR: amostra envelhecida com 0% de perda
  permite probes de aumento mesmo após o vídeo parar. Correção pendente;
  não demonstra a causa da tela preta. Falta log do VibeShine instalado.
  Evidências em [DIAGNOSTIC_LOG_REVIEW.md](DIAGNOSTIC_LOG_REVIEW.md) e comparação
  local em [VIBESHINE_INTEGRATION.md](VIBESHINE_INTEGRATION.md).
- Nova captura, ainda **v1**, confirma **SIGABRT** às **20:02:58**, PID **9531**,
  thread **9617**, 95 segundos após abrir. `aurora-run.state` corresponde e está
  `crashed`. PC/LR relativos à libc repetem **0x21ca6 / 0x5e7c4** do primeiro
  relatório; a origem do abort continua desconhecida. Último vídeo com zero
  erros de feed às 20:02:57.197. O log estendido também registra falha NDL e
  início de shutdown por SDL_QUIT às 20:01 na execução anterior (PID **7483**),
  seguida de startup indicando `crashed`; o relatório fatal dessa execução foi
  sobrescrito pelo último crash. São ocorrências separadas. Evidências locais
  em `.cache/crash-diagnostics/sigabrt-2026-10-22-200258/`; detalhes no relatório.
- Relatório recebido identifica Aurora **1.3.2**, PID **3474**, thread **5373**,
  **SIGABRT (6), si_code=-6**, às **19:29:27 de 22/10** em São Paulo. PC/LR
  pertencem à libc da TV; a v1 não registrou a origem da chamada. Marcador
  `crashed` foi confirmado no startup seguinte. O estado `running` fornecido
  pertence à nova execução, PID **7483**. Isso valida captura fatal v1 persistida
  e identificação de execução anterior nessa C2; não identifica a causa.
- Captura **v2** acrescenta nome da thread/remetente do sinal, pilha local de
  até 32 endereços via libunwind opcional e mensagem fatal da glibc quando
  disponível (512 caracteres). Probing/símbolos resolvidos no startup; handler
  usa buffers fixos e I/O de sinal. Converte o contexto GP ARM corretamente,
  retém tabelas de unwind próprias e solicita só metadados DWARF/EXIDX.
  Pilha pode ficar parcial por ausência de tabelas na libc/firmware.
- Relatório básico é sincronizado antes do unwind. Falha do próprio unwinder
  preserva sinal original e evidencia `backtrace_fault_signal`; biblioteca
  ausente mantém captura básica. Mensagem da libc é lida com segurança via
  descritor de `/proc/self/mem`, sem dump de memória bruta. A causa original
  do SIGABRT permanece pendente. Detalhes no relatório de diagnóstico.
- Testes CMake Linux aprovados: file/crash/reset, unwind disponível/ausente e
  falha deliberada no unwinder. Fixture ARM32 passou sob QEMU + glibc Ubuntu e
  libunwind 1.6.2, incluindo abort com pilha parcial e motivo de asserção.
  Isso não emula o firmware LG. Testes também aprovados em macOS e Linux com
  ASan/UBSan; o simulador compilou. Build/IPK v2 concluído e validado conforme
  recibo acima; validação física da v2 ainda pendente. A versão central continua
  **1.3.3** após o incremento posterior; a limpeza por instalação está mantida.

## Limpeza de logs a cada instalação (22/10/2026)

- Preferência permanente do usuário registrada no AGENTS.md: cada instalação
  deve começar com os três arquivos de diagnóstico limpos, incluindo reinstalar
  exatamente o mesmo IPK/versão. Reabrir a mesma instalação preserva o histórico.
- Todo IPK contém `aurora-logs-reset.pending` na raiz do app. A primeira abertura
  remove somente `aurora.log`, `aurora-crash.log` e `aurora-run.state` de
  `$HOME/conf` e do fallback `/tmp/moonlight-tv/conf`, antes dos writers, e
  consome o marcador. Não usa versão/data do build como único gatilho.
  Registra sucesso/erro; em falha mantém o marcador para repetir a limpeza.
- Configurações, perfis, pareamentos e outros arquivos ficam preservados.
  A limpeza usa nomes exatos, sem seguir symlinks de logs/diretórios ou truncar
  inodes compartilhados. O verificador do IPK exige o marcador válido; os
  fixtures de publicação cobrem rejeição sem substituir o instalador anterior.
- Validação: fixture de reset passou em macOS/Linux com warnings tratados como
  erros; nove testes Python de release/publicação passaram. Integração CTest
  aprovou `test_diagnostic_reset`, `test_crash_log` e `test_file_log`; alvo
  `aurora-ui-sim` compilou. Build ARM e IPK validados, incluindo marcador e código
  de reset no executável do pacote. Limpeza real na C2 ainda não executada;
  a instalação do novo pacote não é automática.

## Diagnóstico de fechamento na C2 (22/10/2026)

- O log fornecido identifica Aurora **1.3.1**, webOS **10.3.1**, NDL,
  AV1 8-bit 2560×1440@90, HDR desligado e ABR com teto 95000 kbps. A interrupção
  abrupta às **21:27 de 21/10**, seguida de reinicialização e `/resume`, é distinta
  da falha explícita de feed AV1 às **22:46**, com teardown e `quit_app=0`.
  A causa do primeiro fechamento continua indeterminada. Evidências detalhadas
  em [DIAGNOSTIC_LOG_REVIEW.md](DIAGNOSTIC_LOG_REVIEW.md).
- O processo instalado chama-se `aurora`; seus limites soft/hard de core foram
  conferidos na TV como **0/0**. As pastas temporárias não continham relatórios
  acessíveis na inspeção após uma reinicialização da TV. Isso não exclui crash.
- Nova captura textual em `aurora-crash.log` (**64 KiB**, último fatal preservado
  ao reabrir) e marcador `aurora-run.state` (**511 bytes**) ao lado de `aurora.log`.
  Registra sinais fatais, horários, versão/PID/boot ID, PC/SP/LR ARM e mapas
  executáveis. O próximo startup alerta se o anterior não concluiu o shutdown.
  A captura usa descritores abertos previamente, buffers fixos e I/O seguro em
  signal handler; não usa logger, malloc, unwind ou mutex no handler.
- Arquivos exportáveis por Dev Manager, links/arquivos não regulares recusados,
  conteúdo limitado. Captura independente de root e da lista Crash Reports;
  usa o mesmo diretório/fallback do log e informa persistência no startup.
  Core completo continua desativado fora de Debug: a entrega é texto de falha,
  não dump de memória nem stack trace completo. Não recupera o incidente anterior.
- Registros de ciclo SDL, origem do pedido de saída e início/fim do shutdown
  ajudam a distinguir saída normal, falha capturada e execução incompleta.
  SIGKILL/energia/armazenamento e corrupção severa podem impedir o relatório;
  o marcador sozinho não identifica OOM nem causa da queda. Só a thread principal
  recebe stack alternativo; overflow de stack de outras threads não é garantido.
- Testes isolados passaram em macOS e Linux, incluindo sinais fatais reais,
  falhas de memória na principal e em worker, término pelo sinal original,
  SIGKILL sem relatório inventado, marcadores entre processos, preservação do
  último fatal, limites, permissões e recusa de links. Linux também passou com
  AddressSanitizer + UndefinedBehaviorSanitizer (interceptação de sinais fatais
  dos sanitizers desabilitada para exercitar o handler real).
- Integração CMake Linux aprovada: `test_file_log` e `test_crash_log` passaram por
  CTest e o alvo `aurora-ui-sim` compilou. Build webOS ARM RelWithDebInfo e
  validação do IPK concluídos; executável empacotado contém os novos marcadores e
  arquivo de símbolos correspondente foi preservado (recibo na seção de build).
  Versão central preservada em **1.3.1**; nenhuma instalação ou validação física
  desta captura foi realizada na TV. A causa da queda original não foi corrigida
  nem reproduzida: a entrega prepara o diagnóstico da próxima ocorrência.

## Skill de atualização dos idiomas (18/10/2026)

- Criada [aurora-update-languages](../.agents/skills/aurora-update-languages/SKILL.md),
  com adaptação contextual por língua, revisão de terminologia/placeholders,
  sincronização gettext e conferência dos recursos JSON webOS e da UI localizada.
- Auditor read-only incluído na skill: cobertura contra POT, fuzzy, duplicatas,
  `msgfmt --check --check-format` e comparação com a saída do AWK real. O relatório
  inicial está em `.cache/i18n/skill-creation-audit.json` (artefato local ignorado).
- Inventário observado: 16 catálogos, 13 em `I18N_LOCALES`; `ko` aparece no seletor
  mas não na lista de empacotamento, e `uk`/`zh-TW` têm catálogo sem seletor.
  Todos os catálogos têm pendências contra o POT existente. A criação da skill
  não sincronizou/traduziu esses arquivos nem habilitou idiomas.
- O simulador fixa a preferência em inglês, enquanto o desktop inicia o locale
  pelo ambiente e configurações reaplicam a preferência. É necessário alinhar
  ambos e preparar locale/MO/fontes. A skill exige prova de idioma carregado
  e inspeção dos PNGs em duas resoluções; nenhum idioma foi validado visualmente
  ou na TV durante a criação da skill.
- Validação da ferramenta: estrutura/metadados da skill aprovados; dez fixtures
  passaram usando gettext e o conversor AWK real, incluindo erro de placeholder,
  fuzzy, duplicatas, contextos/plurais sem suporte, acentos e linhas longas.

## Atualização dos idiomas e revisão visual (19/10/2026)

- Os 16 catálogos existentes têm as **211 mensagens atuais traduzidas**, sem
  entradas vazias ou fuzzy. O POT foi extraído novamente do código; comentários,
  créditos e traduções antigas fora de uso foram preservados. A revisão inclui
  ajuda/atalhos, configurações, erros, estatísticas e a distinção entre
  desconectar e encerrar a sessão no PC. O indicador de resolução nativa também
  passou a usar localização.
- Coreano foi incluído no empacotamento, pois já era selecionável. Os nomes
  próprios do coreano e neerlandês foram corrigidos no seletor. Ucraniano e chinês
  tradicional continuam sem opção no seletor/pacote; japonês mantém a condição
  DEBUG. Atualizar os catálogos não alterou essas condições.
- O simulador aceita `AURORA_UI_SIM_LANGUAGE`, compila MO dos 16 catálogos e
  prepara locales/fontes CJK. O roteiro `tools/ui-sim/localization.json` usa os
  textos traduzidos reais e percorre duas resoluções. Textos de checkbox e valores
  de dropdown podem ser verificados; `scroll_text` inspeciona sem clicar.
- Foram corrigidos o overflow da barra superior em 720p, que escondia botões em
  idiomas com textos longos, e um estado de dropdown que consumia o próximo
  Voltar. O seletor de servidor usa cópia de texto, pois o modo de reticências do
  LVGL modifica o buffer e não pode escrever na memória read-only do gettext.
- Validação: gettext e JSON webOS aprovados nos 16 idiomas; **16 × 80 etapas**
  da UI aprovadas em 1080p/720p, mais **16 × 8 etapas** para a resolução nativa
  localizada. Capturas foram abertas para conferir layout e glifos. Oito testes
  dos helpers de release passaram. Detalhes, cobertura inicial e limites em
  [I18N_REVIEW.md](I18N_REVIEW.md).
- Não houve instalação na C2, geração de IPK ou incremento de versão. Fontes e
  carregamento de bundles na TV, seletor de idioma específico de webOS e overlay
  durante streaming real continuam dependentes de validação física.

## Implementado no cliente atual

| Área | Comportamento presente | Evidência |
| --- | --- | --- |
| Plataforma | Cliente C11, CMake, LVGL v8. webOS usa SDL3 via sdl2-compat; desktop continua SDL2. Vídeo/áudio passam por SS4S. | `CMakeLists.txt`, `third_party/commons/cmake/ExternalSDL3ForWebOS.cmake`, `src/app/stream/` |
| Catálogo e launch | Preserva UUID/ArtVersion, envia UUID no launch e mantém fallback numérico; reconhece controles sintéticos do VibeShine por UUID. | `core/libgamestream/src/{xml,client}.c`, `src/app/ui/launcher/apps.controller.c` |
| Retomada | A aplicação atual retoma por `/resume`, inclusive com HDR. | `gs_start_app_ex` em `core/libgamestream/src/client.c` |
| Saída | Disconnect encerra transporte; End session confirma `/cancel`. O overlay bloqueia End session ao entrar diretamente por Remote Monitor/Input. | `src/app/ui/streaming/streaming.controller.c` |
| Controles 410 | Conclusões conhecidas, com UUID e mensagem correspondentes, retornam ao launcher sem RTSP; confirmação permanece mensagem, sem repetição automática. | `gs_start_app_ex`, `src/app/stream/session_worker.c` |
| Bitrate | Sessão respeita teto do backend; ABR respeita esse teto e bitrate aplicado pelo host; para antes do encerramento da conexão. | `session.c`, `adaptive_bitrate.c`, `session_worker.c`, `gs_set_bitrate_ex` |
| Estatísticas | Novas sessões limpam amostras anteriores; FPS de saída mede submit ao backend, não apresentação física. | `src/app/stream/video/session_video.c` |
| Configurações | Stream, Input, Host e Other (Outros) são os painéis registrados. Persistência INI e perfis locais existem. | `src/app/ui/settings/settings.controller.c`, `src/app/app_settings.c`, `src/app/profile/` |

## README and built-in controls guide (2026-10-18)

- The README now reflects the current settings, bitrate limits, local `aurora.ipk`
  build helper, diagnostics, and session controls. Its English copy distinguishes
  C5 experiments from C2 validation and removes blanket compatibility claims.
- The home screen's **?** dialog opens **Controls & shortcuts**, with English
  source copy for TV remote, gamepad, virtual mouse, on-screen keyboard, and
  Disconnect / End session. It replaces the upstream wiki launch buttons and
  keeps version, decoder, audio backend, and system details at the bottom.
- The guide wraps long text and supports D-pad scrolling, mouse wheel, and
  Back/B/Escape dismissal. In particular, remote Hold Back opens the stream menu;
  gamepad Select/Back held alone for four seconds toggles pinned stats.
- New help catalog entries initially retained English fallback; all existing
  catalogs were completed in the 19/10 language review above. The app's language
  default and existing user language preferences are unchanged. A streaming hint now
  advertises the stats shortcut instead of pointing to removed wiki content.
- Validation: the Docker SDL/LVGL simulator built successfully; 33 steps passed,
  covering opening the guide, D-pad and wheel scrolling, reaching About, resizing from
  1920 × 1080 to 1280 × 720, Escape and close-button dismissal, reopening, and
  opening Settings afterward. Local README links and gettext catalogs also passed
  validation. Screenshots were inspected at both resolutions. This validates UI
  behavior with dummy backends, not remote key delivery or streaming on a TV.

## Contratos do VibeShine já esclarecidos

- **Remote Input:** transporta entrada com vídeo preto sintético, sem capturar
  desktop ou transmitir áudio. Seu papel é liberado ao terminar o transporte.
- **Remote Monitor:** captura o monitor virtual exato; por padrão o mantém para
  Resume após desconexão. Flags do host podem remover o display ao terminar o
  stream ou desconectar. Mantém reprodução de áudio no host; silenciar o áudio
  enviado ao monitor é uma configuração separada.
- **Desktop duplicado:** catálogo secundário contém intencionalmente atalho de
  retomada e aplicação configurada. `currentgame=0` não prova host sem jogo ativo.
- **Terminate:** preserva papéis remotos. Por padrão, secundário confirma com
  segundo acionamento em até 60 s; o dono do jogo pode encerrar no primeiro pedido.
- **10-bit SDR:** suportado no host por preferência de dispositivo/aplicação.
  `hdrMode=1` também negocia Main10. A preferência SDR suprime HDR efetivo;
  a preferência da aplicação e o override explícito de HDR têm precedência.
  A opção local do Aurora sozinha não configura essa política administrativa.
- **ABR:** host anuncia decisão adaptativa não suportada; disponibiliza `/bitrate`
  para aplicar decisões do cliente. Retorna o valor aplicado e pode limitá-lo.
- **VRR do host:** política de captura/display virtual, incluindo solicitação de
  1000 Hz e filas menores. É independente de VRR físico na LG.

Esses contratos têm rastreamento e referências de código no
[complemento VibeShine da revisão](NONARY_MOONLIGHT_WEBOS_REVIEW.md).

## Lacunas e decisões pendentes

| Prioridade | Situação atual | Próximo passo verificável |
| --- | --- | --- |
| Alta | `quitappafter` está persistido/visível, sem consumidor encontrado no encerramento. | Definir/corrigir a semântica sem tornar Disconnect um `/cancel` involuntário. |
| Alta | Perfis só guardam resolução, FPS inteiro, bitrate, rotação, HDR e HEVC. AV1 e outras opções permanecem globais. | Tornar explícito o escopo ou ampliar perfis, verificando alternância e migração. |
| Alta | UI permite até 400 Mbps; sessão aplica teto menor sem explicar valor efetivo. | Exibir solicitado/aplicado/limite com base no backend selecionado. |
| Média | Stream agora sinaliza reconexão para vídeo/bitrate/ABR; NTSC ainda está no painel Host sem esse callback. | Revisar o restante das preferências de Host separadamente. |
| Alta | Codec/FPS anunciado pelo backend não prova capacidade de cada modo na C2. | Medir HEVC/AV1 por resolução/FPS e apresentação física, preservando caps SS4S. |
| Média | Nome 10-bit SDR local não explica dependência da política do host. | Melhorar a informação e validar Main10 SDR real na C2. |
| Média | Confirmação de Terminate/substituição aparece como erro genérico. | Tratar confirmação na UI mantendo uma nova ação explícita do usuário. |
| Pesquisa | VRR físico não está implementado/demonstrado no NDL atual; PyroWave/4:4:4 não têm caminho validado nessa TV. | Exigir API/backend e prova de hardware antes de oferecer essas capacidades. |

Features do Nonary com melhor potencial atual: diagnóstico por etapa/frametime,
avisos de configuração e calibração. Não há portabilidade direta do renderer
DXGI/Vulkan/Metal para a apresentação NDL. Não importar presets 116/100 FPS como
se garantissem suavidade num painel de apresentação fixa a 120 Hz.

## Evidência de validação disponível

| Evidência executada nas revisões desta conversa | Resultado | Limite |
| --- | --- | --- |
| Fixtures `tests/core/run_vibeshine_protocol.sh`, Docker `aurora-ui-sim:local`, ASan/UBSan | Passaram | Parsing/protocolo com rede/crypto simulados; não TV/host real. |
| VibeShine `test_remote_session.cpp`, `test_hdr_request_policy.cpp`, `test_framegen_policy.cpp`, Clang/macOS, ASan/UBSan | 48 testes passaram | Políticas; não execução real dos drivers Windows. |
| Replay `test_wgc_source_admission.py`, Clang/macOS | 794 verificações, zero falhas | WinRT/relógios modelados; não captura física. |
| `python3 -m unittest discover -s tests/scripts -p test_release_helpers.py -v` | 8 testes passaram | Incrementos em cópias temporárias e fixtures IPK; preservação do instalador anterior e rejeição de metadados/arquitetura divergentes. |
| `python3 scripts/webos/build_ipk.py --backend docker --jobs 6`, RelWithDebInfo | Build ARM e entrega `aurora.ipk` 1.3.1 concluídos | Valida pacote/ELF; não instala nem valida execução na C2. |

Não houve novo teste físico na C2 nem deploy nas revisões. O log disponível
inclui sessões 1440p90 a 93 Mbps no NDL webOS5, mas não identifica sozinho
modelo/firmware nem demonstra a causa de cada travada. 90 FPS em apresentação
fixa a 120 Hz é uma hipótese de judder a isolar comparando 60/120 no mesmo pan.
Os experimentos anteriores na C5 permanecem no
[histórico de streaming](WEBOS_STREAMING_HISTORY.md), referenciado pelo AGENTS.md.

## Como manter este estado

Use as skills do [AGENTS.md](../AGENTS.md) para a comparação correspondente.
Cada revisão deve atualizar somente o escopo examinado, manter a data e os
commits/estado sujo, ligar o relatório quando necessário e distinguir observação,
inferência e plano. Registre correções de conclusões anteriores explicitamente.
Uma tarefa rotineira não exige revalidar os três repositórios completos.

O helper `scripts/dev/comparison_snapshot.py` produz evidência local em JSON:

```sh
python3 scripts/dev/comparison_snapshot.py --reference vibeshine=../vibeshine --reference nonary=../moonlight-qt --output .cache/comparisons/current.json
```

Os caminhos de referência são exemplos: use checkouts reais. O JSON registra
HEAD, status e hashes de diffs/arquivos novos/submódulos sem copiar o conteúdo
dos fontes. O identificador muda se o estado local mudar, mesmo sem novo commit.
O snapshot é um manifesto, não backup da árvore, e não verifica atualização remota.
Use `.cache` para evitar incluir o próprio arquivo de saída no próximo manifesto.

## Registro de correções desta linha de base

- 18/10/2026: revisão inicial interpretou `hdrMode=1` como indício de conflito de
  10-bit SDR. Inspeção do host confirmou suporte explícito e dependência de sua
  preferência administrativa; a comparação detalhada já foi corrigida.
- 18/10/2026: confirmado que Remote Input é liberado com o transporte e que seu
  controle Disconnect existe no protocolo, mas não é publicado pelo catálogo
  atual. A retenção padrão é de Remote Monitor.
- 18/10/2026: diferenças ABR descritas na investigação histórica de logs já têm
  correções no working tree atual; não repetir o diagnóstico antigo como estado
  presente. O build webOS também já usa SDL3 com compatibilidade SDL2.
- 18/10/2026: AGENTS.md reorganizado como guia geral de arquitetura, implementação,
  builds, testes e comparação. Os relatos anteriores de pacing/AV1/áudio C5 foram
  preservados em WEBOS_STREAMING_HISTORY.md; não houve mudança de streaming.
- 18/10/2026: criadas skills de IPK e incremento de versão, com helpers executáveis.
  A marca fixa antiga `aurora-v1.3.0` foi substituída por derivação de APP_VERSION,
  mantendo a versão do projeto em 1.3.1. O instalador passa a ter nome de entrega
  fixo na raiz; falhas de validação preservam a entrega anterior.
- 2026-10-18: updated COPYRIGHT in English to identify
  `https://github.com/GuiDev1994/aurora-tv/`, distinguish Aurora contributions
  from inherited Moonlight TV code, and retain GPL-3.0-or-later and dependency
  attribution. Checked against the referenced repository and local license files;
  no license terms or application behavior were changed.
- 2026-10-18: compared `.gitignore` with mariotaku/moonlight-tv `main` and added
  its missing `.virtualenv` and `.mcp.json` exclusions. Aurora also ignores local
  webOS packages/SDK downloads, root build logs, Visual Studio workspace data,
  Python bytecode/environments, and Ninja/CPack output. Removed 13 already tracked
  local artifacts (Visual Studio data, two build logs, and the previously ignored
  `.superpowers` report) from the index; their on-disk contents were preserved.
  Source files, patches, shared VS Code settings, and project skills remain
  trackable. Validated positive/negative ignore cases and local-file hashes.
