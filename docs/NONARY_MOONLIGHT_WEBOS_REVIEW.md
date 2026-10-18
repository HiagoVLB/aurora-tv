# Revisão: Nonary/moonlight-qt e Aurora na LG C2

Revisão em 18/10/2026. Referência remota: `Nonary/moonlight-qt`, branch
`master`, commit `3ae5f3b0e8dfb7538422f2920d38126385d9efb4`.
Aurora: base `d241136e3b0de3aaa446a6be899b986181d802db`, incluindo as
alterações locais já presentes. VibeShine local:
`97835dbfb14faafd8b4f13d98bc00cc921d45296`.

Escopo: leitura do código dos três projetos, documentação LG, foto do launcher
e `../aurora.log`. As conclusões de compatibilidade abaixo distinguem suporte
implementado, hipótese de melhoria e comportamento que ainda exige TV real.
Esta revisão não altera o streaming nem instala nada na TV.

Complemento: inspeção aprofundada do VibeShine local na mesma data, incluindo
as políticas de HDR/SDR, captura WGC, sessões remotas e os testes existentes.
As conclusões abaixo já incorporam as correções dessa segunda inspeção.

## Resultado

Vale acompanhar o fork como referência de temporização, diagnóstico e protocolo
VibeShine. O renderer desktop não é diretamente portável para o caminho NDL do
Aurora. O ganho mais imediato para a C2 é corrigir opções inconsistentes,
escolher uma cadência adequada e medir as etapas certas.

VRR do painel não está implementado no caminho webOS atual. A própria LG
documenta que a opção VRR/G-Sync depende de conexão HDMI. O fork controla a
apresentação por APIs nativas de desktop; o Aurora entrega vídeo comprimido
ao NDL, sem uma API equivalente de apresentação adaptativa.

## Botões da foto e sessões VibeShine

Esses itens vêm de `/applist` do host. O fork Qt os trata pela infraestrutura
normal de aplicativos; a semântica dos papéis está sobretudo no VibeShine.
O Aurora local já tem tratamento adicional por UUID reservado.

| Item | Comportamento verificado no código | Situação no Aurora |
| --- | --- | --- |
| Desktop / Steam configurados | Iniciam o aplicativo configurado no host. A mesma aplicação em execução pode ser retomada. | `gs_start_app_ex` usa `/resume` quando o ID corresponde à aplicação atual, inclusive com HDR. |
| Desktop adicional | Para cliente secundário, o host publica intencionalmente a cópia de retomada do jogo ativo junto da entrada configurada, com ID próprio, UUID terminado em `07` e espaços iniciais para ordenar. | A composição da foto corresponde a esse catálogo. A função `project` e o teste `SecondaryCatalogueKeepsConfiguredRunningAppBesideInvisibleResumeDuplicate` confirmam a duplicação intencional. O catálogo da execução ainda seria necessário para provar os IDs dos cartões fotografados. Não deduplicar por título. |
| Remote Monitor | Ativa/reutiliza um monitor virtual por cliente e transmite sua imagem. O host gerencia criação, modo, posição e retenção do monitor. | UUID terminado em `05`; pode ser acionado com outra aplicação ativa. Não equivale a selecionar um monitor físico existente. |
| Remote Input | Encaminha entrada sem capturar o desktop nem transmitir áudio. Produz frames pretos compatíveis com o encoder para manter o transporte de vídeo exigido pelo protocolo. | UUID terminado em `06`; tela preta é comportamento esperado desse papel. Usa o fluxo normal de sessão do cliente. |
| Resume | Retoma o jogo ativo ou o monitor retido conforme o papel do cliente. | UUID terminado em `01`; o host arbitra destino e permissões. |
| Terminate | Encerra transportes do jogo, aplicação e seu ciclo de display, preservando Remote Monitor/Input. O cliente dono do jogo pode encerrar no primeiro pedido. Por padrão, cliente secundário precisa repetir em até 60 segundos; `remote_monitor_terminate_on_first_request=true` permite um pedido. | UUID terminado em `04`. Sucesso conhecido retorna ao launcher; a primeira confirmação ainda aparece como mensagem de erro e exige novo acionamento manual. A confirmação está vinculada ao cliente, geração e aplicação. |
| Disconnect Monitor / Input | Libera explicitamente o respectivo papel. O catálogo do cliente dono de um monitor retido contém Resume e Disconnect Monitor. Disconnect Input existe no protocolo, mas `project` não o publica no catálogo atual; o papel Input é liberado ao terminar seu transporte. | UUIDs terminados em `02`/`03`; os sucessos documentados são reconhecidos sem tentar abrir RTSP. Não instruir o usuário a procurar um cartão Disconnect Input que o host não anuncia. |
| Disconnect no overlay | Encerra o transporte do cliente e deixa a aplicação no host. Remote Input perde seu papel ao finalizar o transporte; Remote Monitor permanece retido por padrão, sujeito às duas políticas de remoção do host. | `session_interrupt(..., false, ...)`. As flags de remoção de monitor são descritas no complemento abaixo. |
| End session no overlay | Pede `/cancel` ao host, que pode fechar a aplicação configurada e restaurar o display. | Tem confirmação. Desabilitado ao entrar diretamente por Remote Monitor/Input, pois `/cancel` não é a liberação desse papel. |
| Default | Seleciona um perfil local do Aurora. | Atualmente cobre apenas parte das preferências; ver achados abaixo. |

