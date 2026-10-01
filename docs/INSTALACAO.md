# Instalação para quem está chegando

## Escolha o programa certo

- Você vai jogar: baixe ou receba `01-CLIENTE-INICIAL.zip`.
- Você vai emitir licenças: receba `01-EQUIPE-INICIAL.zip` e a orientação do proprietário.
- Você é o proprietário trocando de PC: leia RECUPERACAO.md antes de instalar ou apagar o PC antigo.

Não use o botão Code / Download ZIP para instalar. Ele baixa fontes para desenvolvedores.
Os arquivos de uso estão nos assets da release ou são enviados pela equipe.
O repositório é privado; pessoas sem acesso devem receber o ZIP por um canal combinado.

## Requisitos do cliente e do menu

1. Windows x64. O alvo desta entrega é Windows 10/11 com as APIs usadas pelo programa;
   isso não é uma afirmação sobre o ciclo de suporte do Windows.
2. Driver de vídeo compatível com DirectX 11.
3. Zumbi Blocks 2 instalado em uma versão compatível. O loader verifica a assembly do jogo
   antes de carregar; atualizar o jogo pode exigir uma nova versão do menu.
4. Permissão para salvar arquivos na pasta Documentos do usuário atual.
5. Licença emitida para o ID mostrado por esse cliente.

O runtime do menu, a ponte gerenciada, Harmony, o monitor e o helper acompanham o
pacote. Não baixe DLLs avulsas de sites de terceiros. O jogo fornece seu próprio
Unity Mono. O cliente não exige Visual Studio, Python, Cheat Engine ou CMake.

## Requisitos do Admin

Windows x64, vídeo compatível com DirectX 11 e .NET Framework para o serviço local.
Use .NET Framework 4.8 ou versão 4.x compatível mais recente suportada pelo seu
Windows. Essa é a base recomendada; não foi homologada uma matriz de versões antigas.
O jogo não precisa estar instalado para emitir licenças. É necessária autorização
do proprietário para que uma nova estação emita.

Windows 11 inclui .NET Framework 4.8 ou 4.8.1 conforme a versão. Para verificar ou
instalar o componente, use Windows Update e a documentação oficial da Microsoft:
https://learn.microsoft.com/en-us/dotnet/framework/install/

O painel não baixa nem repara essas dependências automaticamente nesta versão.

## Primeiro uso do cliente

1. Extraia o ZIP para uma pasta fixa, gravável pelo seu usuário. Não execute dentro do ZIP.
2. Abra ZB2Menu.exe e entre em Meu acesso > Copiar ID.
3. Envie o ID à equipe. Não precisa editar caminho ou nome de usuário no código.
4. Receba o ZIP ativado, feche o programa e extraia os novos arquivos na pasta do aplicativo.
5. Abra ZB2Menu.exe. O arquivo de licença incluído é importado automaticamente.
6. Abra o jogo e entre em uma partida. Com AUTO-INJECT ativado, aguarde a confirmação.
7. Use INSERT no jogo. Se houver erro, abra Detalhes e envie a mensagem à equipe.

## Primeiro uso da equipe

1. Extraia o ZIP e abra ZB2Admin.exe.
2. Em Meu acesso, informe seu nome e crie a solicitação.
3. Envie o arquivo .zb2station ao proprietário.
4. Receba o ZIP autorizado, feche o Admin e extraia o novo pacote.
5. Abra no mesmo PC e usuário Windows. Confira o acesso antes de emitir.

## Se não abrir

| Sintoma | Próxima ação |
|---|---|
| SmartScreen / fornecedor desconhecido | Conferir origem e hashes; consultar a equipe. A entrega não tem Authenticode |
| Antivírus remove um arquivo | Guardar o nome exato da detecção e do arquivo; não desativar a proteção como solução |
| Janela não aparece ou erro gráfico | Atualizar o driver por fonte oficial e conferir compatibilidade DirectX 11 |
| Serviço do Admin não inicia | Conferir .NET Framework, permissões e diagnóstico da equipe |
| Acesso negado ao jogo | Abrir jogo e loader no mesmo nível de permissão; enviar o código de erro se persistir |
| Versão incompatível do jogo | Aguardar pacote compatível da equipe |
| Arquivo ausente ou pacote alterado | Obter novamente o pacote verificado; não misturar DLLs de versões diferentes |

As pastas de dados são calculadas pelo Windows. Não copie a pasta Admin de outra
pessoa nem apague Documentos/ZB2Menu para tentar resolver uma instalação.
