# Desenvolvimento

## Preparar o ambiente

- Windows x64, Visual Studio com C++ v145 e Windows SDK.
- Python 3.12 ou superior e pacote `cryptography` para o empacotamento assinado.
- CMake e Ninja para compilar o FreeType estático. Os scripts usam o CMake do Visual Studio quando necessário.
- Jogo compatível com o hash exigido por scripts/build_managed_aim.py.
- Chave de emissão autorizada e protegida por DPAPI para compilar uma entrega de produção.

Os clientes não precisam instalar Python, CMake, Ninja ou Visual Studio.
Fontes, dependências e recursos são incorporados durante a compilação.

## Caminhos

Os fontes C++ do menu ficam em src/menu. O projeto Visual Studio continua na raiz
e preserva os diretórios de saída build/Release_x64 e build/intermediates.
O código de apps/loader e apps/admin tem seus próprios scripts de build.
Não mova imgui ou kiero sem atualizar includes, scripts e testes que os referenciam.

## Compilar e verificar o menu

No terminal de desenvolvimento do Visual Studio:

```powershell
msbuild kiero-dx11-base.vcxproj /p:Configuration=Release /p:Platform=x64
python scripts/build_managed_aim.py --managed "CAMINHO_DO_JOGO/ZumbiBlocks2_Data/Managed"
python tests/check_repository_layout.py
python tests/check_runtime_ownership.py
python tests/run_aim_validation.py
```

O caminho do jogo precisa ser informado. Os testes gerenciados dependem dos arquivos
gerados pelo build gerenciado; consulte os scripts run_*_validation.py em tests.

## Gerar o cliente e o Admin

O script build_loader.py exige --runtime, --runtime-commit, --version, --key e --public.
Use --help para a sintaxe. O commit informado deve identificar o runtime real,
não um commit de documentação escolhido apenas por ser o HEAD.

1. Compilar o loader com a versão desejada e --tests.
2. Compilar `python scripts/build_admin.py --tests` para incorporar esse cliente.
3. Executar as validações de licença, UI, cache e pacotes com credenciais de teste.
4. Executar `python scripts/package_delivery.py` para produzir os dois ZIPs iniciais.

Nunca criar ou trocar a chave de produção para contornar uma falha de DPAPI.
Ela depende do usuário Windows correto e fica fora do Git.

## Saídas e limpeza

Não colocar objetos, imagens de teste, relatórios ou executáveis na raiz. Use build.
Somente os executáveis atuais e ZIPs iniciais pertencem a dist. Dados de uso ficam
em Documentos/ZB2Menu e seguem RETENCAO-E-LIMPEZA.md.
Compilar não autoriza trocar uma DLL em uma partida. Testar e instalar são etapas distintas.
