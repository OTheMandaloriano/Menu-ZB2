# Troca de computador e recuperação de acesso

## Resposta direta

Baixar o repositório ou o programa em outro PC não devolve acesso de proprietário.
O GitHub guarda fontes, recursos e releases. Ele não guarda sua chave privada.

Hoje, a chave da estação e o arquivo de manutenção do proprietário são protegidos
por DPAPI com escopo CurrentUser. Não conte com a cópia do arquivo .dpapi ou de
station.json como um backup portátil. Uma conta com o mesmo nome em outro Windows
também não garante que os dados possam ser descriptografados.

O botão de recuperação existente exige a chave original que o usuário Windows
consiga abrir. Ele não é um login GitHub, não recupera a chave pelo nome e não é
um assistente de transferência entre computadores.

## Proprietário: antes de trocar ou formatar

1. Preserve o computador e o usuário Windows que ainda conseguem abrir a chave.
2. Guarde backups privados da estação, histórico e material de manutenção em mídia
   protegida. Esses backups são importantes, mas a cópia DPAPI isolada não comprova portabilidade.
3. Solicite uma migração assistida ou o desenvolvimento de exportação/importação
   criptografada por senha enquanto a chave original ainda puder ser aberta.
4. Exija um teste de restauração no PC de destino, verificando a mesma chave pública
   e a emissão de uma licença de teste, antes de apagar a origem.
5. Nunca envie chaves, backups ou perfis por GitHub, chat de suporte ou ZIPs de cliente.

**Limitação atual:** não existe no painel exportação portátil por senha nem um
procedimento de migração entre PCs homologado. Não formate o PC original contando
que baixar o Admin será suficiente.

## Se o PC original já foi perdido

É necessário avaliar os backups e as condições de recuperação do Windows. Não há
garantia de recuperar a chave. A chave pública do programa não permite reconstruir
a privada. Se ela for irrecuperável, será necessário planejar uma nova autoridade,
novos programas e reemissão dos acessos; isso não é uma troca automática de nome.

## Integrante da equipe em outro PC

Crie uma nova estação no PC novo, envie uma nova solicitação ao proprietário e
importe a autorização emitida para ela. Preserve o histórico antigo em backup;
ele não se sincroniza automaticamente e não concede permissão de emissão.

## Cliente em outro PC

O ID pode mudar. Abra o cliente no novo PC, copie o ID e solicite à equipe o acesso
correspondente. Copiar a licença antiga pode resultar em erro de dispositivo.

## Atualização no mesmo PC

Substituir o executável com o programa fechado mantém o perfil em Documentos.
Isso é diferente de trocar de PC, conta Windows ou reinstalar o sistema.

Referência de proteção de dados:
https://learn.microsoft.com/en-us/dotnet/api/system.security.cryptography.dataprotectionscope
