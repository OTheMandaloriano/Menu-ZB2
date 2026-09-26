# Contrato das funções de mira — v2

> Revisão posterior: as descrições abaixo de Silent limitado a solo e do Prefix em PhysicalGun foram substituídas por [SILENT_SYNC.md](SILENT_SYNC.md). O adaptador atual mantém um único ShotPath no tiro e na sincronização, habilitado nos três modos, ainda sujeito à validação em partida.

Esta revisão substitui as descrições de Silent/360/Trigger do relatório anterior. A interface descreve comportamento implementado; testes simulados não equivalem à validação em partida.

| Opção | Comportamento |
|---|---|
| Aimbot | Move a câmera para o alvo elegível enquanto a tecla Hold/Toggle está ativa. |
| Auto Aim | Mantém a ativação da mira sem segurar a tecla. Não dispara sozinho. |
| Silent Aim (solo) | Redireciona o argumento ShotPath no disparo da arma, sem alterar a orientação da câmera. Requer tecla ativa, Auto Aim ou Auto Fire. |
| FOV Radius | Círculo em pixels para selecionar inimigos projetados à frente. Zero ou Limit Radius desligado remove o limite circular, mas não habilita alvos atrás. |
| 360 Mode | Considera candidatos em todas as direções 3D, incluindo atrás da câmera, até Aim Distance. Ignora o círculo FOV. |
| Silent + 360 | Seleciona inimigo ao redor e direciona o tiro sem virar a câmera. Um alvo por disparo; não dispara automaticamente em todos os inimigos. |
| Aim Distance | Alcance em metros para seleção e consultas de disparo, independente da distância do ESP. |
| Aim Priority | Crosshair: pixels no FOV ou ângulo 3D em 360. LowestHP e Nearest usam vida e distância. |
| Aim Bone | Osso escolhido; ausência de osso válido elimina o candidato. |
| Smoothing | Suaviza apenas o movimento visível da câmera. Não interfere no Silent. |
| Auto Fire | Solicita disparo pelo fluxo nativo da arma. Sem Silent exige inimigo sob o raio central; com Silent exige caminho livre até o alvo selecionado. |
| Triggerbot | Solicita disparo quando o primeiro collider não pertencente ao jogador, no raio central, pertence a um inimigo vivo. Isoladamente não move a câmera. |
| Draw Radius | Exibe o círculo FOV. Fica oculto em 360 porque os alvos podem estar fora da tela. |

Obstáculos, alvo morto, foco perdido, menu aberto e pedidos expirados não autorizam ações automáticas. O Silent revalida visibilidade a partir da câmera e das origens do tiro. Spread, munição, cooldown, pellets, recuo e efeitos continuam no código original da arma. Sem redirecionamento válido, o disparo manual original é preservado.

Silent está limitado ao modo solo e armas do fluxo PhysicalGun.Shoot; lançadores usam outro fluxo. A sincronização cooperativa do Silent não está implementada. Prediction e Lag Compensation continuam sem execução; não foram apresentados como novos recursos funcionais.

## Implementação e dependências

- `aim_logic.h`: projeção versus ângulo 3D, seleção limitada a 128 candidatos, ativação e ordenação.
- `aim_runtime.inl`: coleta e aplicação da mira. Consulta até três candidatos por ciclo, rotacionando a busca após bloqueios.
- `aim_bridge.inl`: carrega o adaptador Mono e publica pedidos com referências gerenciadas.
- `managed/AimBridge.cs`: Harmony Prefix em PhysicalGun.Shoot e Postfix em PlayerArms.ReadFireInput. Os callbacks do disparo executam na thread do jogo e não chamam código nativo da DLL do menu. Pedido expira após 150 ms. Buffers de raycast são locais por thread, sem alocação por consulta após inicialização; lotação total bloqueia a ação.
- Harmony 2.3.3 obtido do NuGet oficial, com SHA-256 fixo e licença MIT distribuída junto. Não se modifica Assembly-CSharp.dll no disco.

Arquivos necessários lado a lado com o injetor: `kiero-dx11-base.dll`, `Zb2.AimBridge.dll` e `0Harmony.dll`. Copiar apenas a DLL nativa deixa o adaptador indisponível; nesse caso há log e as funções dependentes ficam suspensas.

## Build e testes

1. Compilar o projeto C++ Release x64.
2. `python scripts/build_managed_aim.py --managed "CAMINHO/ZumbiBlocks2_Data/Managed"`. O script verifica o hash do jogo, baixa/valida Harmony quando necessário e gera as dependências em build/Release_x64.
3. `python tests/run_aim_validation.py`.
4. `python tests/run_managed_aim_validation.py`.

Resultados desta revisão: 33 verificações C++ e teste concorrente de snapshots; 22 verificações C#/Harmony real com doubles de Unity/jogo. Os testes comprovam patch e preservação do método original, redirecionamento sem rotação de câmera, paredes, ordem dos hits, saturação, exceções, morte, jogador diferente, foco, expiração, cooldown, munição e retirada dos patches. Build nativo e adaptador gerenciado passaram. Validação da integração no Mono/Unity em partida ainda pendente.

## Fontes verificadas

- [Unity Vector3.Angle](https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Vector3.Angle.html): comparação angular entre direções 3D.
- [Unity Physics.Raycast](https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Physics.Raycast.html): direção, alcance, máscara e tratamento de triggers.
- [Harmony Prefix](https://harmony.pardeike.net/v2/articles/patching-prefix.html): alterar argumentos mantendo o método original.
- [Harmony argument injection](https://harmony.pardeike.net/v2/articles/patching-injections.html): parâmetros por índice e referência.
- [Mono embedding](https://www.mono-project.com/docs/advanced/embedding/): carregamento, invocação e thread attach.
- DnSpyMCP local: ShotPath, PlayerArms.ShootGun/ReadFireInput, PhysicalGun.Shoot, ZombieObject.GetZombie. Hash Assembly-CSharp: `c41a298975d35f0dad0a05531bce6e0b6e274d0ddf265217d65ce3ac5cbc84e1`.

Não existe uma especificação pública única de “grandes menus do mercado”. Os nomes foram confrontados com o comportamento solicitado e com as APIs oficiais, sem alegar equivalência com produtos proprietários não auditados.
