---
name: aurora-update-languages
description: "Atualiza e revisa os idiomas existentes do Aurora para webOS: sincroniza gettext, traduz conforme o contexto e o uso natural de cada língua, preserva placeholders e confere recursos cstrings, fontes, cortes e navegação na interface real. Use para atualizar traduções, completar idiomas, revisar localização ou corrigir textos traduzidos que não cabem/aparecem incorretamente no app."
---

# Atualizar os idiomas do Aurora

Localizar a raiz Aurora e ler [AGENTS.md](../../../AGENTS.md). Executar o trabalho
até tradução, validação técnica e inspeção visual dos idiomas afetados. Se o
pedido for criar/manter esta skill, testar em fixtures/cópias temporárias sem
atualizar os catálogos reais como efeito colateral.

## Definir o conjunto e o contexto

1. Conferir `git status --short`; preservar alterações locais, traduções e créditos.
2. Inventariar **todos** os `src/i18n/*/messages.po`, `I18N_LOCALES` no
   `CMakeLists.txt` e o registro em `src/app/platform/common/i18n_common.c`.
   Distinguir catálogo existente, idioma selecionável, condicionado a DEBUG e
   recurso empacotado. Não limitar a tarefa a pt-BR nem supor que essas listas
   coincidem. Incluir os catálogos existentes na atualização geral; conferir
   visualmente os selecionáveis/afetados e explicar os que não são acessíveis.
3. Respeitar um subconjunto explicitamente solicitado. Não adicionar idiomas nem
   habilitar catálogos inativos incidentalmente. Investigar e corrigir recursos
   ausentes de um idioma já selecionável quando necessário para ele funcionar.
4. Ler cada string nova/alterada no código consumidor: tela, ação, estado,
   tamanho disponível, formatação e terminologia próxima. Usar o inglês atual
   como fonte de significado; não traduzir todos os idiomas a partir de pt-BR.
5. Ler [references/aurora-i18n.md](references/aurora-i18n.md) antes de sincronizar
   ou testar. Reconfirmar os pontos mutáveis no checkout atual.

## Sincronizar os catálogos

- Extrair os textos atuais de `src/` com as mesmas keywords `locstr` e
  `translatable` do CMake. Gerar POT temporário primeiro e conferir entradas,
  duplicatas e mudanças locais antes de substituir `src/i18n/messages.pot`.
  Incluir strings concatenadas/multilinha e cabeçalhos `.h`.
- Não confiar só no POT existente: ele pode ter textos removidos ou não conter
  telas novas. Não renomear msgids para facilitar tradução; corrigir fonte apenas
  quando houver problema real de texto ou texto visível sem localização.
- Mesclar cada catálogo do escopo com `msgmerge --update --backup=none` usando
  o POT conferido. O alvo `i18n-merge-po` atual só atualiza japonês; não usá-lo
  como se cobrisse todos os idiomas.
- Revisar mensagens vazias, `fuzzy`, traduções herdadas que mudaram de sentido e
  textos iguais à fonte. Remover `fuzzy` **somente depois de revisar significado
  e tradução**, nunca em lote para aumentar a cobertura. Conservar termos iguais
  à fonte quando naturais, como HDR, FPS e nomes próprios.
- Preservar UTF-8, comentários úteis, metadados e variantes regionais. Conferir
  `Language` e `Plural-Forms`; não inventar nome/email de tradutor.

## Traduzir como interface de cada língua

- Adaptar vocabulário, ordem das palavras, tratamento, capitalização e pontuação
  ao uso natural da variante. Preferir ações curtas e claras; não fazer tradução
  literal palavra por palavra nem generalizar uma variante regional para outra.
- Manter glossário consistente por idioma entre configurações, ajuda, erros,
  menus e overlay. Distinguir controle da TV, gamepad e ponteiro/mouse virtual.
  Preservar a distinção entre Disconnect (sair do stream mantendo o app no host),
  End session (encerrar no host), Resume, Remote Input e Remote Monitor.
- Preservar nomes de produto, protocolos, teclas e combinações quando forem
  identificadores; explicar a função em linguagem local quando ajudar o usuário.
  Não inventar capacidades nem mudar instruções de atalhos para caber no layout.
- Preservar argumentos printf (`%s`, `%d`, `%u`, `%zu`, precisão, `%%` e `*`), tipos,
  número e associação com os valores. Reordenar argumentos posicionais somente
  se o consumidor/plataformas suportarem e a validação confirmar. Conferir flags
  `c-format` nos usos reais de printf; `msgfmt` não cobre strings sem essa flag.
