# Catálogo visual, Chams, equipe e revisão do Magnet

## Busca e catálogo

A lista anterior mostrava identificadores de código, como `PumpShotgunOld`, e o tratamento de mensagens do Windows descartava entrada quando a trava de UI estava ocupada. Agora a fila de entrada é consumida no renderizador, sem chamar ImGui diretamente de outra thread. A busca normaliza caixa, espaços e acentos e aceita o nome traduzido ou o identificador interno.

O botão **Escolher itens** abre um modal com filtros e nomes obtidos de `ItemsBase`. Os ícones vêm do sprite do próprio item: cópia limitada a 48×48 pixels no callback Unity, transporte de pixels por snapshot e criação de textura D3D no renderizador. Não é feita chamada Unity no Present. Ícones são carregados sob demanda, no máximo um por 100 ms, e mantidos em cache. Quando o jogo não fornece um ícone válido, permanece um espaço identificado pelo nome; nenhuma imagem é inventada ou distribuída com os fontes.

## Equipe

Box 2D, box 3D, cantos, nome, distância, barra de vida, percentual, linha com origem própria, ponto na cabeça e esqueleto usam controles independentes dos zumbis. Cores, presets de posição e editor de arraste são específicos da equipe e persistem no JSON. Vida percentual usa `PlayerMain.MaxHealth`; ossos usam o Animator humano real. Se o modelo não fornecer esse rig, não se desenha esqueleto fictício.

## Chams e ondas

Chams adiciona um CommandBuffer à câmera: passes separados de profundidade para partes visíveis e ocultas, com cores próprias. Não modifica os materiais dos inimigos. Desligar ou trocar de câmera libera os recursos e remove os comandos. A implementação requer pipeline Built-in e shader `Hidden/Internal-Colored` com `_Color` e `_ZTest`; indisponibilidade aparece como diagnóstico. A compilação não comprova o resultado gráfico na GPU do jogo.

O antigo checkbox Onda não tinha coletor. Foi substituído por **Horda ativa**, que indica o centro dos zumbis de onda carregados e vivos. Isso não representa um ponto de spawn futuro nem inclui entidades que não existem na lista local.

## Magnet e análise da especificação recebida

O documento do usuário foi tratado como proposta funcional. Nesta revisão, o Magnet ganha destinos estáveis, distância frontal configurável (2,5 m padrão) e destino separado de bosses (8 m padrão). A mesma entidade não é teleportada novamente só porque a IA andou. Uma mudança significativa do ponto de reunião libera uma nova passagem. O carregamento de um boss é solicitado no máximo uma vez por identidade e ativação; isso evita repetir conversões de LOD. Há lotes, espaçamento e verificação de chão/ocupação. Não há criação de novos zumbis nem gancho de spawn nesta revisão.

Sugestão de evolução, em etapas verificáveis:

| Proposta | Estado / condição |
|---|---|
| Frente da mira, filtro de alvos, distância de bosses | Implementado, aguardando partida |
| Congelar atraídos e bloquear ataques especiais | Pendente; precisa suspender a execução e restaurar estado. **Zerar cooldown pode liberar o ataque**, não bloqueá-lo. |
| Horda seguir jogador e redirecionar spawn | Pendente; exige selecionar identidade estável e implementar no host, sem misturar com o Magnet frontal. |
| Escolta, trazer aliado, agrupar caídos | Pendente; exige auditar estados vivo/caído e autoridade do jogador. HP zero sozinho não basta para classificar todos os estados. |
| Item Magnet e coleta automática | Pendente; mover loot exige atualizar a célula e sincronizar; coleta deve validar espaço com o método real do inventário. |
| Modal de jogadores com avatar Steam | Pendente; primeiro auditar a API/cache existente. Foto, ping e SteamID não devem ser presumidos nem fabricados. |
| Hold/Toggle independente por módulo | Pendente para os novos módulos; os atalhos atuais continuam funcionando. |

O host continua necessário para mover zumbis com efeito na sala. A revisão não promete suporte autoritativo de Magnet por um cliente comum.

## Validação

Testes de normalização da busca, 20.000 mensagens preservadas na fila, coleta/projeção/ABI, controles e persistência de equipe, ciclo de vida de Chams, e Magnet sem reposicionamento repetido. Build nativa e gerenciada contra o jogo auditado. Ícones, Chams e física precisam de verificação em partida pelo usuário, sem controle de tela pelo agente.

Referências oficiais: [Renderer.bounds](https://docs.unity3d.com/kr/6000.0/ScriptReference/Renderer-bounds.html), [CommandBuffer.DrawRenderer](https://docs.unity3d.com/cn/current/ScriptReference/Rendering.CommandBuffer.DrawRenderer.html), [Texture2D.ReadPixels](https://docs.unity3d.com/kr/2021.3/ScriptReference/Texture2D.ReadPixels.html).
