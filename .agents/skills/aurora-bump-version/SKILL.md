---
name: aurora-bump-version
description: "Incrementa a versão central do Aurora em CMakeLists.txt e confere a derivação da versão no appinfo, APP_VERSION e marca do ELF. Use quando pedirem aumento/troca de versão; patch por padrão, sem gerar IPK, criar tag ou publicar release automaticamente."
---

# Incrementar a versão do Aurora

Localizar a raiz Aurora e ler [AGENTS.md](../../../AGENTS.md). Usar
`MOONLIGHT_VERSION` no `CMakeLists.txt` como única versão editável do produto.
`deploy/webos/appinfo.json` usa `@PROJECT_VERSION@`, a UI usa `APP_VERSION`, e
a marca `aurora-v` do executável deve concatenar `APP_VERSION`.

## Aplicar o incremento solicitado

Na raiz Aurora, conferir e aplicar:

```sh
python3 scripts/dev/aurora_version.py --check
python3 scripts/dev/aurora_version.py --part patch --apply
python3 scripts/dev/aurora_version.py --check
```

Se o usuário disser apenas "incrementar a versão", usar **patch**. Respeitar
minor/major ou uma versão exata quando especificados:

```sh
python3 scripts/dev/aurora_version.py --part minor --apply
python3 scripts/dev/aurora_version.py --part major --apply
python3 scripts/dev/aurora_version.py --set 1.4.0 --apply
```

Esses são exemplos alternativos, não uma sequência de incrementos. Omitir
`--apply` para apenas simular. Usar números `X.Y.Z`, sem prefixo `v`, sufixos
ou zeros iniciais. Rejeitar a mesma versão ou um downgrade. Não incrementar
duas vezes por repetir a validação: `--check` não altera arquivos.

Se o check acusar contrato quebrado (versão duplicada, appinfo literal ou marca
do ELF desatualizada), inspecionar/corrigir esse contrato no escopo do pedido.
Não substituir versões por busca global: patches históricos, relatórios,
submódulos e ferramentas de recursos têm versões independentes.

## Registrar e compor com o build

Conferir o diff: a operação normal altera apenas `MOONLIGHT_VERSION`. Atualizar
a versão declarada e o registro de alterações em
[CURRENT_STATE.md](../../../docs/CURRENT_STATE.md), sem reescrever a identidade
dos pacotes históricos. Informar versão anterior e nova.

Um IPK anterior, inclusive `aurora.ipk`, continua com a versão antiga até ser
recompilado. Se o pedido incluir gerar instalador, executar depois a skill
[aurora-build-ipk](../aurora-build-ipk/SKILL.md); ela substitui o arquivo fixo
somente após verificar a versão nova. Não editar o conteúdo de IPK pronto.

Não criar commit/tag, release, push ou instalar na TV como efeito colateral do
incremento. Não incrementar agora quando o pedido for apenas criar/manter esta
skill; validar seu comportamento em cópia temporária nesses casos.
