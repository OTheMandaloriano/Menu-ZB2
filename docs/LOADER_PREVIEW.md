# Loader: primeira etapa

O usuário aprovou retomar o loader em etapas. Esta entrega é uma prévia visual
nativa C++17/Win32/D3D11 com Dear ImGui já usado no projeto, sem copiar os loaders
baixados. Telas: Ativação, Início e Pacote. Não consulta o jogo, não injeta, não
acessa rede, não valida licenças nem instala/extrai arquivos. Botões dessas ações
permanecem desabilitados e identificados como pendentes. Não é a distribuição
operacional que substitui o injetor atual.

O executável incorpora um ZIP de desenvolvimento com seis arquivos permitidos.
O empacotador exclui PDB/MAP e outros arquivos por allowlist, usa timestamps fixos
e verifica tamanho/hash dos arquivos. Manifesto marca `unsigned-development`:
hash sozinho não comprova autenticidade, nem protege contra alteração coordenada
do conteúdo e do manifesto. Licenças e assinatura do pacote entram depois da
aprovação visual. Nenhuma chave privada/credencial foi criada ou incorporada.

## Compilar

```
python scripts/package_runtime.py --source build/Release_x64 --output build/distribution/runtime-preview.zip --version 0.1-preview --commit <hash-completo-da-build>
python scripts/build_loader_preview.py --bundle build/distribution/runtime-preview.zip
```

Saída: `build/loader-preview/ZB2Menu-Preview.exe`. O script exige MSVC e Windows
SDK já instalados. O commit passado ao empacotador é metadado informado pelo
operador; confirmar correspondência com a build antes de distribuir. Nesta etapa,
as DLLs incorporadas foram comparadas com o manifesto da UTILITIES-11.

O processo usa a fonte Segoe UI instalada no Windows, sem redistribuí-la.
O aplicativo não grava presets/logs próprios nem modifica o diretório do jogo.
Testes de DPI, teclado, resize e execução em outro PC ainda são necessários;
renderização offscreen não comprova toda a interação nativa.

## Testes

```
python tests/test_package_runtime.py
python tests/run_aim_validation.py --only loader_preview_tests
```

Oito testes de pacote cobrem conteúdo, determinismo, dependência ausente,
arquivo alterado, entrada extra/travessia e metadados explícitos de desenvolvimento.
As três telas são renderizadas sem controlar a área de trabalho. Os exemplos
externos não foram executados. O injetor e a DLL instalados continuam preservados.

## Próxima etapa

Aprovar o visual, depois integrar verificador offline e emissor separado, seguindo
o plano de licenciamento. Um executável de entrega não torna DLLs inextragíveis.
Não prometer expiração inviolável sem referência externa de tempo. Atualização
automática e serviço online permanecem fora desta primeira etapa.