VibeShine usa status **XML 410** tanto para controles concluídos quanto para
pedidos de confirmação. O tratamento atual do Aurora distingue UUID e mensagem
de conclusão. Não se deve converter qualquer 410 em sucesso ou repetir
automaticamente uma solicitação de Terminate.

Referências locais:

- `core/libgamestream/src/xml.c`: `xml_app_is_remote_control`, `xml_app_is_remote_display`.
- `core/libgamestream/src/client.c`: `gs_start_app_ex`.
- `src/app/ui/launcher/apps.controller.c`: `item_click_cb`, `open_context_menu`.
- `src/app/ui/streaming/streaming.controller.c`: `exit_streaming`, `suspend_streaming`.
- `../vibeshine/src/remote_session.cpp`: `project`, `dispatch`, `synthetic_running_game`, `successful_control_completion`.
- `../vibeshine/src/nvhttp.cpp`: confirmação de Terminate e ativação dos papéis remotos.

## Achados nas opções atuais

1. **Encerrar o jogo quando o stream termina não tem consumidor de execução.**
   `quitappafter` aparece na definição, valor padrão, leitura/escrita do INI e
   checkbox de Host. Não aparece no caminho de encerramento. O comportamento
   efetivo é decidido pelo argumento de `session_interrupt`. A UI promete algo
   que o código atual não executa. Corrigir ou retirar a opção exige preservar
   a semântica explícita de Disconnect e os papéis remotos.

2. **Perfil de streaming incompleto.** `profile_from_settings` e
   `profile_to_settings`, em `src/app/profile/profile_manager.c`, só copiam
   resolução, FPS inteiro, bitrate, rotação, HDR e HEVC. AV1, 10-bit SDR,
   refresh fracionário/NTSC, áudio, backend e ABR não pertencem ao perfil.
   Em particular, trocar para um perfil HEVC pode deixar AV1 global ligado;
   `session_config_init` prioriza AV1 e ignora a preferência HEVC nesse caso.
   É uma combinação possível por leitura estática, ainda sem reprodução na TV.

3. **Bitrate mostrado e aplicado podem divergir.** A interface permite
   5–400 Mbps, mas `session_config_init` limita ao `video_cap.maxBitrate`.
   O NDL webOS5 anuncia 95 Mbps para SoC classificado como high-end e 65 Mbps
   para os demais, com sugestões de 45/35 Mbps. A UI não explica o clamp.
   Exibir bitrate solicitado, limite e valor aplicado ajudaria. O controle ABR
   local já respeita esse teto e o clamp retornado pelo host.

