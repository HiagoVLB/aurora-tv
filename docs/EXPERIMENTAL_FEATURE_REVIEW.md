# Revisão das opções experimentais — 24/10/2026

Escopo: todas as opções que estavam no painel Experimental, seus consumidores,
persistência, ciclo de vida e organização. Revisão do working tree, preservando
as alterações anteriores de HTTP, diagnóstico e SS4S. Versão central mantida.

## Organização e comportamento

| Opção | Local atual | Revisão |
| --- | --- | --- |
| Idioma | Outros, nos builds com seletor | Reaplicação e reconstrução dos fragments conservam display e dispositivos de entrada. Catálogos e bundles conferidos; seletor webOS ainda exige TV. |
| Mostrar logs | Outros | Continua imediato. Sinais entre produtor e UI usam atomics; filtro/ring são serializados. Reabrir configurações não descongela a captura. |
| Game Mode | Outros, somente com Homebrew/root detectado | Preferência copiada para a sessão. Valores anteriores e memória de rollback são obtidos antes das escritas; falha/timeout conserva o snapshot. HDR é aplicado pelo worker, sem compartilhar um ponteiro de estado com callbacks. |
| Full range YUV | Stream → Video | Consumidor continua limitado a SDR. UI desabilita com HDR efetivo; teste confere o colorRange resultante. |
| HEVC sliced frames | Stream → Video | Capacidades SDP derivam da configuração efetiva da sessão, incluindo resolução/FPS resolvidos. UI exige HEVC suportado e desabilita ao selecionar AV1. Continua um hint ao host. |
| Atualização periódica HEVC e intervalo | Stream → Video | Valor copiado para a sessão, sem ler preferências mutáveis na thread de vídeo. Intervalo medido em milissegundos monotônicos, independente do FPS efetivamente enviado pelo host. Ambos os sentidos do toggle avisam sobre reconexão. Intervalo e estado persistem; slider é condicionado ao codec/toggle. |
| Bitrate adaptativo e modo | Stream → Bitrate | Uma decisão por amostra nova/com frames; ausência/repetição de estatísticas não provoca aumento. Mantidos clamp do host/backend, parada antes do transporte e backoff. Deadlines toleram wrap do relógio; cálculos proporcionais evitam overflow. |
| Restaurar padrões | Outros → Reset | Libera strings antigas, preserva diretório/persistência e arquivos de pareamento, atualiza a UI. Diálogo é destruído junto ao fragment; reset confirmado/cancelado exercitados. |

As preferências INI mantêm seus nomes, defaults e escopo global anterior; não há
migração destrutiva nem ampliação incidental dos perfis. Idioma e logs não
exigem reconexão; vídeo, ABR e Game Mode entram na próxima sessão. Alterações
comuns de resolução/FPS/bitrate/codec/HDR também sinalizam reconexão.

## Falhas reproduzidas e corrigidas durante a validação

- UBSan detectou `fallback` não inicializado nas opções de FPS. Inicializado o
  array; corrigida também a seleção com índice de fallback `-1` nos dropdowns.
- ASan detectou leitura do container de configurações depois de liberado.
  A limpeza foi movida para `obj_will_delete_cb`, com ownership explícito do
  fragment Stream: os `user_data` de submenus guardam classes, não fragments.
- Voltar podia liberar o objeto focado antes de LVGL enviar CANCEL após KEY.
  O fechamento é adiado até o fim da dispatch, com cancelamento apenas quando
  ainda pendente (o callback em execução não cancela/libera a si próprio).
- Reset confirmado reproduziu queda ao substituir display/input dentro da
  atualização de UI. A reconstrução agora preserva display e input existentes.
- O simulador podia clicar no cabeçalho Reset atrás do diálogo homônimo.
  A busca percorre objetos da frente para trás, e o roteiro exige que a
  confirmação realmente desapareça. Estados salvos e checkboxes foram conferidos
  além do sucesso das ações.
