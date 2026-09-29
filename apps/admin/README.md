# ZB2 Admin

Aplicativo administrativo Windows Forms / .NET Framework, separado do loader.
Usa Windows CNG para ECDSA P-256 e DPAPI para a chave privada de cada estação.
A chave principal é importada somente no PC/usuário do proprietário. Integrantes
geram chaves próprias e recebem autorizações assinadas com prazo e limite.

- `Core.cs`: chaves, assinaturas, estações, autorizações e histórico atômico local.
- `Program.cs`: interface, seleção de prazo, emissão assíncrona, busca, exportação,
  configuração de estação e gerenciamento das autorizações.
- `scripts/build_admin.py`: executável único com fonte e licença embutidas.
- `tests/AdminCoreTests.cs`: permissões, persistência, assinatura e adulteração.
- `tests/AdminUiTests.cs`: eventos sintéticos dentro do formulário de teste.
- `tests/run_admin_validation.py`: testes C# e verificação cruzada C++.

As telas não exigem comandos do usuário. Não há chave privada, credencial de
servidor ou histórico de clientes embutido no programa. Não distribua a pasta
de dados de uma estação junto com o executável.

Operação: [guia da equipe](../../docs/ADMIN.md).
