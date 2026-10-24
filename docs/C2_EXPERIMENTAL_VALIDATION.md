# Validação física das opções revisadas na C2

Estado em 24/10/2026: **rodada iniciada**. IPK atualizado na C2 e aplicação
reaberta inicialmente às **19:57 São Paulo**, com segunda instalação às 20:08
para corrigir o intervalo do refresh HEVC. AV1/HEVC, três modos ABR, slices,
refresh e ativação HDR foram exercitados no cliente real. Observação de imagem,
áudio, cores e navegação pelo usuário continua pendente; esta rodada ainda não
certifica a estabilidade de todas as opções.

## Artefato e ponto de partida

- Arquivo: `../aurora.ipk`, Aurora **1.3.3**, ARM, RelWithDebInfo.
- SHA-256: `dc2e20ccf82a56d462c980937983dcfab8c83da4e2783d26539ce5dfca584c51`.
  Segundo pacote da rodada, gerado às 20:08 São Paulo, 2.385.978 bytes.
- Recibo: `.cache/ipk/run-hfluctai/package-feature-audit.json` e
  `.cache/ipk/latest.json`. Menu Outros e os 14 bundles empacotados conferidos,
  incluindo `Other → Outros` em pt-BR; versão/ELF/marcador de logs aprovados.
- Acesso SSH obtido pelo Docker no endpoint cadastrado. A falha `errno 65` do
  processo nativo do Mac às 19:43 não significava indisponibilidade da TV.
- Antes da atualização, exportados os logs, configurações e hashes dos arquivos
  de pareamento. Backup local restrito em
  `.cache/experimental-physical/before-update.tar`; pacote anterior preservado.
- Serviço de instalação retornou `state=installed`, `update=true`.
  ELF instalado confere com o contido no IPK:
  `d1c5e52a694d7089824a4f1f4629444f87bf24b053ef8455df06ddf96a2bb548`.
- Launch retornou sucesso; primeiro PID 9739 no pacote inicial, 11808 no pacote
  com temporizador corrigido e 12641 após restaurar as preferências. Startup
  identifica Aurora 1.3.3.
  Marcador de reset consumido e log novo confirma limpeza por instalação.
  Crash log vazio nas coletas. Hashes dos arquivos de pareamento idênticos.
  As configurações temporárias da rodada foram restauradas; perfis e lista de
  hosts conferem byte a byte com os arquivos preservados. Uma mudança de AV1
  para `true` foi observada entre o startup inicial e a primeira sessão; sua
  origem não foi determinada e o valor em uso antes da matriz foi preservado.
- Recibo específico da instalação e evidências:
  `.cache/experimental-physical/timer-fix-installation-receipt.json`,
  `timer-package-runtime-review.json` e `install-timer-fix.txt`. A primeira
  instalação e os testes ABR/slices estão preservados separadamente em
  `installation-receipt.json` e `first-package-runtime-review.json`.
  O recibo de build permanece distinto da validação em execução.

## Dados da rodada

Dados observados nas duas instalações e sessões desta rodada:

| Campo | Valor observado |
| --- | --- |
| Modelo / firmware da C2 | OLED42C2PSA / 33.31.68; webOS SDK 10.3.1 |
| Aurora instalado / hash do pacote | 1.3.3, IPK e ELF conferidos conforme hashes acima |
| Backend SS4S selecionado no log | Vídeo e áudio `ndl-webos5`; load/playing e primeiro feed confirmados |
| Host e versão / encoder | DESKTOP-HO7UDGJ, `serverinfo` anuncia protocolo `7.1.431.-1` e compatibilidade GFE `3.23.0.74`; versão real do host/encoder pendentes |
| Rede e caminho de áudio | TV pela rota `wlan0`, Ethernet inativa; sessão estéreo NDL; saída física de áudio pendente |
| Codec efetivo / resolução / FPS / HDR | AV1 8-bit SDR e HEVC SDR/HDR; 2560×1440 a 90 FPS solicitados. Desktop frequentemente enviou cerca de 16 FPS; scanout não medido |
| Bitrate solicitado / aplicado / teto do backend | Perfil 400000 kbps; sessão limitada a 95000 kbps pelo NDL; ABR variou abaixo desse teto |
| Homebrew/root disponível | Serviço Homebrew inexistente; SSH root :22 recusado; Game Mode indisponível nesta instalação |

A consulta HTTP pública de `serverinfo` não usa a identidade do cliente; seu
`PairStatus=0` não é evidência de perda de pareamento. A cópia dos arquivos e o
status do host no Aurora são evidências separadas. Catálogo autenticado e sessões
Desktop foram abertos com sucesso durante esta rodada.

Manter a mesma cena e o restante da configuração, alterando uma opção por vez.
Começar com uma configuração de streaming já funcional nessa TV e confirmar o
codec efetivo. Repetir saída normal e reconexão; em caso de falha, guardar logs do
cliente e do host, horário e o passo que a provocou. Não usar FPS de submit do
overlay como prova de apresentação física.

## Casos e critérios

