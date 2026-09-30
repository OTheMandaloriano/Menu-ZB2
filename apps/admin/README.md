# ZB2 Admin

Aplicativo administrativo C++ / Dear ImGui / D3D11, separado do loader e com a
mesma base visual compartilhada. Não utiliza WinForms para telas ou mensagens.
Um serviço .NET Framework sem janela usa Windows CNG para ECDSA P-256 e DPAPI
para a chave privada de cada estação.
A chave principal é importada somente no PC/usuário do proprietário. Integrantes
geram chaves próprias e recebem autorizações assinadas com prazo e limite.

- `Core.cs`: chaves, assinaturas, estações, autorizações e histórico atômico local.
- `Backend.cs`: serviço sem janelas, protocolo limitado por pipes privados.
- `native/ui.*`: campos, tabelas, ajuda e ações de interface em ImGui.
- `native/backend.*`: extração verificada do serviço e processo sem console.
- `native/controller.*`: worker proprietário e snapshots sincronizados.
- `native/model.*`: protocolo público e modelos de tela; não transporta chaves privadas.
- `native/main.cpp`: janela sem moldura, DPI, D3D11 e escolha de arquivos.
- `apps/shared/*`: fontes, widgets, recursos e gráficos compartilhados com o cliente.
- `scripts/build_admin.py`: executável único com o serviço e fontes embutidos.
- `tests/AdminCoreTests.cs`: permissões, persistência, assinatura e adulteração.
- `tests/admin_native_tests.cpp`: renderização e eventos sintéticos do ImGui.
- `tests/test_admin_bridge.py`: emissão e permissões pelos pipes reais, incluindo
  preservação do arquivo de estação.
- `tests/run_admin_validation.py`: regras C# e verificação cruzada C++.

As telas não exigem comandos do usuário. Não há chave privada, credencial de
servidor ou histórico de clientes embutido no programa. Não distribua a pasta
de dados de uma estação junto com o executável.

O serviço é extraído em `Documentos/ZB2Menu/Admin/runtime/<hash>`, conferido por
SHA-256 e iniciado sem console. O canal herda somente os handles necessários.
Requests e respostas têm limites; ações de emissão não são repetidas automaticamente.
Uma falha de comunicação exige conferir o histórico antes de uma nova tentativa.

As atribuições das dependências estão embutidas em Créditos, sem arquivo AVISOS.txt. `scripts/package_delivery.py` produz CLIENTE.zip e EQUIPE.zip por allowlist.

Operação: [guia da equipe](../../docs/ADMIN.md).

Organização de pastas e ícones: [PASTAS.md](../../docs/PASTAS.md).