4. **Nem todas as alterações indicam reconexão.** Backend e áudio chamam
   `module_changed_cb` e marcam `needs_stream_reconnect`. Resolução/FPS chamam
   `on_res_fps_updated`, atualmente vazio; HEVC/AV1/HDR/10-bit atualizam estados
   visuais, sem marcar essa flag. A sessão usa uma configuração copiada na
   criação. O aviso de aplicação na próxima conexão deve ser consistente.

5. **Capacidades não são específicas por codec/modo.** `GetCapabilities` do
   NDL anuncia H.264/HEVC/VP9/AV1, consulta dimensões usando HEVC e não informa
   `maxFps`. Isso não prova AV1 120 ou qualquer combinação de resolução/FPS.
   A política atual precisa de limites por codec e resultados medidos no modelo.

6. **10-bit SDR existe no host; a preferência local não basta para selecioná-lo.**
   Correção da primeira análise: `hdrMode=1` não prova uma implementação
   incorreta. VibeShine usa esse pedido também para identificar um cliente
   que negociou Main10. Com `prefer_10bit_sdr=true` no dispositivo, ou
   `prefer-10bit-sdr=true` na aplicação, suprime HDR efetivo e mantém 10 bits
   no encode. A preferência da aplicação sobrepõe a do dispositivo; o override
   explícito de HDR do host tem prioridade. O Aurora envia Main10/`hdrMode=1`
   com a opção local ligada, mas não envia essa preferência administrativa.
   Assim, a UI deveria explicar a configuração correspondente no VibeShine.
   Com preferência desligada, o mesmo pedido segue a política normal de HDR.
   Há suporte implementado e testes de política no host; a reprodução de
   10-bit SDR no NDL da C2 ainda exige teste físico.

## O que as configurações fazem