| Caso | Ação e evidência necessária | Critério para encerrar | Resultado |
| --- | --- | --- | --- |
| Idioma | Em Outros, alternar pt-BR/inglês, navegar, fechar/reabrir e reiniciar o app. Conferir fontes e textos completos com o controle real. | Idioma persistido, labels coerentes, sem perda de foco, glifos ausentes ou queda na reconstrução da UI. | Pendente |
| Logs | Ligar/desligar Mostrar logs; exercitar Live/Frozen/Off com Yellow durante streaming e reabrir configurações. | Estado e captura respondem ao controle; congelamento permanece ao reabrir; sem travamento ou regressão perceptível durante feed. | Streaming com show_logs=true abriu e manteve feed sem crash na coleta; ciclo Yellow/Live/Frozen/Off e leitura na tela pendentes |
| Game Mode | Somente quando root/Homebrew estiver disponível: anotar picture/sound antes, ativar, reconectar, alternar SDR/HDR e sair. Guardar logs de aplicação e restauração. | Ajustes com snapshot válido são aplicados e restaurados; nenhuma alteração sem valor anterior; HDR não causa queda. Sem root, registrar indisponível, sem contar como teste aprovado. | Indisponível nesta C2: serviço Homebrew inexistente; rollback físico não exercitado |
| Full range SDR | Comparar desligado/ligado, reconectando, com a mesma imagem de níveis de preto/branco; registrar negociação do host e resultado visual. Repetir com HDR ligado. | Preferência tem efeito no SDR compatível sem clipping indevido; controle fica indisponível em HDR e a sessão HDR mantém range limitado. | HEVC SDR com preferência ligada e sessão HDR abriram sem erro de feed/crash; negociação de range coberta pelo fixture. Calibração de níveis/cores na tela pendente |
| Slices HEVC | Ligar, reconectar em HEVC e guardar o log de capacidades SDP. Comparar a mesma cena; depois selecionar AV1 e voltar a HEVC. | Hint corresponde ao modo resolvido, sem falhas de feed/decodificação; controles HEVC ficam indisponíveis em AV1. Host ignorar o hint não prova benefício de latência. | SDP mudou 1→6 slices/frame em HEVC; feed aceito. AV1 anunciou 1 slice/no RFI com preferência persistida. Slicing real do encoder e efeito físico não medidos |
| Refresh HEVC | Ativar com intervalo conhecido, reconectar e observar pedidos de IDR e apresentação. Alterar/desligar e reconectar; repetir em AV1. | Intervalo e desligamento obedecem à nova sessão; não há pedidos periódicos em AV1. Registrar qualquer hitch: IDR periódico pode provocá-lo e isso não deve ser apresentado como suavidade comprovada. | Corrigida dependência de contagem de frames; novo pacote gerou pedidos com intervalos de ~2,04–2,12 s numa sequência de 2000 ms, também com separações maiores; a política reinicia a referência ao receber IDRs. Desligado e AV1: zero pedidos periódicos nos trechos coletados. Hitches físicos pendentes |
| ABR | Exercitar Balanced/Quality/Low latency, reconectando. Comparar cena e condição de rede conhecidas, logs de amostra/decisão e valor aceito pelo host. Sair e iniciar outra sessão. | Ajustes respeitam host/backend; não aumentam por ausência de amostra; sem ajuste depois de encerrar a sessão, hang no teardown ou estado reaproveitado indevidamente. | Três modos ajustaram valores aceitos pelo host e completaram Stop, sem atividade ABR posterior na coleta. Respeitado teto 95000; perdas de rede/áudio ainda observadas. Comparação controlada de movimento e qualidade pendente |
| Reset | Alterar opções, cancelar o reset e conferir preservação. Confirmar depois, reabrir e reiniciar, conferindo defaults e pareamento. | Cancelar conserva valores; confirmar atualiza interface e persistência; pareamento continua; Voltar e reabertura funcionam sem queda. | Pendente |
| Menus e controle | Percorrer Stream/Input/Host/Outros, sliders, dropdowns, popups e confirmações usando D-pad e ponteiro real. | Vídeo/ABR em Stream; preferências gerais em Outros; foco/rolagem/Voltar acessíveis e previsíveis. | Pendente |

## Evidências e limite da rodada

O primeiro pacote (`94dc5af2…`, ELF `ef8a0e52…`) registrou a integração ABR e
HEVC slices. O segundo (`dc2e20cc…`, ELF `d1c5e52a…`) incluiu somente a correção
posterior do temporizador HEVC e repetiu refresh, desligamento, SDR full range,
HDR e gating em AV1, além da abertura final com a configuração restaurada.
Seis CTests e o fixture do temporizador com ASan/UBSan passaram; os testes
anteriores de UI localizada/protocolo continuam registrados no relatório de revisão.

Os logs de HDR confirmam sinal do host e metadados NDL com
`transferCharacteristics=16`, `colorPrimaries=9`, `matrixCoeffs=9`.
Isso não mede cores na tela. Perdas de frames/áudio foram observadas nesta rede;
a causa não foi isolada. O usuário ainda não respondeu à solicitação de confirmar
imagem, áudio, controle e menu Outros; nenhum desses resultados foi presumido.

## Registro de resultado

Para cada falha, registrar configuração efetiva, reprodução mínima e anexos de
log; corrigir e repetir o caso afetado e a navegação de saída/reabertura. Uma
observação sem o backend/codec efetivo não valida a opção. Critério ainda sem
resultado ou inconclusivo permanece pendente.

Os resultados locais e as correções de código estão em
[EXPERIMENTAL_FEATURE_REVIEW.md](EXPERIMENTAL_FEATURE_REVIEW.md). A estabilidade
física só pode ser encerrada depois da execução e conferência desta rodada.
