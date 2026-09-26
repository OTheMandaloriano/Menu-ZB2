# Piscadas simultâneas do menu e ESP — 26/09/2026

## Causa identificada

`hkPresent` e `GameUpdate` disputavam a mesma trava exclusiva. `GameUpdate` segurava a trava durante todo o ciclo (reflexão, raycasts, coleta e modificadores). Ao perder a disputa, `hkPresent` chamava o Present original sem desenhar nada. Menu, watermark, debug e ESP desapareciam juntos naquele quadro. Não era explicado por falta de inimigos nem pelos limites de distância.

## Correção

Configurações editadas pelo renderizador são publicadas junto com o tamanho da tela em um snapshot de três buffers. O callback Unity copia o último snapshot no início do ciclo e usa somente essa cópia. Nenhuma chamada Unity/Mono segura a trava da interface. O produtor é Present; o consumidor é Unity Update. A publicação acontece após editar o menu e antes de iniciar o bootstrap.

A trava `RuntimeGate` agora protege somente chamadas da interface (Present/ResizeBuffers/WndProc). Não foram introduzidas esperas pelo jogo nem acessos Unity no renderizador. O tamanho de viewport também foi migrado, para evitar uma corrida durante redimensionamento.

Os snapshots de entidades continuam indo no sentido oposto, de Unity para Present, com propriedade dos buffers. Os testes conferem que a cópia de configurações do ciclo não muda durante a edição de sliders e checkboxes.

## Validação e limites

- 10.000 quadros simulados com a atualização Unity deliberadamente ocupada: nenhum quadro perde a trava para o jogo; última configuração chega ao próximo ciclo sem alterar a cópia em uso.
- Verificação de fronteira: código do jogo não pode acessar `Config::` nem `RuntimeGate::`; todas as opções usadas precisam constar da captura.
- Regressões de snapshots, renderização ImGui, relógio, alcance e modificadores.
- Compilação Release x64. Validação visual em partida continua pendente; o usuário controla a tela.

As imagens também exibem muitos rótulos distantes sobrepostos. Isso é densidade visual, distinta da falha de quadros, e não foi disfarçado removendo inimigos ou reduzindo silenciosamente a distância. Transições reais de cena e dados inválidos ainda limpam o ESP por segurança.
