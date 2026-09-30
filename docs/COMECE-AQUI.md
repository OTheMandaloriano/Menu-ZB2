# Comece aqui: DEADBLOCK / Zumbi Blocks 2

Este é o ponto de entrada para o proprietário, a equipe e o próximo agente.

## O que funciona hoje

- Distribuição manual por ZIP, com programas iniciais que não exigem ID ou solicitação.
- Emissão de licença e autorização offline. Cada PC conserva os próprios dados.
- Cliente 1.15-local; interface do Admin 1.14, recompilada para incorporar esse cliente.
- No Admin: Configurações > Analisar resíduos > Limpar analisados.
- Essa limpeza remove apenas serviços antigos do Admin que passaram nas verificações.

## O que ainda não existe

- Atualização automática pelo GitHub.
- Instalador de atualização com retorno automático à versão anterior.
- Limpeza automática dos runtimes antigos do cliente/menu.
- Reparação automática de dependências do Windows.
- Assinatura Authenticode de publicador.

Não prometer esses recursos ao cliente como se já estivessem implementados.

## Qual guia ler

| Necessidade | Documento |
|---|---|
| Enviar pela primeira vez ao cliente ou integrante | [ADMIN.md](ADMIN.md) |
| Atualizar quem já recebeu o programa | [ATUALIZAR-USUARIOS.md](ATUALIZAR-USUARIOS.md) |
| Entender as pastas | [ESTRUTURA.md](ESTRUTURA.md) |
| Entender resíduos e o que preservar | [RETENCAO-E-LIMPEZA.md](RETENCAO-E-LIMPEZA.md) |
| Assumir o desenvolvimento em nova sessão | [CONTINUIDADE.md](CONTINUIDADE.md) |
| Mapa do código e de Release_x64 | [ARQUITETURA.md](ARQUITETURA.md) |
| Compilar e distribuir uma nova versão | [ATUALIZACOES.md](ATUALIZACOES.md) |
| Implementar atualizações online no futuro | [ATUALIZACAO-GITHUB.md](ATUALIZACAO-GITHUB.md) |
| Interpretar estados do loader | [ESTADOS-LOADER.md](ESTADOS-LOADER.md) |
| Alertas de antivírus | [ANTIVIRUS.md](ANTIVIRUS.md) e [ANALISE-VIRUSTOTAL.md](ANALISE-VIRUSTOTAL.md) |

O projeto oficial está em `D:/Projeto/ZB2 Menu`. A pasta `D:/ZB2-Retomada` contém
cópias de trabalho e auditorias de sessões anteriores. Ela não é a fonte canônica
para uma nova atualização e não deve substituir automaticamente o projeto oficial.

Identidade pública e caminhos que funcionam em qualquer conta: [PORTABILIDADE.md](PORTABILIDADE.md).
