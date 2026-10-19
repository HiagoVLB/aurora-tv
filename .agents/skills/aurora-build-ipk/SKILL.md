---
name: aurora-build-ipk
description: "Compila, empacota e valida o Aurora para webOS ARM, entregando sempre aurora.ipk na raiz do projeto. Use para gerar/regerar o instalador IPK atual; preserva o pacote anterior em falhas e não incrementa a versão nem instala na TV."
---

# Gerar o IPK do Aurora

Localizar a raiz Aurora por `AGENTS.md` e `CMakeLists.txt`; ler
[AGENTS.md](../../../AGENTS.md) e a seção de versão/build do
[estado atual](../../../docs/CURRENT_STATE.md). Entregar **`aurora.ipk` nessa
raiz**, não na pasta que contém os repositórios irmãos. Manter o mesmo nome a
cada build; a versão interna continua sendo a versão central do projeto.

## Executar

Na raiz Aurora:

```sh
python3 scripts/dev/aurora_version.py --check
python3 scripts/webos/build_ipk.py
```

Usar Python 3.9 ou posterior. O helper seleciona Docker Linux/amd64 no Mac e em
ambientes sem SDK configurado; em Linux com `TOOLCHAIN_FILE` válido, seleciona
build nativo. Para escolher explicitamente:

```sh
python3 scripts/webos/build_ipk.py --backend docker --jobs 6
python3 scripts/webos/build_ipk.py --backend native --build-type Release
```

O SDK nativo deve fornecer também `cmake`, `cpack`, `ares-package`, toolchain ARM
e dependências de build. No Docker, o helper reutiliza/cria a imagem
`aurora-webos-builder:local` e o volume `aurora-webos-build-cache`; downloads
iniciais usam as versões fixadas pelo projeto. O source é montado somente para
leitura; SDK e compilação permanecem no volume, artefatos/logs em `.cache/ipk/`.
Não executar dois builds ao mesmo tempo no mesmo volume/diretório de compilação.

Se faltarem submódulos, inicializá-los recursivamente em clone novo. Em checkout
existente, examinar patches antes; o helper não faz pull/reset/update de fontes.
Se Docker não estiver disponível, usar um SDK Linux existente ou relatar a
dependência concreta; não substituir o cross-build pelo simulador de UI.

## Aceitar somente a entrega verificada

O helper gera o pacote numa pasta nova por execução, valida control/appinfo,
ID `com.aurora.gamestream`, versão, arquitetura ARM e marca de versão do ELF.
Compara o estado das fontes antes/depois para rejeitar build durante mudanças.
Somente depois substitui `aurora.ipk` atomicamente. Falha de compilação ou
validação preserva o instalador anterior; não o apresentar como um build novo.

Consultar `.cache/ipk/latest.json`: versão, SHA-256, commit/estado local,
backend, tipo de build e diretório dos logs. Em erro, ler o `build.log` da
execução indicada e corrigir a causa antes de repetir. Não escolher um IPK
antigo de `dist` pelo mtime nem renomeá-lo como se fosse compilado agora.

Não incrementar a versão como efeito colateral de gerar o IPK. Se o pedido
incluir incremento, executar primeiro
[aurora-bump-version](../aurora-bump-version/SKILL.md). Não criar tags, release,
push ou instalar na TV como etapa automática deste fluxo.

Informar caminho clicável de `aurora.ipk`, versão interna, hash e resultado da
validação. Distinguir pacote compilado de funcionamento na C2: verificação de
ELF/metadados não valida streaming, áudio nem instalação. Atualizar a área de
build do estado atual com o resultado efetivamente obtido.
