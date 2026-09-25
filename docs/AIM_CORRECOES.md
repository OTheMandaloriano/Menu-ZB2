# Mira: paredes, proximidade e independência do ESP

## Problemas corrigidos no código

- Physics era procurada no módulo de Transform, e qualquer falha autorizava mirar. Agora a classe é resolvida nas imagens carregadas; o overload é escolhido pelos seis tipos dos parâmetros. Falha suspende a mira e gera log `[AIM] visibilidade indisponivel` com limite de frequência.
- A consulta antiga avançava a origem 30 cm, impunha distância mínima de 1 m e estendia o raio além do alvo. A consulta atual parte da câmera e termina no osso escolhido. O primeiro collider é classificado pela hierarquia: collider do alvo permite a mira; obstáculo bloqueia. Triggers são ignorados; todas as camadas físicas são consultadas.
- O tamanho de RaycastHit é obtido por `mono_class_value_size`; não há offsets de campos nem interpretação manual de arrays de hits. O getter gerenciado resolve o collider.
- A coleta de mira não depende de ESP, Skeleton, tipo de caixa ou distância visual. Ela publica candidatos próprios na worker antes dos filtros de desenho. O movimento usa o snapshot coletado no mesmo ciclo.
- O fallback de projeção deixou de rejeitar tudo abaixo de 1 m. Os dois caminhos usam plano próximo de 5 cm. A caixa 2D permanece quando cruza a viewport mesmo que cabeça e pés estejam fora dela; caixas 3D parcialmente projetadas também permanecem.
- Top-3 percorre a lista completa. Sticky mantém a identidade por 500 ms, não renova o prazo a cada ciclo e não ignora distância/FOV. O candidato efetivamente escolhido é revalidado antes de mover a câmera.
- Auto Aim funciona com Aimbot ligado. Tecla não atribuída não ativa a mira. Menu aberto e janela sem foco suspendem ações. Triggerbot isolado não move a câmera. Disparo automático exige alinhamento medido e collider do alvo confirmado.
- Silent Aim foi identificado como não implementado e aparece indisponível, preservando a leitura de presets antigos. A antiga opção 360 passou a indicar corretamente que apenas desativa o limite do raio para alvos projetados à frente.
- O valor exibido do raio corresponde aos pixels reais; o formato salvo mantém compatibilidade.
- O snapshot visual usa três buffers com propriedade por thread. Um produtor não reutiliza o buffer enquanto o consumidor o lê. Cena inválida publica snapshot vazio.
- Resize anterior à inicialização da GUI apenas repassa a chamada. Repetição automática de INSERT/DELETE não alterna o menu várias vezes.

## Organização

- `aim_logic.h`: matemática, seleção, ativação e regras determinísticas.
- `aim_runtime.inl`: adaptador interno Mono incluído no namespace Mono; coleta, consultas de física e aplicação da câmera. Todas as chamadas são da worker.
- `latest_snapshot.h`: publicação single-producer/single-consumer; um único consumidor deve chamar Read.
- `tests/aim_tests.cpp` e `tests/snapshot_tests.cpp`: regressões sem jogo.

O restante de mono.cpp ainda contém funcionalidades antigas. Esta alteração não equivale a concluir toda a auditoria de ciclo de vida, telemetria e configurações globais.

## Evidência estática

Assembly-CSharp.dll SHA-256: `C41A298975D35F0DAD0A05531BCE6E0B6E274D0DDF265217D65CE3AC5CBC84E1`.

Consultado pelo DnSpyMCP instalado:

- PlayerCamera.AxialInput: rotaciona o transform e atualiza Angle, limitado a ±80 graus.
- PlayerCamera.SetupFPSCameraTransform: usa a rotação do jogador e aplica o pitch de Angle.
- PlayerArms.GetCameraBasedShotPath: o tiro usa CameraTransform.forward.
- ZombieObject: componente MonoBehaviour com capsuleCol e limbColliders.
- UnityEngine.PhysicsModule: RaycastHit possui getter collider; QueryTriggerInteraction.Ignore = 1.

Nenhum binário do jogo foi alterado e nenhum offset novo de objeto do jogo foi inferido de metadata.

## Validação

`python tests/run_aim_validation.py` usa g++ ou MSVC instalado, grava saídas em build/aim-validation e executa:

- 24 verificações determinísticas: candidato no final da lista, alvo próximo, morte, limites durante sticky, NaN, física desconhecida, ativação e interseção da viewport.
- 100 mil publicações concorrentes: integridade, sequência e limpeza de cena.

Build Release x64 realizado em cópia isolada. A suíte existente de layout passou com 75.409 cenários, 560 transições de arraste e 450.246 assertions; existem avisos preexistentes de conversão numérica nos testes. A GUI simulada completa não foi executada nesta rodada.

## Ainda requer partida

Durante a primeira verificação o jogo não estava em execução. Depois, pelo CE, foram confirmados uma partida com um jogador e 16 inimigos, a classe Physics e os seis tipos exatos do overload de Raycast. O acesso direto ao endereço estático confirmou as listas; o primeiro helper baseado no getter estático retornava zero incorretamente. A DLL antiga já estava carregada. A consulta completa da DLL nova e o movimento ainda precisam ser certificados após reiniciar o jogo.

Verificar uma casa com inimigo atrás da parede, passagem pela porta, outro inimigo visível, distância de 20–90 cm, ESP/Skeleton desligados, diferentes distâncias de ESP e mira, morte do alvo, troca de cena e Alt+Tab. Procurar `[AIM] Physics.Raycast(out RaycastHit) pronto` no log. Se aparecer diagnóstico de indisponibilidade, a mira ficará suspensa até a cadeia ser resolvida; não há fallback silencioso através de paredes.

Riscos pendentes: colliders com hierarquia atípica e câmera dentro de geometria precisam de teste real. Invocações Unity continuam na worker conforme arquitetura do projeto. Encerramento completo da DLL e sincronização geral de Config/State permanecem itens separados da auditoria.
