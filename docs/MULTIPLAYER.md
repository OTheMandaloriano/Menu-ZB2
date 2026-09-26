# Modo multiplayer

O cliente e o host não têm a mesma autoridade no jogo. O menu identifica o modo por `MultiplayerController`: `SINGLE`, `HOST` ou `CLIENTE`.

| Função | Cliente | Motivo |
|---|---|---|
| ESP, FOV e 360 | Local | Só desenham e selecionam informações no cliente. |
| Aimbot visível | Local | Move a câmera do jogador local. |
| Silent Aim | Solo apenas | Host e cliente enviam `SyncShotOnline`/`BroadcastShot`; nesses modos a opção usa mira visível em vez de redirecionar o vetor sincronizado. |
| Auto Fire e Triggerbot | Entrada local | Solicitam o fluxo normal da arma; a aceitação do tiro e do dano continua no servidor. |
| Movimento, cadência, vida, itens, dinheiro e slots | Não garantido no cliente | O servidor pode validar ou sobrescrever o estado. O menu não tenta contornar essa autoridade. |

Quando Silent Aim estiver marcado em uma configuração salva e o modo for `CLIENTE` ou `HOST`, o redirecionamento é desligado e o aimbot usa a mira visível. Isso evita que a opção faça o aimbot parecer inativo e preserva a direção sincronizada com o servidor.

Evidência: `PlayerPositionSynchronizer.SyncShotOnline` chama `ClientController.instance.GetSpeaker.SyncShot(..., shotVector)` no cliente e `ServerController.instance.GetSpeaker.BroadcastShot(..., shotVector)` no host. Animações e rolagem seguem a mesma separação de cliente/servidor.