| Opção/grupo | Implementação e implicação para a C2 |
| --- | --- |
| Resolução / FPS | Entram no modo de launch e na configuração do protocolo. webOS limita a 120 FPS. FPS solicitado ou submetido não prova FPS exibido. |
| HEVC / AV1 | Checkboxes se excluem ao clicar; H.264 permanece como fallback. AV1 tem preferência quando ambas as flags chegam ligadas por configuração/perfil. Para testar 120, priorizar HEVC e validar a apresentação na C2. |
| HDR | Depende de codec Main10 e capacidade do backend; TV recebe metadados HDR10. HDR10 não implica Dolby Vision. |
| 10-bit SDR | Pede Main10. No VibeShine, configurar também a preferência SDR de 10 bits no dispositivo/aplicação para converter o pedido em Main10 SDR. O host separa profundidade de bits e HDR efetivo. |
| Full range YUV | Override de SDR; HDR usa limited range. O NDL anuncia `fullColorRange=false` e registra risco de cores saturadas. Manter desligado como referência até validar níveis. |
| Video backend | Seleciona um grupo SS4S. NDL/Starfish têm comportamento próprio; mudar o backend não acrescenta VRR. |
| Audio | Negocia canais; o NDL webOS5 anuncia no máximo 6. O mapeamento Opus 5.1 e `surroundParams=642014523` já implementados devem ser preservados. A disponibilidade de 7.1 na interface não demonstra suporte desse backend. |
| SOPS | No VibeShine, o parâmetro é armazenado e exposto como `SUNSHINE_CLIENT_ENABLE_SOPS` aos processos. Não foi encontrado ajuste automático de gráficos do jogo condicionado a essa flag. Não controla VRR nem Game Optimizer. Scripts personalizados podem consumir a variável. |
| Mute PC | Usa o inverso de `localAudioPlayMode` em sessões normais. Remote Monitor força reprodução no host; seu envio de áudio ao cliente é controlado separadamente por `remote_monitor_mute_audio`. Remote Input não transmite áudio. |
| Auto resume | Tenta retomar uma aplicação anunciada em execução ao iniciar/voltar ao Aurora, com proteção contra reentrada. Não necessariamente encontra um papel que o host anuncia como livre. |
| Stats / Compact stats | Controlam a exibição. O FPS de saída atual deriva de frames submetidos ao backend; não mede scanout. |
| NTSC refresh | Sinaliza refresh fracionário: 59,94/119,88 Hz, arredondados a centihertz. Não é VRR e não mede/altera diretamente o refresh físico da C2. |
| View-only | Bloqueia envio de entrada da sessão. Não é o papel Remote Monitor e não cria monitor virtual. |
| Capture system keys | Atua no encaminhamento de modificadores/teclas. No webOS o grab durante streaming é forçado para permitir teclas USB; não garante captura de todos os atalhos reservados pelo sistema. |
| Hardware mouse / Absolute mouse | Escolhem captura por dispositivo e posicionamento absoluto ou relativo. Modo absoluto serve ao desktop; captura relativa é mais apropriada para jogos de câmera. Dependem da disponibilidade do dispositivo no webOS. |
| Deadzone / Swap ABXY / Virtual mouse | Transformações locais de entrada. Já existem; não precisam do renderer Qt. |
| Touchpad mode/speed/gestures/scroll | Traduzem o touchpad do controle para protocolo nativo ou mouse, com ajustes locais. Dependem dos eventos expostos pelo SDL da TV. |
| Battery / Rumble test | Anunciam/enviam capacidades e usam SDL. Rumble, sensores, LED e gatilhos exigem suporte do controle, conexão, permissões e driver; o hardware da C2 sozinho não determina isso. |
| Game mode | Só aparece quando detecta serviço Homebrew Channel. Tenta alterar/restaurar ajustes de imagem/som por Luna/root; não é HDMI ALLM e não habilita VRR interno. Testes históricos documentados são de C5; confirmar aliases na C2. |
| HEVC sliced frames | Anuncia sugestão de slices ao host para processamento em pipeline; depende do encoder. É experimental e não garante redução de latência. |
| Periodic decoder refresh | Solicita IDR periódico para recuperação HEVC. Pode aumentar picos de bitrate; não corrige a cadência do painel. |
| Adaptive bitrate / profiles | Ajusta bitrate por perdas/pressão e perfil local. Low latency aqui é política de bitrate, independente do preset VRR de nome parecido no fork. Não é sincronização de apresentação. |
| Language / Show logs / Reset | Preferências locais. Reset não desfaz configurações do host nem substitui um teste de streaming. |

## VRR: o que o fork realmente oferece

O fork tem um worker de pacing com contrato `IVrrFramePresenter`, backends
DXGI/Composition, Vulkan e Metal, feedback de preparação/apresentação e fallback
quando o renderer não pode cumprir o contrato. A opção exige V-Sync no nível
da UI; cada plataforma escolhe sua forma nativa de apresentação.

Em uma tela de 120 Hz, a política oferece 120 FPS, uma recomendação VRR de
116 FPS e outra de 100 FPS. Esses valores são derivados de headroom para
apresentação adaptativa. Não se deve importar 116/100 como presets de suavidade
para o compositor fixo da TV.

Os presets VRR do fork definem limites de buffer de 0,5/1/4 frames para
Low Latency/Balanced/Smooth, metas de 99%/99,5%/99,95%, histórico de
60/120/300 segundos e tolerância padrão de 0,5 ms. São limites e objetivos,
não latência fixa garantida. Reduce judder e Absorb microstutter alteram
temporização e podem adicionar atraso; não geram quadros intermediários.

O parâmetro `clientVrrRequested=1` também informa ao VibeShine que o cliente
usa a temporização RTP para pacing. No modo automático do host, promove o
refresh solicitado do monitor virtual de 4x o FPS para 1000 Hz, sujeito às
limitações do sistema e aos overrides de modo. Também limita o pool WGC a
um frame e, no encoder AMF nativo, escolhe fila de entrada de um frame quando
não há override explícito. Enviá-lo não habilita VRR na LG e não deve ser feito
sem implementar e validar a capacidade correspondente no cliente.