- Preservar quebras com função, itens separados por `\n` em dropdowns, ordem dos
  índices/opções, símbolos LVGL, markup, URLs e unidades técnicas. Adaptar frases
  ao redor desses tokens sem alterar seu contrato.
- Quando a tradução ficar longa, reformular preservando a informação. Se ainda
  não couber, ajustar minimamente wrap/altura/rolagem no componente afetado e
  verificar os outros idiomas. Não remover acentos, reduzir toda a fonte nem
  omitir avisos como solução genérica.
- Revisar cada idioma em uma segunda passagem, comparando tradução, contexto e
  glossário. Registrar dúvidas específicas; não apresentar preenchimento de
  msgstr como revisão linguística concluída.

## Validar os dois caminhos de recursos

Executar o auditor incluído antes e depois, usando Python com `polib` e gettext
disponíveis. Seguir a preparação na referência. Na raiz Aurora:

```sh
python3 .agents/skills/aurora-update-languages/scripts/audit_catalogs.py \
  --repo . --output .cache/i18n/after.json
```

Usar `--locale pt-BR --locale fr` apenas para escopo restrito. O auditor não
modifica fontes; informa por idioma cobertura, fuzzy, duplicatas, falhas gettext,
igualdade com inglês e diferenças entre PO e o JSON produzido pelo AWK real.
Corrigir falhas no escopo e executar novamente. Ele não substitui a extração do
POT atual, a revisão semântica, a inspeção de flags printf ou a conferência visual.

Verificar também catálogo → caminho instalado → locale escolhido em runtime.
Conferir `resources/pt/BR/cstrings.json`, por exemplo, sem trocar silenciosamente
códigos como `es-ES` por `es`. Não introduzir contextos/plurais gettext sem
resolver seu suporte no conversor flat JSON e no consumidor webOS.

## Conferir a exibição de verdade

1. Ler `tools/ui-sim/main.c`, `run.sh`, `run.py` e README. No estado inicial desta
   skill, o driver fixa a preferência `settings->language = "en"`, mas o desktop
   inicia com `setlocale("")` em `app_common.c`. Conferir a ordem real e as
   reaplicações da preferência nas configurações: o literal isolado não
   determina o locale inicial. Preparar locale, compilar MO e definir
   `TEXTDOMAINDIR`; provar no log/árvore e em texto exclusivo que o idioma foi
   carregado e permanece correto ao navegar. Rodar o padrão em inglês não valida
   traduções; não presumir que apenas mudar `LANG` basta.
2. Se faltar seleção de idioma no simulador, implementar o suporte mínimo de
   teste necessário, preservando o estado isolado e o default do produto. Propagar
   a opção pelo Docker e preparar o locale Linux. Não trocar apenas um literal
   de inglês por outro idioma fixo. Conferir fontes/fallback no ambiente.
3. Criar roteiros por idioma com expectativas traduzidas. Abrir launcher,
   configurações Stream/Input/Host/Experimental, dropdown de idioma, ajuda/atalhos,
   erros e confirmações afetadas. Exercitar overlay em fixture segura quando
   possível; registrar explicitamente telas que não puderam ser exercitadas.
4. Capturar e **abrir/inspecionar** PNGs da UI real, junto às árvores de elementos,
   em 1920×1080 e 1280×720, ou resoluções equivalentes justificadas pelo alvo.
   Conferir cortes, reticências que escondem informação, sobreposição, quebras,
   acentos, CJK/cirílico, glifos ausentes, contraste e alinhamento. Comprimento
   de string, árvore com texto completo e `report.json` aprovado não provam
   que todo o texto foi exibido corretamente.
5. Navegar com D-pad/controle, rolar mensagens longas e fechar com Back/Escape;
   conferir retorno de foco, seleção de opções e reabertura. Repetir capturas
   afetadas após corrigir qualquer corte ou ajuste no layout.
6. Distinguir fontes/locale do Linux das fontes e bundle da LG. Validar recursos
   webOS localmente; relatar separadamente qualquer teste real na C2. Se Docker,
   locale, fontes ou TV não estiverem disponíveis, concluir o trabalho possível
   e declarar a pendência visual/física sem marcar a verificação como aprovada.

## Entregar com evidência

Conferir o diff, links e traduções; atualizar a área correspondente de
`docs/CURRENT_STATE.md` com idiomas, cobertura antes/depois, checks, caminhos de
capturas e pendências. Relatar o que foi adaptado, quais idiomas foram vistos
na UI e quais validações ficaram limitadas ao catálogo.

Não gerar IPK, incrementar versão, instalar na TV, publicar ou fazer commit/push
por consequência desta skill. Se o pedido incluir IPK ou versão, executar as
skills correspondentes depois da revisão dos idiomas.
