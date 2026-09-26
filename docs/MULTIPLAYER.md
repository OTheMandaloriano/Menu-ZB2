# Modo multiplayer

O cliente e o host não têm a mesma autoridade no jogo. O menu identifica o modo por `MultiplayerController`: `SINGLE`, `HOST` ou `CLIENTE`.

| Função | Cliente | Motivo |
|---|---|---|
| ESP, FOV e 360 | Local | Só desenham e selecionam informações no cliente. |
| Aimbot visível | Local | Move a câmera do jogador local. |
| Silent Aim | Solo apenas | O jogo cliente chama `SyncShotOnline` e envia a direção final ao servidor. O menu não altera esse protocolo no modo cliente. |
| Auto Fire e Triggerbot | Entrada local | Solicitam o fluxo normal da arma; a aceitação do tiro e do dano continua no servidor. |
| Movimento, cadência, vida, itens, dinheiro e slots | Não garantido no cliente | O servidor pode validar ou sobrescrever o estado. O menu não tenta contornar essa autoridade. |

Quando Silent Aim estiver marcado em uma configuração salva e o modo for `CLIENTE`, ele é ignorado na execução e o aimbot usa a mira visível. Isso evita que a opção faça o aimbot parecer inativo.

Evidência: `PlayerPositionSynchronizer.SyncShotOnline` chama `ClientController.instance.GetSpeaker.SyncShot(..., shotVector)` quando `IsClient()` é verdadeiro. Animações e rolagem seguem a mesma separação de cliente/servidor.
