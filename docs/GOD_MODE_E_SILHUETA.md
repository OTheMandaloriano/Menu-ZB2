# Revisão GOD-SIL-2

## God Mode

A implementação anterior restaurava HP para 100 em ciclos separados do dano.
Uma queda fatal podia mudar o jogador para Dying antes desse reparo, e o valor
fixo ignorava perks. O dnSpy confirmou que `PlayerMovement.GetGround` cria
`DamageType.Fall`, encaminhado a `PlayerMain.TakeDamage`.

Agora um prefixo Harmony em TakeDamage impede o desconto apenas no jogador
local protegido. Não intercepta jogadores remotos. O adaptador repõe os dois
reservatórios usando `PlayerMain.MaxHealth`, sem constante 100. Desligar remove
a propriedade de proteção; a rotina de restauração recebe mais um ciclo mesmo
com os outros controles desligados. Um watchdog libera a proteção se o runtime
deixar de responder. Ativar não ressuscita quem já estava caído ou morto.

Testes Harmony: dano fatal de 10000, dano comum, perks 133,3/175, desativação,
troca de jogador, objeto remoto, runtime parado e máximo inválido. Não se
declara validada toda possível causa de morte; foi auditado o caminho de dano
normal e de queda descrito acima.

## Silhueta

A captura revelou NotSupportedException, mas a mensagem antiga descartava o
motivo. O adaptador agora preserva motivo e tipo de exceção para diagnóstico.
Na ponte nativa, texturas RGBA/BGRA typeless recebem uma view explicitamente
UNORM; a criação de view implícita era incompatível com esse formato. O teste
WARP passou a usar uma textura typeless e confirmou os pixels do contorno.
A confirmação da causa específica na máquina depende da nova execução.

## Ferramentas e implantação

Foram usados o servidor dnSpy MCP local e o CE MCP em turnos desta auditoria;
o CE confirmou previamente 133,3 HP no jogador local e os valores padrão dos
aliados. Nesta revisão o código de dano/queda foi inspecionado via dnSpy.
Nenhum dano fatal foi provocado automaticamente na partida para testar.

A skill de segurança Mono passou a registrar a autorização para atualizar:
fechar o jogo, verificar encerramento, copiar binários com hashes, reabrir e
aguardar entrada no mapa para injetar. Sem controle de tela nem reinjeção de
DLL já carregada. Fontes, testes e documentação são versionados; saídas de
compilação e diagnósticos ficam fora do commit. Não se afirma uma limpeza
integral das pastas históricas sem auditá-las.
