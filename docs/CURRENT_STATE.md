# Estado atual do Aurora

Linha de base revisada em **18/10/2026**, horário de São Paulo. Este é o ponto de
entrada mantido para saber onde estamos e comparar futuras versões. Os relatórios
detalhados continuam em [integração VibeShine](VIBESHINE_INTEGRATION.md),
[comparação Nonary/webOS](NONARY_MOONLIGHT_WEBOS_REVIEW.md) e
[investigação de logs](DIAGNOSTIC_LOG_REVIEW.md).

## Identidade da versão examinada

| Projeto | Estado local usado |
| --- | --- |
| Aurora | `d241136e3b0de3aaa446a6be899b986181d802db`, **com alterações locais ainda sem commit**. O SHA sozinho não representa a implementação descrita aqui. |
| Nonary/VibeShine | `97835dbfb14faafd8b4f13d98bc00cc921d45296`, checkout `../vibeshine`, limpo na inspeção. Origem: `https://github.com/Nonary/vibeshine`. |
| Nonary/moonlight-qt | `3ae5f3b0e8dfb7538422f2920d38126385d9efb4`, checkout limpo consultado nesta sessão. Origem: `https://github.com/Nonary/moonlight-qt`. A cópia usada estava em `/tmp`; seu caminho não é uma dependência permanente. |

Os SHAs das referências identificam o código lido. Não houve nova verificação de
HEAD remoto nesta criação de skills; não interpretá-los como versões mais recentes
garantidas. O `CMakeLists.txt` declara versão **1.3.1**; isso não prova qual IPK
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

As alterações locais incluem protocolo VibeShine, ABR/teardown, catálogo e overlay,
navegação/configurações, descoberta e testes/simulador de UI. Nem todas foram
produzidas por esta revisão; não considerá-las automaticamente testadas ou prontas
para release. Nenhum código de streaming foi alterado nas duas revisões comparativas.

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
| Configurações | Stream, Input, Host e Experimental são os painéis registrados. Persistência INI e perfis locais existem. | `src/app/ui/settings/settings.controller.c`, `src/app/app_settings.c`, `src/app/profile/` |

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
| Alta | Aviso de reconexão não cobre todas as alterações de vídeo. | Cobrir opções que só entram na configuração da próxima sessão. |
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
