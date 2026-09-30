# Proposta de atualizações pelo GitHub

## Situação atual

O repositório OTheMandaloriano/Menu-ZB2 é privado. Nenhum atualizador online foi
ativado nesta entrega, nenhum repositório novo foi criado e nenhuma release foi publicada.

## Direção recomendada

Manter o código no repositório privado e publicar somente os executáveis em um
repositório de distribuição separado. O nome desse repositório ainda será escolhido.
Os arquivos distribuídos seriam públicos; o uso continuaria sujeito à licença.
Se os próprios downloads precisarem ser privados, será necessário um serviço de
autorização ou credenciais individuais. Nunca embutir um token pessoal do GitHub no cliente.

## Experiência para o usuário

1. Ao abrir, o programa consulta a versão estável disponível.
2. Uma atualização válida é baixada em segundo plano, se essa opção estiver ativada.
3. A interface mostra o que mudou e informa quando está pronta para instalar.
4. Com o jogo fechado, o usuário aceita reiniciar o aplicativo para aplicar.
5. Licenças, histórico, configurações e chave do Admin são preservados.
6. Se a nova versão falhar na inicialização, restaurar a anterior.

Não substituir uma DLL dentro de uma partida. O menu acompanha a atualização do
cliente e só é carregado na próxima sessão compatível do jogo.

## Verificações obrigatórias

- HTTPS e origem de download permitida.
- Manifesto assinado com chave específica de atualização, diferente da chave de licenças.
- Versão, produto, arquitetura, tamanho máximo, SHA-256 e compatibilidade com o jogo.
- Rejeitar assinatura inválida, arquivo alterado e downgrade não autorizado.
- Conferir tudo antes de executar o instalador ou substituir o programa.
- Troca atômica, cópia de recuperação e verificação após reinício.
- Não executar comandos fornecidos por descrições ou textos de uma release.
- Não incluir credenciais do repositório, perfis ou chaves nos arquivos de distribuição.
- Offline ou limite de API: manter a versão instalada funcionando quando a licença permitir.

## Etapas de implementação

1. Decidir se o download dos binários pode ser público.
2. Definir repositório de distribuição e autoridade de assinatura.
3. Implementar e testar primeiro a consulta de versão, sem instalar nada.
4. Acrescentar download, verificação e instalação com recuperação.
5. Publicar uma release de teste somente após autorização explícita.

GitHub hospeda os arquivos; não elimina o SmartScreen. A assinatura Authenticode
identifica o publicador no Windows e é distinta da assinatura do manifesto de
atualização ou da licença offline.

Referências:
- https://docs.github.com/en/rest/releases/assets
- https://docs.github.com/en/repositories/releasing-projects-on-github/linking-to-releases
- https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/smartscreen-reputation