- O fixture reproduziu JSON ABR inválido sob locale com vírgula decimal.
  Feedback agora usa ponto independente do locale e rejeita NaN/Inf. O editor
  de FPS fracionário também interpreta o ponto aceito pelo campo sem depender
  de LC_NUMERIC, preservando 59.94/119.88.
- O backend Luna tinha liberação dupla de reply no fallback de `getConfigs`.
  Corrigido; restauração serializa valores como JSON e protege apóstrofos no
  argumento enviado ao shell do Homebrew.

- Na rodada real de HEVC, o refresh configurado em 2000 ms ainda dependia
  de contar `FPS negociado × intervalo` frames. Com o Desktop enviando cerca
  de 16 FPS numa sessão negociada a 90 FPS, isso alongava o intervalo.
  A política agora mede tempo monotônico desde o último IDR/pedido; reinicia
  por sessão, limita pedidos repetidos e tolera wrap do relógio. A restrição
  ao codec HEVC e o default desligado foram preservados.

## Verificação local

- CTest: `test_settings`, `test_game_mode`, `test_adaptive_bitrate`,
  `test_session_options`, `test_fractional_fps` e `test_idr_refresh` aprovados. Cobrem round-trip/reset/pareamento,
  falhas e efeitos colaterais picture→sound, HDR repetido, timeout após escrita,
  loss/backoff/clamp/overflow, snapshots e capacidades HEVC/AV1/SDR/HDR,
  além de intervalos IDR sob FPS variável/pausas/wrap e valores grandes de INI.
- Fixtures de política executados com AddressSanitizer + UndefinedBehaviorSanitizer.
- Interface SDL/LVGL real: inglês + 16 catálogos, **76 etapas por idioma**,
  1920×1080 e 1280×720, em `.cache/experimental-visual/`. Capturas de Outros e
  controles de Stream abertas para conferir layout e glifos.
- Roteiro `tools/ui-sim/stream-options.json`: **35 etapas em cada resolução**,
  com ASan/UBSan e backend dummy, em `.cache/experimental-final-ui/`.
  Cobre D-pad/mouse, slider, reabertura, gating HDR/AV1, ABR, logs e reset real.
  HEVC/AV1/HDR são capacidades sintéticas explícitas desse fixture de UI.
- Roteiro completo localizado em pt-BR repetido com ASan/UBSan após as correções
  de ciclo de vida, incluindo as duas resoluções, na mesma pasta de UI final.
- Fixture de protocolo `tests/core/run_vibeshine_protocol.sh` com ASan/UBSan
  aprovado, incluindo payload decimal em pt-BR e rejeição de NaN/Inf.
- Auditor gettext + conversão AWK real para `cstrings.json`: **16 catálogos
  aprovados**, sem faltantes/fuzzy/erros de formato ou divergência de bundle;
  `.cache/experimental-i18n-audit.json`. A mudança de nome foi traduzida em todos.
- Cross-build ARM webOS do executável aprovado; logs de verificação em
  `.cache/experimental-checks-complete.log` e `.cache/experimental-webos-complete.log`.

## Limite para encerrar a estabilidade física

A troca de nome para Outros organiza o menu; não certifica firmware ou hardware.
O pacote atual 1.3.3 foi gerado em 24/10/2026, 20:08 São Paulo, com SHA-256
`dc2e20ccf82a56d462c980937983dcfab8c83da4e2783d26539ce5dfca584c51`.
ELF/identidade/marcador de logs, novo menu e 14 bundles foram conferidos;
instalado na OLED42C2PSA (firmware 33.31.68, webOS 10.3.1), com ELF idêntico
ao pacote. Pareamento preservado e logs reiniciados por instalação. Startup
pt-BR e sessões NDL foram observados; os casos executados estão detalhados abaixo.
Scanout, áudio físico, calibração de cores SDR/HDR e navegação real continuam
pendentes de observação e comparação controlada.
Game Mode está indisponível nesta C2 sem o serviço Homebrew; seu rollback físico
não foi exercitado.
O teste de UI não cobre o seletor de idioma específico de webOS. `uk` e `zh-TW`
foram conferidos como catálogos existentes, sem habilitá-los no produto.

