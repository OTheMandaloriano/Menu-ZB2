# Organização em Documentos — versão 1.5

Pasta principal: `Documentos/ZB2Menu`, resolvida pela API Known Folders do Windows.

| Pasta | Conteúdo |
|---|---|
| Aplicativos/Admin | ZB2Admin.exe de uso diário |
| Aplicativos/Cliente | ZB2Menu.exe de uso diário |
| Admin | Identidade de proprietário/integrante, histórico e serviço local |
| loader | Ativação, preferências e runtime verificado do cliente |
| Pacotes | Destino inicial da janela Salvar ZIP; pacotes iniciais |
| Icones | PNGs originais e ICOs de Admin, Cliente e Menu |
| Privado | Chave DPAPI do proprietário; nunca distribuir |
| Backups | Cópias anteriores à migração; nunca distribuir |

O aplicativo não usa a pasta TEMP do Windows para instalar seu serviço ou runtime.
Arquivos `.tmp` usados para escrita atômica são criados ao lado do destino e
renomeados/removidos ao concluir. O Windows, o jogo e ferramentas externas podem
manter seus próprios caches; essa mudança não limpa nem controla tais arquivos.
O Player.log do jogo continua sendo lido em LocalLow, sem transferir arquivos do jogo.

O código-fonte e os outputs de compilação continuam no repositório de desenvolvimento.
Os ícones gerados foram copiados para o projeto e para Documentos; não dependem
da pasta de geração da ferramenta. Os ZIPs personalizados continuam com apenas
o executável e a licença/autorização. Os ícones e atribuições ficam embutidos.

Nesta máquina foi migrado o perfil limpo verificado WeFagundes — Proprietário de
`D:/ZB2-Retomada/loader-private/admin-proprietario`. A chave DPAPI foi copiada
sem alterar seus bytes, para o mesmo usuário Windows. Dados antigos são preservados
como recuperação; o atalho de uso aponta exclusivamente para Documentos.

Não enviar a pasta inteira ZB2Menu a clientes ou equipe. Enviar somente o ZIP
personalizado gerado pelo Admin, conforme [ADMIN.md](ADMIN.md).
