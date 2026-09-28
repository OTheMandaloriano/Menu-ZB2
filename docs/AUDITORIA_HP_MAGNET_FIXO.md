# Vida dos aliados e retenção do Magnet

## HP: pendência confirmada, não corrigida por um percentual artificial

A captura enviada mostra o fallback de estado, não uma barra numérica.
A revisão do assembly auditado confirmou:

- `NetCharacterMessage.PlayerHealthState`, MethodDef `0x0600167D`, escreve
  o identificador do jogador e um byte `HealthState`. Não envia HP atual/máximo.
- `PlayerMain.SetHealthState`, MethodDef `0x06000B7A`, atualiza o enum;
  sua sincronização opcional também transmite o enum.
- `PlayerMain.UpdateNonLocallyControlled`, MethodDef `0x06000B81`, atualiza
  armas, chão e posição recebida. Não processa a saúde local daquele jogador.
- `PlayerMain.MaxHealth`, MethodDef `0x06000B68`, retorna 100 para o jogador
  remoto, independentemente de seus perks. Não é uma fonte de máximo real.

A auditoria não encontrou uma fonte alternativa confiável de HP real do aliado.
`WorldEspBridge.Team` continua sinalizando indisponibilidade numérica, preservando
vivo/caído. Isso não atende à solicitação de barra/% real e permanece pendente.
Somente o PC do usuário terá o menu, conforme confirmado. Não se estimou saúde
por distância, dano observado, valor padrão ou um percentual constante.

## Magnet de amigos

Trocar o filtro de zumbis por nomes de amigos não adapta o protocolo:
`Zombie.TeleportTo` move uma entidade `Zombie`; jogadores usam `PlayerMain`
e a própria simulação de movimento. A seleção pode mudar o alvo da interface,
mas não concede ao chamador controle sobre o jogador local de outro cliente.
Não houve envio experimental de pacotes ou mudança na posição de participantes.
Essa função não foi implementada nesta revisão.

## Defeito corrigido: ponto fixo deixava inimigos saírem

Antes, o modo de mira salvava uma âncora, mas só suspendia os inimigos se o
checkbox opcional de congelamento estivesse marcado. Sem ele, a IA podia sair.
Agora os dois modos fixos retêm os atraídos automaticamente até desligar:

1. **Seguir minha posição:** mantém o comportamento anterior; congelar é opcional.
2. **Fixar no ponto da mira:** ao ativar, captura chão válido na mira uma vez.
3. **Fixar à minha frente:** captura chão perto do jogador ao ativar, dispensando
   apontar a câmera para o chão. Não acompanha mudanças posteriores de posição.

H alterna a função. Desligar libera e restaura física/animação; ativar de novo
captura outro ponto. A animação de nascimento termina antes de puxar/congelar.
Trocar de modo fixo para seguir libera o congelamento automático, preservando
a preferência independente do checkbox. Os inimigos permanecem distribuídos
em posições verificadas ao redor da âncora, não sobrepostos no mesmo collider.
Continuam as verificações de chão, obstáculo, raio, tipo de alvo e solo/host.

## Referência solicitada

O endereço `https://github.com/ioisaque/wohax/tree/master/neo` foi consultado,
mas seu conteúdo não pôde ser recuperado nesta revisão. A raiz do repositório
ficou acessível; não se presume o conteúdo de `neo` a partir dela. Esta correção
implementa o comportamento descrito pelo usuário e testes do ZB2, sem afirmar
ser um port fiel do código inacessível.

## Validação

32 verificações de Magnet passaram, incluindo retenção sem checkbox, desligar,
captura próxima, jogador se afastar, troca de modo, nascimento e chão ausente.
Os testes simulam o jogo e incluem os hooks Harmony existentes; a integração
com a partida ainda requer teste após recarregar a DLL.