O NDL integrado ao Aurora expõe `Play(buffer, size, pts)`, tamanho/flush da
fila, limiar de descarte e metadados HDR. Não expõe um contrato equivalente de
adaptive present ou confirmação do instante em que cada frame apareceu.
Os experimentos de PTS/pacing já descartados no `AGENTS.md` não devem ser
repetidos sob outro nome. As conclusões anteriores em C5 não comprovam o
comportamento da C2, mas limitam o que é justificável prometer.

## Features aproveitáveis e prioridade

| Recurso do fork | Aplicabilidade | Prioridade |
| --- | --- | --- |
| Diagnóstico por etapa e percentis | Medir chegada/reassembly/feed/fila sem chamar submit de apresentação física; exportar uma captura limitada e reproduzível. Já há base de diagnóstico no Aurora. | Alta |
| Avisos de configuração/capacidade | Informar clamp de bitrate, codec efetivo, teto por modo e necessidade de reconectar; corrigir perfis e opções sem efeito. | Alta |
| Cadência estável de origem | Alinhar modo do host e stream a 60/120 ou família fracionária realmente medida, sem usar o sinalizador de VRR indevidamente. | Alta |
| Calibração de conexão/decoder | Aproveitar a ideia de testes com perdas, jitter, crescimento de fila e custo de processamento. As APIs de probe do fork são específicas de PyroWave e exigem negociação; não copiar cegamente para HEVC. | Média |
| Diagnóstico de controles | Capacidades reais, bateria, rumble, sensores e testes de permissões; parte já existe no Aurora. | Média |
| Pacing adaptativo / absorver microstutter | Só experimentar após obter observabilidade e controle real do backend; o algoritmo desktop não resolve sozinho a apresentação NDL. | Pesquisa |
| VRR do painel no aplicativo interno | Sem suporte demonstrado no caminho atual; LG documenta VRR/G-Sync para HDMI. Não oferecer checkbox funcional sem outro backend/API e prova no hardware. | Sem implementação viável demonstrada |
| PyroWave | Codec GPU próprio com caminho Vulkan/Metal e alta demanda de rede. NDL não o decodifica; SS4S não tem driver para ele. Exigiria um novo caminho gráfico/decoder e investigação do SoC. | Fora do escopo prático atual |
| YUV 4:4:4 | O suporte no cliente desktop não prova decodificação 4:4:4 no NDL. Não anunciar sem perfis/capacidades específicos e teste. | Sem suporte validado |
| High-performance GPU power | Implementação do fork é específica de amdgpu/Linux. Não se aplica ao SoC LG; o Aurora já tem ajustes próprios de prioridade/root, que precisam de avaliação separada. | Não portar diretamente |

## Complemento: comportamento resolvido no VibeShine

### 10-bit SDR, HDR e precedência

O fluxo completo é `nvhttp.cpp:make_launch_session` →
`hdr_request_policy.h`/`rtsp.h` → `rtsp.cpp` → `video_colorspace.cpp`.
Em política automática, os casos são:

| Pedido do cliente | Preferência efetiva de SDR de 10 bits | Resultado do host |
| --- | --- | --- |
| SDR / decoder de 8 bits | Ligada ou desligada | A preferência não promove um cliente de 8 bits para Main10. |
| Main10 / `hdrMode=1` | Ligada | Main10 SDR, HDR efetivo desligado; HEVC/AV1 Main10 precisam estar disponíveis. |
| Main10 / `hdrMode=1` | Desligada | Pedido normal de HDR; o resultado depende também do display e demais políticas. |

