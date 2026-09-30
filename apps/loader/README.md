# Loader nativo

Implementação offline, Windows x64, 400 × 384 unidades lógicas. A revisão visual
segue a referência Activation / WARDOGS escolhida pelo usuário após a versão 1.0,
com título centralizado e controles neutros. Lexend e
FontAwesome 6 Free ficam embutidos no executável. As atribuições são fornecidas
em AVISOS.txt no pacote de entrega, sem ocupar a página de informações do menu.

| Arquivo | Responsabilidade |
|---|---|
| main.cpp | Entrada e exclusão de instâncias concorrentes |
| window.cpp/h | Janela, mensagens, arraste nativo e DPI |
| apps/shared/graphics.cpp/h | Recursos D3D11 e apresentação, com RAII |
| ui.cpp/h | Desenho e publicação de ações; não executa operações de processo |
| apps/shared/theme.cpp/h e widgets.cpp/h | Tokens, fontes, escala, alinhamento e transições compartilhados |
| controller.cpp/h | Worker proprietário, fila de ações, snapshots sincronizados |
| license.cpp/h | SHA-256 e ECDSA P-256 via Windows CNG, claims e identificação |
| services.cpp/h | DPAPI, pacote assinado, instalação, processo e helper |
| assets/ | Fontes, headers, licenças e hashes de origem |

O desenho não executa criptografia, enumeração de processos ou instalação.
O worker é encerrado com join; não há thread detached. Se fechar durante uma
chamada do helper, o processo do loader pode levar até 45 segundos para terminar,
sem interromper a operação em andamento nem encerrar o jogo.

O estado contém verificação, ativação, espera, pronto, carregamento, sucesso e
erro. O checkbox de mapa é uma confirmação do usuário, não detecção automática.
Ele é desmarcado ao mudar o PID. Sucesso exige confirmação do módulo no processo;
isso não comprova que todas as funções internas do menu inicializaram corretamente.

Build, emissão, instalação, testes e limites: [docs/LOADER.md](../../docs/LOADER.md).

O emissor gráfico da equipe é o aplicativo separado [ZB2 Admin](../admin/README.md).
O loader aceita importar `.zb2license` e disponibiliza Copiar ID na própria ativação.
A página Sobre mostra os 64 caracteres do ID, versão, estado do jogo e validade.
A ajuda faz parte da própria interface ImGui.
