# ZB2 Menu Injector (v0.3.0)

Auto-inject x64 sem interface: watcher com polling 1s, LoadLibrary remoto,
logs coloridos com timestamp, config externa.

## Uso

1. Rode `injector.exe` **como administrador** (antes ou depois de abrir o jogo).
2. Ele espera o `ZumbiBlocks2.exe` aparecer (ate `timeout`), injeta sozinho,
   mostra o HMODULE remoto e pausa no final.
3. No jogo, pressione **INSERT** para abrir o menu.

## `config.ini` (ao lado do exe, opcional)

```ini
process=ZumbiBlocks2.exe
dll=Deadblock.Menu.dll
timeout=120
retry=3
retry_delay=5
bootstrap_delay=3
```

Sem `config.ini`, usa esses mesmos defaults. `dll` aceita caminho literal
ou so o nome do arquivo (procura ao lado do exe, `build\Release_x64`, dev).

## Flags

- `--nowait` (ou `-n`): pula a pausa final (para scripts).
- `--help` (ou `-h`, `/?`): mostra ajuda e sai.

## Cores do log

| Cor | Nivel |
|-----|-------|
| Cinza | hora |
| Ciano | INFO |
| Verde | OK / sucesso |
| Amarelo | AVISO (sem admin, sem SeDebug) |
| Vermelho | ERRO + `GLE=` (GetLastError) |

Saida redirecionada para arquivo sai sem cor (deteccao de console).

## Codigos de saida

- `0` sucesso (HMODULE remoto valido)
- `2` DLL nao encontrada | `3` DLL nao-x64 | `4` processo nao apareceu
- `5` alvo nao-x64 | `1` falha na injecao

## Notas

- Metodo: LoadLibrary remoto (estavel para jogo sem anti-cheat de kernel).
- `Documents\ZB2 Menu\injector_state.txt` registra size+mtime da DLL
  (detecta "DLL ATUALIZADA" entre builds).
- Evolucao prevista: `manualmap.cpp` (`--method=manual`) — ver HISTORICO
  no topo de `injector.cpp`.
