# Raios independentes do ESP

O slider de itens e o slider de pontos fixos usavam ambos `fItemRadius`. Corrigido com `fPoiRadius`, independente na interface, no JSON de presets, no snapshot de configurações e nos argumentos do coletor gerenciado.

Itens continuam usando `fItemRadius`. Loot fixo, bancadas, fogueiras e mercadores usam `fPoiRadius`. Pontos dinâmicos mantêm o comportamento anterior sem esse limite; o controle está identificado como **Raio pontos fixos**. Ambos começam em 150 m. Presets antigos sem a nova chave mantêm o valor atual dos pontos, sem copiar o valor de itens. AIM e ESP de zumbis não foram alterados.

O renderizador também filtra snapshots antigos ao reduzir cada limite. Testes verificam os dois sentidos da independência, a gravação/leitura real do preset e o desenho de pontos com raio de itens curto. Validação ao vivo ainda necessária.
