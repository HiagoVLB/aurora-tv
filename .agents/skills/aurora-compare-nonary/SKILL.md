---
name: aurora-compare-nonary
description: "Compara o Aurora atual com Nonary/moonlight-qt para avaliar comportamento, recursos e viabilidade de adaptação ao webOS/LG C2, incluindo pacing, VRR, codecs e diagnóstico; atualiza a linha de base com evidências e limitações."
---

# Comparar Aurora com Moonlight Qt do Nonary

Avalie o que o cliente desktop oferece, o que o Aurora já implementa e quais
adaptações são justificáveis no backend real da LG. O fork de referência é
`https://github.com/Nonary/moonlight-qt`, não apenas o Moonlight Qt upstream.

## Identificar os dois estados

Localize a raiz Aurora pelo `AGENTS.md`/`CMakeLists.txt` e leia
[AGENTS.md](../../../AGENTS.md), [CURRENT_STATE.md](../../../docs/CURRENT_STATE.md)
e as partes relevantes de
[NONARY_MOONLIGHT_WEBOS_REVIEW.md](../../../docs/NONARY_MOONLIGHT_WEBOS_REVIEW.md).
O estado atual é uma linha de base datada, não comprovação de que a branch remota
continua no mesmo commit.

Use o checkout de referência informado pelo usuário; alternativamente,
`../moonlight-qt` se pertencer ao fork Nonary, ou um clone isolado em
`.cache/references/nonary-moonlight-qt`. Não dependa de um caminho antigo em
`/tmp`. Não faça reset/pull sobre trabalho existente. Se o pedido exigir a
última versão, consulte o SHA da branch padrão remota e examine um checkout
isolado desse commit. Em comparação local, registre a ausência de verificação
de atualização remota. Informe o commit efetivamente lido em todos os casos.

Na raiz Aurora, com o caminho real da referência:

```sh
python3 scripts/dev/comparison_snapshot.py --reference nonary=../moonlight-qt --output .cache/comparisons/nonary-state.json
```

O helper registra também alterações ainda sem commit e submódulos; ele não
atualiza referências pela rede.

## Comparar comportamento e dependências de plataforma

Rastreie opção → armazenamento → configuração efetiva → protocolo → decoder →
apresentação. Identifique defaults, exclusões entre opções, campos ausentes em
perfis, fallback e ajustes que só se aplicam ao reconectar.

| Tema | Fork Nonary | Aurora |
| --- | --- | --- |
| Preferências e interface | `app/gui/SettingsView.qml`, `app/settings/streamingpreferences.*` | `src/app/app_settings.c`, `src/app/profile/`, `src/app/ui/settings/settings.controller.c`, `panes/` |
| Sessão/protocolo | `app/backend/nvhttp.cpp`, `app/streaming/session.cpp`, `moonlight-common-c/` | `core/libgamestream/`, `core/moonlight-common-c/`, `src/app/stream/session{,_worker}.c` |
| VRR e apresentação | `app/streaming/vrrratepolicy.*`, `app/settings/vrrtimingoptions.h`, `app/streaming/video/ffmpeg-renderers/ivrrframepresenter.h`, `pacer/` e renderers concretos | `src/app/stream/video/`, `third_party/ss4s/modules/webos/ndl/`, `smp/` |
| Telemetria | `app/diagnostics/`, `docs/vrr-diagnostics.md` e estudos atuais em `docs/` | `src/app/stream/video/session_video.c`, `src/app/ui/streaming/`, `docs/FRAME_DIAG.md` |
| Codecs/novos caminhos | `docs/pyrowave-protocol.md`, implementações/negociação em `app/` | negociação em `session.c` e capacidades/feed SS4S |

Compare conceitos e contratos; não proponha copiar QML/C++/DXGI diretamente
para LVGL/C11/NDL. Leia o backend concreto e as condições de ativação, além
do texto da UI. Confira se opções descritas em estudos antigos foram retiradas.

Para cada recurso, classifique: já equivalente; adaptação viável; depende de
configuração do host; pesquisa de backend necessária; ou incompatível com o
caminho atual. Indique custo de latência, memória/rede, fallback e observação
necessária para decidir. Um benefício em desktop é hipótese até medir na LG.

Cuidados específicos desta comparação:

- Pacing, V-Sync, VRR e refresh de captura do host não são sinônimos. Exija um
  contrato de apresentação que o backend webOS realmente possa cumprir.
- 120 submits/s não provam 120 frames distintos exibidos. Não chamar tempo de
  `Play` de decode nem fila estimada de latência física.
- Respeite o histórico de tentativas e os invariantes em `AGENTS.md`, incluindo
  direct submit, áudio 5.1 e experimentos de PTS já descartados na C5. Resultados
  da C5 não são testes da C2; uma nova hipótese precisa explicar a diferença.
- Capacidade HDMI, capacidade de codec/NDL e taxas do compositor são distintas.
  Não elevar limites SS4S apenas com base em especificações do painel.
- Se a conclusão depende de SDR Main10, controles sintéticos ou flags de
  launch, confira o lado VibeShine; use a skill irmã quando a investigação do
  host for substancial. O nome de uma flag sozinho não define seu efeito.

## Evidências e manutenção da linha de base

Registre commits/estado local, arquivos e símbolos dos dois clientes, comportamento
efetivo, compatibilidade webOS e prioridade justificada. Execute testes pertinentes
quando eles puderem verificar o contrato em questão; um teste de algoritmo ou
simulador de UI não valida apresentação física. Se a comparação não incluir TV,
descreva o teste de hardware que falta sem declarar o recurso validado.

Atualize as áreas examinadas de `docs/CURRENT_STATE.md`: data, referências,
diferenças desde o estado anterior, resultados realmente executados, limitações
e próximos passos. Corrija premissas superadas com uma nota de correção; preserve
evidências e conclusões da comparação VibeShine. Uma revisão extensa pode ficar
em `docs/comparisons/AAAA-MM-DD-nonary.md`, resumida e ligada no estado atual.
Uma análise não deve aparecer como funcionalidade já implementada ou instalada.
