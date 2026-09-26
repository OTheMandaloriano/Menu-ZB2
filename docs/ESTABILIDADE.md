# Auditoria de crashes e travamentos — 25/09/2026

## Evidências preservadas

Logs e dumps foram copiados para `D:\ZB2-Retomada\crash-audit` antes de reiniciar qualquer processo. Nenhuma interação com a tela foi utilizada nesta revisão.

| Ocorrência | Evidência | Conclusão permitida |
|---|---|---|
| 01:58 | Minidump: 0xc0000005 em 0x7ffb2b303634; topo da stack em kiero-dx11-base | Falha dentro da DLL antiga. Sem PDB correspondente não é possível atribuir uma função com segurança. |
| 16:00 | Minidump: 0xc0000005 em 0x7ffabf4154dc, UnityPlayer | Falha nativa no Unity; stack isolada não prova o responsável pela corrupção. |
| 20:03 | Minidump: 0xc0000005; SetActive → LODCollider.SetColliding → LODController.UpdatePhysics | Falha durante atualização de colisores/LOD. Compatível com disputa de acesso, mas não prova causalidade exclusiva do menu. |
| 21:13 e 21:20 | Eventos Windows 1001 AppHangB1 e 1002 | Travamentos confirmados. Não há dump de hang correspondente preservado para demonstrar a cadeia de espera. |
| Sessão PID 29512 | Log do menu: redirected=32, blocked=3; worker continua registrando AMMO até 21:20:14 | Silent executou antes do travamento. Funcionamento parcial não demonstra estabilidade. A worker ainda estar ativa não identifica qual thread travou. |

## Defeitos estruturais encontrados e tratados

1. **APIs Unity na worker.** Transform, Camera, Renderer, Physics e funcionalidades do jogo eram invocados em paralelo ao Update. `mono_thread_attach` registra a thread no runtime; não torna objetos Unity seguros para acesso concorrente. A referência local anterior que recomendava worker para todas essas chamadas estava incompleta.
2. **Mono inicializado dentro de Present.** A thread de renderização era anexada ao Mono e executava resolução. Agora Present só dispara a inicialização auxiliar; coleta e alterações de objetos rodam em callback do Update do jogo.
3. **Estado global compartilhado.** Configuração/viewport/GUI e o ciclo de jogo usavam variáveis comuns. Um gate exclusivo com tentativa sem espera serializa esses acessos. Em disputa, o trabalho daquele ciclo/frame é pulado; nenhum dos dois espera pelo outro. Telemetria é publicada como snapshot.
4. **Esperas durante transições.** O ciclo tinha Sleep de até dois segundos e podia reutilizar o jogador obtido antes da espera. Agora a transição invalida dados, agenda nova tentativa e retorna sem esperar. Cena inválida e mapa instável suspendem o ciclo antes de funcionalidades.
5. **Caches de restauração entre cenas.** Ponteiros de restauração permaneciam após troca de jogador/cena. Agora são descartados nessas transições. Desligar a última opção continua processando restaurações pendentes.
6. **Loader lock e descarregamento.** DllMain fazia logging, consultas de pastas e destruição de recursos. Inicialização de log foi movida para fora dele. No detach, apenas sinaliza parada. Quando o callback gerenciado é registrado, a DLL é fixada até o processo terminar; não se deve substituir/descarregar a DLL em um jogo aberto.
7. **Estado D3D11.** Os render targets e depth stencil anteriores são restaurados após desenhar o menu.
8. **Diagnóstico insuficiente.** Release agora gera PDB e MAP. Há heartbeat `[RUNTIME] Unity Update tid=...`, tempo do ciclo e aviso de ciclo lento. O spam por transição de cada entidade foi removido.

## Arquitetura resultante

Thread auxiliar: bind/resolução Mono e instalação do callback, seguida de detach. Ela não coleta entidades nem invoca Transform/Physics durante o jogo.

Postfix gerenciado de `ZBMain.Update`: chama o ciclo nativo aproximadamente a cada 33 ms, sem Sleep; snapshot visual e telemetria são publicados ao terminar. A coleta verifica orçamento de seis milissegundos entre entidades. Esse orçamento não interrompe uma chamada individual que já começou e não é garantia rígida de duração total.

Present: ImGui e leitura de snapshots. WndProc: eventos de interface. Ambos usam tentativa de acesso ao estado; liberam o gate antes de encaminhar Present/WndProc originais.

O adaptador `Zb2.AimBridge.dll` e `0Harmony.dll` agora também são necessários ao ciclo principal. Falha na instalação suspende as funcionalidades, sem retornar à worker insegura.

## Verificação e limites

- Build nativo Release x64 e build do adaptador contra as DLLs exatas do jogo.
- 33 verificações determinísticas de mira.
- Teste concorrente com 100 mil publicações de snapshot.
- Teste de contenção do gate: a thread concorrente retorna sem esperar; saída de escopo libera acesso.
- 26 verificações com Harmony real e doubles de Unity/jogo, incluindo callback na thread de Update, pausa e remoção do callback.

Esses testes não certificam estabilidade em uma partida real. Ainda é necessário teste prolongado com horda, proximidade, trocas de arma, morte, respawn e Alt+Tab. Novos dumps devem ser analisados com o PDB da mesma build.

## Riscos ainda pendentes

- Subsistemas antigos de munição/restauração ainda mantêm ponteiros nativos para objetos gerenciados. A limpeza de cena reduz riscos, mas não substitui uma migração completa para referências gerenciadas/GC handles e identidade de objetos.
- Logging síncrono e algumas rotinas longas continuam podendo aumentar a duração de um Update. O orçamento entre entidades não cobre todas as funcionalidades.
- O tratamento completo de troca de device/swapchain e falhas parciais de instalação de hooks continua merecendo testes específicos.
- Não foi possível provar a cadeia exata de espera do hang de 21:20: o processo já estava fechado e não havia dump de hang.

Não atribuir todos os incidentes a uma causa única nem marcar esta revisão como livre de crashes antes da validação em partida.

Fontes: [Unity — thread safety](https://docs.unity.com/en-us/engine/6000.3/manual/scripting/optimization/programming-best-practices), [Mono embedding](https://www.mono-project.com/docs/advanced/embedding/), dumps locais e eventos Windows preservados.