`force_on` liga HDR e limpa a preferência de SDR; `force_off` impede HDR,
mas não equivale à opção que solicita Main10. A conversão RTX HDR e a camada
Vulkan HDR também verificam a preferência SDR para não ativar conversão HDR
nessas sessões. SDR de 10 bits pode usar Rec.709 ou BT.2020 SDR, conforme a
negociação; BT.2020 SDR é distinto de BT.2020/PQ HDR10.

### Catálogo, retomada e remoção

O servidor anuncia clientes secundários como livres e inclui um atalho
sintético de retomada junto do aplicativo configurado. Isso permite anexar
Remote Monitor/Input sem disparar o aviso genérico de substituição de jogo
do Moonlight. Logo, `currentgame=0` não prova que nenhuma aplicação esteja
executando. Isso também explica uma limitação do Auto resume local do Aurora,
que depende de o host anunciar uma aplicação atual.

Substituir uma aplicação normal tem uma confirmação própria, diferente de
Terminate: `remote_monitor_confirm_app_replacement=true` por padrão. O
primeiro pedido retorna XML 410, o segundo confirma em até 60 segundos, com
cliente/geração/aplicação de destino vinculados. Se existem papéis remotos
ativos, a substituição retorna 409 e pede Terminate antes de outro aplicativo.

Por padrão, `remote_monitor_disconnect_on_stream_end=false` e
`remote_monitor_disconnect_on_client_disconnect=false`: o monitor permanece
para Resume. A primeira flag remove ao terminar qualquer stream do papel;
a segunda remove quando há desconexão do cliente. Remote Input tem outro
ciclo: o encerramento do transporte esquece seu proprietário. Não tratar os
dois papéis como recursos igualmente retidos.

Remote Monitor mantém áudio no host independentemente do pedido Mute PC.
`remote_monitor_mute_audio=false` por padrão permite áudio ao monitor remoto;
quando true, suprime captura/transmissão de áudio daquele papel.

### Captura WGC e o nome VRR no host

`frame_limiter_auto_virtual_framegen` distingue disabled (refresh base),
legacy (2x), enabled/automático (4x) e vrr (1000 Hz fixos solicitados para o
monitor virtual). No automático, `clientVrrRequested=1` seleciona a política
de 1000 Hz. O FPS do stream é independente desse refresh. Não há geração de
imagens intermediárias apenas por selecionar essa política.

O WGC varia a admissão de frames conforme atividade: até 2x o FPS no desktop
e 4x em jogo, dentro dessa política. `wgc` é a captura variável; `wgcc` é a
constante, que reapresenta o último frame capturado em timeout. Desktop parado
com FPS baixo pode ser comportamento normal do modo variável. O modo constante
pode manter envio sem movimento novo, mas não corrige por si só judder no jogo.

O backend `/bitrate` retorna o valor aplicado, limita ao teto configurado do
host e a 500 Mbps absolutos, e atua nas sessões do cliente autenticado. A
capacidade de ABR do servidor é anunciada como `supported:false`: a decisão
adaptativa permanece no Aurora, usando esse endpoint para aplicar mudanças.

Fontes locais principais: `../vibeshine/src/remote_session.cpp`,
`../vibeshine/src/nvhttp.cpp`, `../vibeshine/src/rtsp.h`,
`../vibeshine/src/rtsp.cpp`, `../vibeshine/src/video_colorspace.cpp`,
`../vibeshine/src/config.h`, `../vibeshine/src/config.cpp`,
`../vibeshine/src/platform/windows/display_wgc.cpp`,
`../vibeshine/src/platform/windows/wgc_capture_policy.h` e
`../vibeshine/src/platform/windows/display_vram.cpp`.

Nesta segunda inspeção foram compilados e executados os testes existentes
`test_remote_session.cpp`, `test_hdr_request_policy.cpp` e
`test_framegen_policy.cpp`, junto do código de produção correspondente e
GoogleTest já incluído no repositório. Clang/macOS, C++20, ASan/UBSan:
**48 testes passaram**. O replay existente
`tests/unit/platform/windows/test_wgc_source_admission.py`, com Clang e
relógios/WinRT modelados: **794 verificações, zero falhas**. Isso verifica
políticas e admissão de frames; não executa WGC/DXGI real nem prova latência
de captura no PC ou apresentação na C2.

