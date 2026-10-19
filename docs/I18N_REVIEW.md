# Revisão dos idiomas do Aurora — 19/10/2026

Os 16 catálogos existentes foram sincronizados com o código e revisados por
contexto. A base inicial tinha 210 mensagens extraídas; localizar o indicador
`%d * %d (Native)` elevou a base final a **211**. Textos removidos do código
permanecem como entradas obsoletas, sem substituir traduções atuais.

## Cobertura

Contagem de mensagens atuais preenchidas e sem fuzzy no início e ao final.
Nomes técnicos, identificadores de botões e siglas podem coincidir com o inglês.

| Idioma | Inicial | Final | Idioma | Inicial | Final |
| --- | ---: | ---: | --- | ---: | ---: |
| cs | 67 | 211 | de | 46 | 211 |
| es-ES | 39 | 211 | fr | 77 | 211 |
| it | 48 | 211 | ja | 49 | 211 |
| ko | 0 | 211 | nl | 67 | 211 |
| pl | 39 | 211 | pt-BR | 99 | 211 |
| ro | 37 | 211 | ru | 54 | 211 |
| tr | 39 | 211 | uk | 0 | 211 |
| zh-CN | 72 | 211 | zh-TW | 0 | 211 |

A adaptação usa o inglês do código como fonte e mantém terminologia consistente
por idioma entre ajuda, menus e configurações. As variantes chinesas foram
adaptadas separadamente. Disconnect mantém o aplicativo aberto no host; End
session solicita encerramento após confirmação. Essas ações continuam distintas
nas traduções. Nomes de teclas/combinações, placeholders e unidades técnicas
preservam os contratos dos consumidores.

O coreano já constava no seletor, mas não era empacotado: agora integra os 14
catálogos de `I18N_LOCALES`. Os catálogos `uk` e `zh-TW` foram atualizados e
exercitados no simulador, sem habilitação no produto. A opção de japonês continua
condicionada a DEBUG. Inglês permanece como idioma fonte e fallback.

## Validação executada

- Auditor da skill: 16 catálogos aprovados, sem mensagens atuais ausentes,
  vazias ou fuzzy; `msgfmt --check --check-format` aprovado, sem duplicatas,
  contextos/plurais incompatíveis ou diferenças na saída do AWK real do webOS.
- Cada placeholder printf das mensagens extraídas possui `c-format`. Os tipos,
  quantidade e ordem dos argumentos foram preservados.
- Interface SDL/LVGL real, com backends dummy e runtime sem rede: **80 etapas por
  idioma**, cobrindo ajuda, rolagem, configurações, dropdown de resolução, painéis
  Input/Host/Experimental, confirmação de reset cancelada, adicionar PC cancelado,
  confirmação de saída cancelada e retorno por Escape. Cada roteiro passa por
  1920×1080 e 1280×720 e encerra o processo normalmente: **1.280 etapas aprovadas**.
- Após localizar a resolução nativa, **8 etapas adicionais por idioma** verificam
  o valor traduzido do dropdown em 720p e a navegação/saída: **128 etapas aprovadas**.
- Capturas de cada idioma foram abertas para inspeção de quebra de linhas,
  sobreposições, acentos, cirílico e CJK. A cobertura Fontconfig de fonte principal
  e fallback não apontou caracteres ausentes nas traduções do ambiente Linux.
- Oito testes de `tests/scripts/test_release_helpers.py`, sintaxe do wrapper Bash
  e `git diff --check` aprovados.

Os testes revelaram e motivaram as correções da barra superior compacta e do
estado de Voltar após um dropdown. O caso de truncamento também confirmou que
`LV_LABEL_LONG_DOT` precisa de texto copiado: sua implementação escreve pontos
no buffer, o que não é permitido em strings estáticas/read-only do gettext.

## Reprodução e artefatos locais

Na raiz do Aurora, com Docker disponível:

```sh
AURORA_UI_SIM_LANGUAGE=pt-BR ./tools/ui-sim/run.sh tools/ui-sim/localization.json
```

Trocar `pt-BR` pelo código do catálogo desejado. O runner traduz as expectativas
com o MO real e o log registra `UI locale` e o nome localizado do idioma.

Os artefatos desta revisão ficam em `.cache/i18n/`, ignorada pelo Git:

- `before.json`, `after.json`: auditoria dos catálogos;
- `visual-complete/matrix.json`: resultados e caminhos por idioma, com relatório,
  capturas, árvores de UI e logs de cada execução;
- `native-check/matrix.json`: validação do indicador de resolução nativa;
- `contacts/`: painéis de capturas usados na inspeção visual;
- `font-coverage.json`: cobertura das fontes do ambiente Linux.

## Limites

Os testes não executaram o firmware LG nem uma sessão real com host. Não
comprovam fontes disponíveis na C2, carregamento de `ResBundleC`, seletor de
idioma específico de webOS, envio físico dos atalhos, pareamento com PC, erros
de host reais ou o overlay durante streaming. A ajuda e os contratos desses
textos foram revisados no código; os cenários sem fixture acessível continuam
dependentes de validação na TV. Não houve IPK, instalação ou mudança de versão.
