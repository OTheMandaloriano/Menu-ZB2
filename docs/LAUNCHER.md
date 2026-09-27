# Launcher 0.2.0, prévia privada

O executável `ZB2Menu.exe` contém um pacote local de runtime. Não exige Cheat Engine, dnSpy ou GitHub CLI no computador do usuário. Requer Windows x64 e .NET Framework 4.8 ou compatível. Não é assinado com Authenticode.

## Primeiro uso

1. Feche o jogo para preparar o pacote inicial.
2. Abra o launcher. Ele instala em Documentos, usando o caminho informado pelo Windows.
3. Abra o jogo e entre no mapa.
4. Marque que está no mapa e use **Carregar menu no jogo**.

```text
Documentos/ZB2Menu/
  runtime/0.2.0/        componentes verificados
  configs/             presets e preferências do launcher
  logs/                logs nativos e do launcher
  licenses/            avisos de dependências
  current.txt          versão ativa
  previous.txt         versão ativa anterior, após atualizar
```

Presets existentes em `Documentos/kiero-dx11-base/configs` são copiados somente quando não existem no destino. A pasta anterior é preservada. Versões de runtime antigas ficam disponíveis para diagnóstico; não há limpeza automática nem downgrade automático nesta prévia.

## Atualizações

O canal é [Menu-ZB2-Releases](https://github.com/OTheMandaloriano/Menu-ZB2-Releases), privado e separado dos fontes. O usuário precisa de acesso ao repositório e de seu próprio token GitHub com permissão **Contents: read**. O token não vem embutido no executável e não é escrito nos logs. Salvar a credencial é opcional; quando solicitado, usa DPAPI do usuário Windows.

- **Local:** usa o pacote embutido, sem consultar a rede.
- **Manual:** consulta GitHub pelo botão ou importa um ZIP escolhido pelo usuário.
- **Ao abrir:** consulta e instala uma versão mais recente somente se a opção estiver marcada. É desligada por padrão.

O canal inclui prévias. Downloads usam HTTPS e o digest SHA-256 da API GitHub; cada arquivo também é verificado pelo manifesto. Não há assinatura criptográfica independente do GitHub. Instalação recusa caminhos fora da lista esperada, arquivos duplicados, hashes divergentes, pacotes excessivos e substituição com o jogo aberto. Uma versão é ativada somente após preparar todos os arquivos.

As atualizações substituem o **runtime do menu**. Atualização do executável do launcher é manual: obtenha o novo `ZB2Menu.exe` na Release. Acesso ao repositório não é um sistema de licenciamento do menu; controle comercial de acesso não foi implementado.

## Build

```powershell
python scripts/build_launcher.py --runtime build/Release_x64 --version 0.2.0
python tests/run_launcher_validation.py
```

O runtime deve conter as seis dependências atuais, com configuração portátil. O script gera `ZB2Menu.exe`, `ZB2Menu-runtime.zip` e `SHA256SUMS.txt` em `build/launcher`. Não versione esses binários nos fontes. A Release privada recebe os arquivos para teste.

Referência do protocolo de download: [API de assets de Releases do GitHub](https://docs.github.com/en/rest/releases/assets).
