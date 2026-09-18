---
name: zb2-ce-mcp
description: Use ONLY when you need to read or change a live value in the running ZB2 game (HP, ammo, stackCount, offsets) via Cheat Engine MCP bridge. Self-heals the bridge automatically (restart CE + reload Lua without clicks). Never asks the operator to click CE GUI.
---

# CE MCP autonomo (sem clique em GUI)

## Regra de ouro

Nunca peca ao operador para clicar no Cheat Engine. O agente faz tudo:
detecta, reinicia, recarrega e valida sozinho.

## 1. Detectar o bridge

```powershell
# pipe existe?
py -c "import ctypes; k32=ctypes.windll.kernel32; h=k32.CreateFileW('\\\\\\\\.\\\\pipe\\\\CE_MCP_Bridge_v99',0x80000000,0,None,3,0,None); print('ABERTO' if int(h)!=-1 else 'ausente')"
```

Ou `cheatengine_ping`. Se responder, pula p/ secao 3.

## 2. Auto-reparo (nessa ordem, para no 1o que funcionar)

1. **Recarrega o Lua via linha de comando** (CE ja aberto, sem clique):
   o CE aceita `-lua:arquivo`? Nao em instancia aberta. Entao:
2. **Mata e reabre o CE com script de arranque**: cria
   `Temp/opencode/ce_reload.lua` com
   `dofile([[C:\Program Files\Cheat Engine\MCP_Server\ce_mcp_bridge.lua]])`
   e abre `cheatengine-x86_64.exe` — o autorun
   (`autorun/ce_mcp_bridge_autorun.lua`) carrega o bridge sozinho
   no arranque. Aguarda 20s e testa o pipe de novo.
3. **Se o pipe subir mas travar em chamada pesada**
   (`mono_image_enumClasses` em imagem grande = timeout conhecido):
   nao chama de novo; usa `mono_class_from_name` direto
   (resolve 1 classe, sem enumerar tudo).
4. **Fallback final**: desiste do CE e usa `zb2-dnspy-auto`
   (estatico) + offsets via `mono_field_get_offset` na nossa DLL
   (runtime, dentro do processo — log `[AMMO]`/`[MONEY]`).

## 3. Uso seguro (nao travar o bridge de novo)

- Sempre `LaunchMonoDataCollector()` 1x por sessao antes.
- NUNCA `mono_image_enumClasses` em mscorlib/CSharp inteira.
- Prefere: `mono_class_from_name(assembly, namespace, classe)`,
  `mono_class_get_field_from_name`, `mono_field_get_offset`,
  leitura/escrita direta de endereco.
- 1 chamada por vez; timeout = bridge morto -> volta p/ secao 2.

## 4. Roteiro padrao (validar offset novo)

1. `open_process(ZumbiBlocks2)`.
2. Resolve classe -> campo -> offset via API Mono.
3. Le o valor atual (confere com o HUD do jogo).
4. Escreve valor de teste BAIXO (ex. ammo-1, nunca 99999 de 1a).
5. Confere HUD; so entao automatiza na DLL.

## Proibido

- Pedir clique no CE (Ctrl+Alt+L, F9, Table, nada).
- `enumClasses` em imagem inteira (mata o pipe; 16/09 e 17/09).
- Escrever 99999 de primeira em valor desconhecido.
- Insistir >2 ciclos de reparo (cai p/ fallback dnlib+DLL).
