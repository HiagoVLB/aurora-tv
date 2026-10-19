# Simulador visual do Aurora

Executa **a interface real SDL/LVGL do Aurora** em Linux no Docker, com decoder
SS4S dummy, entrada por teclado/mouse e capturas PNG de 1920 × 1080. Funciona no
Mac pelo Docker Desktop. Validado em Linux ARM64 dentro do Docker no Mac.

A arquitetura segue o [simulador do PlxNative](https://github.com/GLinnik21/plx-native/blob/3ac9e03fd4ef8b3c3bf1061c6eb372d970a6a3da/docs/building.md):
executável de desenvolvimento separado, estado isolado por execução, envio de
comandos, capturas e validação automática. A referência foi consultada no commit
`3ac9e03fd4ef8b3c3bf1061c6eb372d970a6a3da`, incluindo `tools/sim-smoke.py`.
Este adaptador é uma implementação própria para o Aurora; não executa o cliente
Plex nem o firmware da LG.

## Executar no Mac

Com o Docker Desktop em execução, a partir da raiz do repositório:

```sh
./tools/ui-sim/run.sh
```

Para conferir traduções em 1920×1080 e 1280×720, usar o roteiro de localização:

```sh
AURORA_UI_SIM_LANGUAGE=pt-BR ./tools/ui-sim/run.sh tools/ui-sim/localization.json
```

O wrapper aceita os códigos dos catálogos existentes, prepara o locale UTF-8
correspondente e usa os MO compilados em `/build/mo`. A imagem inclui fontes CJK
para a inspeção de japonês, coreano e chinês. Os textos de `expect`, `absent`,
`click_text` e `scroll_text` são traduzidos pelo catálogo real do teste; um idioma
sem MO faz a execução falhar. O relatório registra idioma/locale, e o log do app
mostra o catálogo carregado. Os catálogos inativos `uk` e `zh-TW` também podem ser
examinados no simulador; isso não os habilita no seletor nem no pacote webOS.

As fontes Linux não comprovam os glifos disponíveis na TV. Abrir as capturas
para conferir cortes e glifos; expectativas de texto sozinhas não validam layout.

A primeira execução cria a imagem Ubuntu 24.04 e compila o aplicativo. As seguintes
reutilizam a imagem e um volume de compilação por arquitetura. Ubuntu 24.04 fornece
SDL 2.30; a versão de SDL do Ubuntu 22.04 não contém todos os tipos de controle
usados pelo Aurora.

O comando imprime o diretório de resultados em `.cache/ui-sim-run-*/artifacts/`:

- `index.html`: relatório visual, com uma captura por etapa;
- `report.json`: resultado das ações e verificações, incluindo falhas na finalização;
- `step-NNN.png`: captura do renderer real;
- `step-NNN.json`: elementos visíveis, textos, coordenadas, foco e estado desabilitado;
- `process.log`: stdout/stderr do processo;
- `state/`: preferências, perfis e `aurora.log` exclusivos da execução.

Os logs da imagem e compilação ficam um nível acima, em `image-build.log` e
`compile.log`. O roteiro padrão verifica as configurações de Stream, Input, Host
e Experimental, o cancelamento de adicionar PC e o cancelamento da saída por
mouse e pela tecla Escape. As opções de Stream ficam na lista principal de
configurações; as outras três abrem painéis secundários.

O runtime usa `--network none`, fonte e compilação montadas somente para leitura,
e nenhum dispositivo/credencial da TV. Nenhum host real pode receber `/cancel`
nesses testes. O próprio aplicativo e seu backend continuam executando: uma falha
de mDNS pela ausência de rede, por exemplo, é exercitada de verdade.

## Roteiros

```sh
./tools/ui-sim/run.sh /caminho/roteiro.json

# Inicializa a interface e as fontes na resolução escolhida (até 3840x2160).
AURORA_UI_SIM_SIZE=1280x720 ./tools/ui-sim/run.sh
```

Exemplo:

```json
{
  "name": "configuracoes",
  "steps": [
    {"action": "settings"},
    {"expect": "Resolution"},
    {"click_text": "Experimental"},
    {"expect": "Show logs"},
    {"key": "Down"},
    {"key": "Escape"}
  ]
}
```

Comandos de roteiro:

| Comando | Argumento |
| --- | --- |
| `action` | `settings`, `add_host`, `help` ou `exit`; clica no botão real da barra |
| `click_text` | Texto exato, em inglês; rola até o item e clica, inclusive em botões de diálogos |
| `scroll_text` | Rola até o texto sem clicar; útil para inspecionar seções e mensagens |
| `click` | Coordenadas, por exemplo `"100 200"` |
| `key` | Nome SDL: `Down`, `Up`, `Return`, `Escape`, etc. |
| `text` | Evento SDL de texto curto, até 31 bytes UTF-8, para o campo em foco |
| `wait` | Tempo de 0 a 10.000 ms; mantém o loop da interface processando |
| `wheel` | Número de passos da rodinha SDL; negativo desce, positivo sobe |
| `resize` | Nova área da janela, por exemplo `"1280 720"`; preserva o menu aberto |
| `expect` | Texto que precisa estar visível |
| `absent` | Texto que não pode estar visível |

Cada ação produz captura e árvore de elementos. Falhas devolvem status de processo
não zero e preservam os resultados; não são convertidas em teste aprovado.
Capturas precisam ser PNG válido, ter a resolução da execução (ou do último
`resize`) e conteúdo não uniforme.
Expectativas verificam o estado real do LVGL; não fazem apenas comparação de pixels.
O processo deve encerrar normalmente para a execução ser aprovada.

## Compilação local Linux

Com as dependências do Dockerfile instaladas:

```sh
cmake -S . -B build/ui-sim -DBUILD_UI_SIM=ON -DBUILD_TESTS=OFF
cmake --build build/ui-sim --target aurora-ui-sim
python3 tools/ui-sim/run.py --binary build/ui-sim/tools/ui-sim/aurora-ui-sim \
  --output /tmp/aurora-ui-run
```

O diretório de saída deve estar vazio. O runner requer Python, Pillow, Xvfb e
xauth. O executável não abre servidor HTTP nem porta de controle: o driver usa
stdin/stdout. `BUILD_UI_SIM` é OFF por padrão e é recusado em builds webOS;
os comandos do simulador não são incluídos no IPK.

## Limites e defeito encontrado

O ambiente valida navegação, foco, painéis, diálogos e seu ciclo de vida. Não
valida o decoder NDL/Starfish, AV1/HDR, sincronismo de vídeo/áudio, permissões do
jail nem transmissão e encerramento contra um VibeShine real. Também não há
seletores DOM do Playwright: a interface nativa é controlada pelo adaptador SDL.

O roteiro inicial passou pelas ações, mas travou ao encerrar. O mDNS havia
falhado por falta de interface de rede; sua thread liberava `discovery_task_t`,
mas `discovery->task` continuava apontando para a memória liberada. A parada
acessava o mutex dessa estrutura. A versão anterior reproduziu
`AddressSanitizer: heap-use-after-free` em `discovery_worker_stop`.

A correção mantém as tarefas sob propriedade do gerenciador até a thread ser
recolhida. A parada normal permanece assíncrona, e a destruição final aguarda as
threads antes de liberar as dependências de callbacks. O teste
`test_discovery_lifecycle` força saída antecipada, parada repetida, reinício e
finalização com callback em andamento. Esse defeito foi reproduzido no ambiente
de testes; não estabelece a causa da falha anteriormente observada na C2.
