# Modo multiplayer

O cliente e o host não têm a mesma autoridade no jogo. O menu identifica o modo por `MultiplayerController`: `SINGLE`, `HOST` ou `CLIENTE`.

| Função | Cliente | Motivo |
|---|---|---|
| ESP, FOV e 360 | Local | Só desenham e selecionam informações no cliente. |
| Aimbot visível | Local | Move a câmera do jogador local. |
| Silent Aim | Adaptador implementado; validação em partida pendente | O mesmo ShotPath é consumido pelo tiro local e por SyncShotOnline/BroadcastShot. Habilitado em solo, host e cliente. |
| Auto Fire e Triggerbot | Entrada local | Solicitam o fluxo normal da arma; a aceitação do tiro e do dano continua no servidor. |
| Movimento, cadência, vida, itens, dinheiro e slots | Não garantido no cliente | O servidor pode validar ou sobrescrever o estado. O menu não tenta contornar essa autoridade. |

Silent habilitado mantém a câmera parada nos três modos. O adaptador modifica a variável local de ShootGun por referência e preserva o envio original do jogo; não cria disparos ou pacotes adicionais. FOV e 360 respeitam alcance e obstáculos. Lançadores não são cobertos. Ver [SILENT_SYNC.md](SILENT_SYNC.md) para evidências, testes e limites.

Evidência: `PlayerPositionSynchronizer.SyncShotOnline` chama `ClientController.instance.GetSpeaker.SyncShot(..., shotVector)` no cliente e `ServerController.instance.GetSpeaker.BroadcastShot(..., shotVector)` no host. Animações e rolagem seguem a mesma separação de cliente/servidor.