O refresh periódico continua desligado por padrão: pedidos de IDR podem causar
hitches por natureza e devem ser usados para diagnóstico de HEVC. Slices dependem
do encoder do host. O histórico C5 não comprova C2; não foram alterados PTS,
pacing, caps do decoder, mapeamento 5.1 ou política de encerramento do host.

A próxima evidência necessária é a observação física na rodada controlada da C2,
com cena conhecida, configuração efetiva registrada e uma opção por comparação.
Faltam idioma/log/reset pelo controle real e restauração do firmware com root.
Sem essa evidência, não há base para declarar todas as opções totalmente estáveis.

O [roteiro da C2](C2_EXPERIMENTAL_VALIDATION.md) registra o artefato atual, critérios
e resultados parciais. O acesso pelo Docker superou a falha de rota nativa do
Mac. Recibos de ambas as instalações e as coletas de streaming estão em
`.cache/experimental-physical/`; o arquivo de build não substitui esses resultados.

## Primeira rodada em execução na C2

O primeiro pacote revisado (`94dc5af2…`) abriu Desktop autenticado em AV1 e
HEVC, 2560×1440 a 90 FPS solicitados, SDR, áudio estéreo NDL. O perfil pedia
400 Mbps e a sessão aplicou o teto de 95 Mbps do backend. O tráfego observado
no Desktop ficou perto de 16 FPS; isso não comprova scanout, movimento ou
latência física. Houve perdas de pacotes e lacunas de áudio, sem erro de feed
NDL nem crash nos arquivos coletados; causa não determinada.

Os três modos ABR aceitaram ajustes reais no host: Balanced 95000→66500→69825,
Quality 95000→66500→47500→49875 e Low latency até 16764 kbps. Nas três saídas,
`Stop begin restore=0` foi seguido de `Stop complete`, sem atividade ABR posterior
no trecho coletado. HEVC com slices passou de 1 para 6 no anúncio SDP e o feed
continuou aceito; o benefício de latência e o uso real de múltiplos slices pelo
encoder não foram comprovados. A rodada de refresh revelou a falha de contagem
de frames corrigida acima. Configuração e perfis temporários foram restaurados.

Registros da rodada: `.cache/experimental-physical/first-package-runtime-review.json`
e os arquivos de cada caso ali listados. Ainda faltam observação física,
cores SDR/HDR, controles reais de idioma/log/reset e rollback em firmware com root.

## Correção do intervalo validada no cliente real

Segundo IPK 1.3.3 gerado às 20:08 São Paulo, SHA-256
`dc2e20ccf82a56d462c980937983dcfab8c83da4e2783d26539ce5dfca584c51`,
instalado e ELF conferido (`d1c5e52a…`). A sequência com 2000 ms registrou
pedidos a cada ~2,04–2,12 s em vários trechos. Receber um IDR reinicia a referência,
então a separação entre pedidos também pode ser maior. O teste com refresh
desligado e o teste em AV1 (mesmo com 2000 ms persistidos) não emitiram pedidos
periódicos. AV1 também anunciou somente 1 slice/no RFI com sliced frames
persistido, e o overlay habilitado não causou crash na coleta da sessão.

HEVC SDR com full range persistido e HEVC HDR abriram com feed aceito, sem crash.
O host sinalizou HDR e NDL recebeu PQ/BT.2020 (transfer=16, primaries/matrix=9).
Ainda faltam calibração de cores/níveis e confirmação física de imagem/áudio/controle.
A TV usa `wlan0`; perdas de frames/áudio foram registradas, sem causa isolada.
Preferências da rodada restauradas; perfis e lista de hosts conferem byte a byte,
e hashes de pareamento permaneceram iguais. Dados em
`.cache/experimental-physical/timer-package-runtime-review.json` e recibo da
segunda instalação. Não houve commit, push ou incremento da versão.
