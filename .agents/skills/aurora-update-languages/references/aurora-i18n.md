# Mapa e comandos de localização

## Arquivos que determinam o comportamento

| Contrato | Arquivos |
| --- | --- |
| Catálogos e fonte | `src/i18n/*/messages.po`, `src/i18n/messages.pot`, usos de `locstr`/`translatable` em `src/` |
| Lista empacotada | `I18N_LOCALES` em `CMakeLists.txt` |
| Seletor e normalização | `src/app/platform/common/i18n_common.c` |
| Desktop gettext | `cmake/MoonlightI18n.cmake`, `src/app/platform/common/{app_common,i18n_gettext}.c` |
| webOS JSON/runtime | `cmake/PackageWebOS.cmake`, `scripts/webos/po2json.awk`, `src/app/platform/webos/i18n_webos.c` |
| Fontes e fallback | `src/app/util/font.c`, `FONT_FAMILY`/`FONT_FAMILY_FALLBACK` e Fontconfig |
| UI real para revisão | `tools/ui-sim/{README.md,main.c,run.sh,run.py,Dockerfile}` |

Inventário inicial (18/10/2026; reconfirmar): 16 catálogos, mas somente 13 em
`I18N_LOCALES`. `ko` está no seletor e tem catálogo, mas não está nessa lista;
`uk` e `zh-TW` têm catálogos sem entrada no seletor. `ja` está condicionado a
DEBUG no seletor. Inglês é a fonte, sem PO separado. Não confundir atualização
dos catálogos existentes com habilitação de novos idiomas.

O webOS usa JSON flat, não a semântica completa de gettext. O AWK atual junta
linhas, ignora traduções vazias, mas não filtra fuzzy nem implementa contextos
e plurais. Remover fuzzy sem revisar mascara uma divergência entre desktop/TV.
O auditor acusa esses casos e compara o JSON efetivo ao conteúdo esperado do PO.

## Preparar ferramentas

Usar gettext (`xgettext`, `msgmerge`, `msgfmt`), AWK e Python com `polib`.
Reutilizar ambiente existente; se necessário, criar venv local ignorado:

```sh
python3 -m venv .cache/i18n/venv
.cache/i18n/venv/bin/python -m pip install polib
.cache/i18n/venv/bin/python .agents/skills/aurora-update-languages/scripts/audit_catalogs.py \
  --repo . --output .cache/i18n/before.json
```

Sem gettext no Mac, usar um ambiente Linux já preparado, como a imagem
`aurora-ui-sim:local`, se disponível. `msgfmt` ausente é check pendente e saída 2,
não aprovação. Não instalar dependências no Python global nem usar `sudo` como
primeira opção. O auditor também exige AWK e `polib` nesse ambiente.

## Extrair e mesclar

Na raiz Aurora, com Python e gettext no mesmo ambiente, gerar POT temporário:

```sh
mkdir -p .cache/i18n
python3 - <<'PY'
from pathlib import Path
import subprocess
files = sorted(str(p) for p in Path('src').rglob('*') if p.suffix in {'.c', '.h'})
subprocess.run(['xgettext', '--from-code=UTF-8', '--keyword=locstr',
                '--keyword=translatable', '--add-comments',
                '-o', '.cache/i18n/current.pot', *files], check=True)
PY
```

Comparar com o POT versionado, preservar entradas locais legítimas e atualizar
o arquivo fonte depois da inspeção. Mesclar cada PO do escopo separadamente:

```sh
msgmerge --update --backup=none src/i18n/pt-BR/messages.po src/i18n/messages.pot
msgfmt --check --check-format -o .cache/i18n/pt_BR.mo src/i18n/pt-BR/messages.po
```

Não executar este exemplo apenas em pt-BR quando o pedido cobrir todos os idiomas.
Para auditar contra extração temporária, usar `--template .cache/i18n/current.pot`.
Comparar antes/depois pelo mesmo POT atual para medir cobertura com a mesma base.
Examinar stderr inclusive com exit code zero: headers podem gerar warnings.

## Preparar verificação visual localizada

O desktop carrega o domínio `moonlight-tv`. Os MO ficam em
`<raiz-mo>/<locale_com_underscore>/LC_MESSAGES/moonlight-tv.mo`.
Apontar `TEXTDOMAINDIR` para `<raiz-mo>` e garantir que o locale UTF-8 exista no
Linux (`locale -a`). O alvo `aurora-ui-sim` isolado pode não compilar o alvo
`moonlight-i18n`; conferir e compilar/gerar explicitamente os MO do teste.

No estado inicial, a imagem não prepara locales nem fontes CJK e o wrapper não
encaminha opções de idioma/TEXTDOMAINDIR. Implementar o suporte necessário quando
executar uma atualização, em vez de anunciar opções ainda inexistentes. Conferir
a ordem real: `main.c` fixa a preferência em `en`, mas `app_common.c` inicia o
desktop com `setlocale("")`, usando o ambiente. Os controladores de configurações
também chamam `i18n_setlocale` com a preferência. Alinhar ambiente e preferência
para evitar mudança de idioma ao navegar. Trocar preferências depois de criar
widgets pode exigir reinicialização da UI.

Reaproveitar as ações abstratas (`settings`, `help`, `add_host`), mas traduzir
`click_text` e `expect` por idioma. Criar asserção para texto exclusivo traduzido;
siglas iguais ao inglês não provam carregamento do catálogo. Inspecionar PNGs
e logs em `.cache/ui-sim-run-*/artifacts/`.

Para ko/ja/zh e cirílico, verificar glifos efetivamente renderizados. Fontes
adicionais no Docker validam o simulador, não a cobertura das fontes da LG.
Manter tabela de idioma → tela → resolução → captura → resultado/pendência.
