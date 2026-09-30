# Loader offline — operação e limites

## Entrega ao cliente

No Admin, use **Clientes → Gerar ZIP do cliente**. O pacote personalizado
contém o executável e `licenca.zb2license`. O cliente extrai tudo e abre o
programa; a licença assinada é verificada para o PC dele e importada automaticamente.
Também é possível colar uma chave ou importar um arquivo manualmente.

O pacote inicial `dist/entrega/CLIENTE.zip` contém apenas o executável, para coletar
o ID antes de emitir. Integrantes emissores seguem o [guia simples](ADMIN.md).

O programa não solicita administrador automaticamente, não instala driver,
não altera o antivírus e não faz conexão com servidores. Atualizações são
manuais: substituir o executável por uma nova build assinada pelo proprietário.

## Emissor separado

Para uso diário, utilize o **ZB2 Admin**, que permite emissão sem terminal,
seleção de prazo, histórico e autorização de integrantes em outros PCs.
Consulte o [guia da equipe](ADMIN.md). O script abaixo permanece como ferramenta
técnica; não é mais a interface de operação da equipe.

Dependência no PC do proprietário: Python com `cryptography`. O cliente não precisa
de Python. A chave é ECDSA P-256 e fica protegida por DPAPI do usuário Windows.
Nenhuma chave privada, PAT ou gerador de licença integra o executável.

Criar uma chave uma única vez, fora do repositório:

```powershell
python scripts/license_admin.py keygen --key D:/ZB2-Privado/issuer.dpapi --public packaging/loader-public-key.json
```

O comando recusa sobrescrever material existente. Na máquina de desenvolvimento
desta entrega, a chave já foi criada em `C:/Users/WeFagundes/Documents/ZB2Menu/Privado/issuer-owner.dpapi`.
Não criar outra chave para renovar licenças dos clientes desta build.

Emitir licença, com validade contada desde a emissão em UTC:

```powershell
python scripts/license_admin.py issue --key C:/Users/WeFagundes/Documents/ZB2Menu/Privado/issuer-owner.dpapi --device ID_DE_64_CARACTERES --days 30 --output cliente.zb2license
```

Guardar a chave e o perfil Windows com backup protegido. Copiar apenas o arquivo
DPAPI para outro PC/usuário não basta para recuperar a chave privada. Esta versão
não oferece exportação portável nem rotação automática de chave.

## Build reproduzível a partir dos fontes e entradas registradas

Requer MSVC C++ x64, Windows SDK e Python `cryptography`. Os assets já estão
versionados; não é necessário baixar fontes durante o build. O script opcional
`prepare_loader_assets.py` exige fontTools 4.60.1 e registra proveniência SHA-256.

```powershell
python scripts/build_loader.py --runtime "D:/Projeto/ZB2 Menu/build/Release_x64" --runtime-commit 21759653045fed481d11e6ccd02122ad558250e7 --version 1.5-local --key C:/Users/WeFagundes/Documents/ZB2Menu/Privado/issuer-owner.dpapi --public packaging/loader-public-key.json --tests
```

O helper é recompilado dos fontes atuais. Os três DLLs são copiados sem alteração
do runtime selecionado. `runtime_commit` identifica a origem dessas DLLs; não
substitui o SHA-256 individual de cada entrada. O manifesto lista os sete arquivos
permitidos, tamanho e hash e recebe assinatura ECDSA. Cada arquivo é comprimido
com MSZIP e embutido em recursos PE. Assinaturas ECDSA e timestamps do compilador
podem tornar dois binários diferentes: não se promete reprodutibilidade bit a bit.

`build/loader/build-metadata.json` registra hashes do executável, da chave pública
e de todas as entradas. `build/loader/build.log` guarda a compilação. Fontes,
símbolos, ferramentas de teste e credenciais não são copiados para dist.

O AUTO-INJECT e seus limites estão documentados em [AUTO_INJECT.md](AUTO_INJECT.md).

## Instalação e recuperação

O loader verifica a assinatura do manifesto e todos os hashes antes de escrever
o runtime em `Documentos/ZB2Menu/loader/runtime/<versão>-<hash>`.
Escreve em staging e publica a pasta por renomeação somente ao terminar.
Instalações existentes são verificadas e nunca substituídas silenciosamente.
Reparse points, arquivos extras, entradas desconhecidas e corrupção são recusados.
Pastas Documentos redirecionadas por junction/symlink não são suportadas nesta fase.

Uma falha de energia pode deixar uma pasta `.staging-*`; ela não é utilizada
como runtime válido. Remoção de resíduos é manual nesta versão. Configurações
e presets do menu são preservados. O loader não faz limpeza recursiva de pastas.

Ativação e maior horário observado ficam em `Documentos/ZB2Menu/loader/license.dat`,
protegidos por DPAPI e substituídos atomicamente. O checkpoint é salvo na ativação,
antes do carregamento e a cada minuto enquanto a licença está válida. Regressões
maiores que 120 segundos são recusadas. A validade também é verificada imediatamente
antes de iniciar o helper. `last-load.log`, na mesma pasta, registra a última tentativa.

Uma DLL já carregada impede nova injeção. O helper recebe um PID específico,
sem espera por outro processo, sem bootstrap artificial e sem repetição automática.
O loader recusa versões diferentes do `Assembly-CSharp.dll` validado. Falha ou
timeout pede reiniciar o jogo. A janela pode fechar sem matar o jogo ou a thread remota.

## Testes

```powershell
python tests/test_loader_functional.py --exe build/loader/loader-tests.exe --key C:/Users/WeFagundes/Documents/ZB2Menu/Privado/issuer-owner.dpapi -v
Push-Location build/loader
./loader-tests.exe
Pop-Location
```

O primeiro conjunto valida licença legítima, outro dispositivo, expiração exata,
início futuro, outro emissor, payload adulterado, claims malformadas assinadas,
entradas excessivas/truncadas, DPAPI, rollback, instalação repetida, corrupção de
arquivos e recursos PE e bloqueio de redirecionamento. Não usa o jogo.

O segundo renderiza os estados reais do ImGui a 100%, 150% e 200% e testa cliques
sintéticos, toggle AUTO-INJECT, acionamento manual, ativação e fechamento. Os cliques
são do contexto de teste; não controlam a tela, mouse ou teclado do usuário.

A validação final em partida, DPI entre monitores reais e um segundo PC sem ambiente
de desenvolvimento continua necessária antes de declarar a versão pronta para venda.

## O que esta proteção não promete

- Sem servidor ou relógio confiável externo, não há revogação imediata ou garantia
  de prazo contra restauração de backups, VM, exclusão do estado ou alteração do executável.
- MachineGuid identifica a instalação do Windows; pode mudar na reinstalação ou
  ser clonado. Não é uma identidade física inviolável.
- A licença é aplicada pelo loader. O runtime legado ainda não valida a licença
  independentemente; extrair e chamar as DLLs por outro caminho pode contornar o loader.
  Esta fase organiza e valida a distribuição; não é DRM completo.
- A assinatura do pacote é diferente de Authenticode. O executável ainda não possui
  certificado de editor. Não há garantia de ausência de alertas de antivírus;
  correções de falso positivo devem usar os canais oficiais do fornecedor.
- Não existem status “UNDETECTED”, atualização online, painel de vendas ou cobrança.

Referências técnicas: [BCryptVerifySignature](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptverifysignature)
e as licenças/proveniências em `apps/loader/assets`.

Organização de pastas e ícones: [PASTAS.md](PASTAS.md).