## Indício concreto no log e validação recomendada

O log fornecido registra várias sessões `2560x1440x90`, `93000 kbps`,
NDL webOS5 e ABR Low latency. Não identifica com segurança o modelo/firmware.
Em apresentação fixa a 120 Hz, 90 FPS não divide uniformemente os intervalos
do painel: uma distribuição possível alterna permanências de 8,33 e 16,67 ms.
Isso é uma causa plausível de judder, não uma prova da causa de cada travada
desse log. O refresh efetivo do caminho de vídeo ainda precisa ser medido.

Para isolar comportamento, comparar o mesmo pan em HEVC a 60 e 120 FPS,
com ABR desligado e bitrate fixo inicial conservador, por exemplo 35–45 Mbps,
na mesma resolução. Registrar modo do host, codec efetivo, perdas, fila,
feed stalls e vídeo da tela; depois variar HDR e resolução separadamente.
Esse bitrate é uma condição inicial de comparação, não um limite universal
ou promessa de qualidade. Só depois aumentar bitrate ou ligar ABR.

A LG documenta, para o player webOS TV 22 em UHD, HEVC Main/Main10 4K60
até 60 Mbps e AV1 4K60 até 50 Mbps. Esses são limites publicados do player;
não provam o teto do DirectVideo usado pelo aplicativo homebrew. Também
não sustentam a promessa de 4K120 interno só porque HDMI aceita 4K120.

Executado nesta revisão: `tests/core/run_vibeshine_protocol.sh` em
`aurora-ui-sim:local`, Linux, com fonte montada somente para leitura e sem
rede. Resultado: **VibeShine protocol tests passed**, com ASan/UBSan.
Os fixtures verificam parsing, UUIDs, retomada HDR, controles concluídos,
confirmação sem repetição automática e bitrate aplicado. Não validam
qualidade de movimento, VRR, áudio físico ou o decoder real da C2.

## Fontes externas

- [Fork examinado, commit fixo](https://github.com/Nonary/moonlight-qt/tree/3ae5f3b0e8dfb7538422f2920d38126385d9efb4).
- [Contrato do presenter VRR](https://github.com/Nonary/moonlight-qt/blob/3ae5f3b0e8dfb7538422f2920d38126385d9efb4/app/streaming/video/ffmpeg-renderers/ivrrframepresenter.h).
- [Política de FPS VRR](https://github.com/Nonary/moonlight-qt/blob/3ae5f3b0e8dfb7538422f2920d38126385d9efb4/app/streaming/vrrratepolicy.h).
- [Presets de temporização](https://github.com/Nonary/moonlight-qt/blob/3ae5f3b0e8dfb7538422f2920d38126385d9efb4/app/settings/vrrtimingoptions.h).
- [Sinalização de launch ao VibeShine](https://github.com/Nonary/moonlight-qt/blob/3ae5f3b0e8dfb7538422f2920d38126385d9efb4/app/backend/nvhttp.cpp).
- [PyroWave e negociação](https://github.com/Nonary/moonlight-qt/blob/3ae5f3b0e8dfb7538422f2920d38126385d9efb4/docs/pyrowave-protocol.md).
- [Prioridades e energia da GPU](https://github.com/Nonary/moonlight-qt/blob/3ae5f3b0e8dfb7538422f2920d38126385d9efb4/docs/stream-performance.md).
- [LG: VRR/G-Sync requer HDMI](https://www.lg.com/us/support/help-library/lg-oled-tv-settings-do-you-want-to-know-how-to-enable-vrr-gsync--20154713308922).
- [LG: formatos e limites publicados de webOS TV 22](https://webostv.developer.lge.com/develop/specifications/video-audio-220).
