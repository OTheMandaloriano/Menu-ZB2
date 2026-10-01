# Recuperar o proprietário em outro Windows

## O que está disponível

O Admin 1.16 exporta um arquivo `.dbrecovery` protegido por senha. Ele permite
restaurar a mesma chave de proprietário em um perfil vazio e protegê-la novamente
com DPAPI no Windows de destino. GitHub não armazena nem recupera sua chave.

O backup contém a chave e o nome do proprietário. **Não inclui histórico de clientes,
autorizações da equipe ou configurações.** Guarde esses dados em backup separado.

## Antes de trocar de PC

1. Abra o Admin no PC e usuário que já têm acesso de proprietário.
2. Entre em Configurações > Recuperação do acesso.
3. Defina uma senha longa e única, de 12 a 128 caracteres, e confirme-a.
4. Salve o arquivo em um nome novo. O destino sugerido é Documentos/ZB2Menu/BackupsPrivados.
5. Guarde uma cópia em mídia externa protegida. Guarde a senha em outro local seguro.
6. Mantenha o PC de origem até testar a restauração. O programa não recupera senha perdida.

O arquivo é privado: quem tiver arquivo e senha terá a chave de proprietário.
Não envie à equipe, clientes, GitHub, VirusTotal ou suporte.

## No destino

1. Baixe o Admin do mesmo produto e extraia para uma pasta gravável.
2. Ainda sem criar uma estação de integrante, abra Meu acesso > Restaurar backup de proprietário.
3. Digite a senha e escolha o arquivo `.dbrecovery`.
4. Aguarde a confirmação e confira o papel Proprietário.
5. Teste uma emissão controlada antes de descartar a instalação anterior.

O programa recusa senha errada, conteúdo adulterado, chave de outro produto e
sobrescrita de uma estação já configurada. Não apague um perfil existente para
contornar o aviso; preserve-o e peça orientação de migração.

## Histórico e equipe

Esta primeira função recupera acesso de emissão, não migra o histórico. As licenças
existentes continuam assinadas pela mesma autoridade. Para consultar emissões e
estações antigas, preserve os dados originais em backup. Não há importador completo
de histórico nesta entrega. Integrantes continuam criando uma nova solicitação
quando mudam de PC. Clientes precisam de licença vinculada ao ID correto.

## O que foi testado

- Autoridade fictícia, senha errada, arquivo modificado/truncado, limite de tamanho e perfis existentes.
- Verificação independente do arquivo cifrado e autenticado em Python.
- Exclusão somente do perfil fictício de origem e recuperação posterior.
- Restauração sob outra conta Windows neste mesmo computador, onde a DPAPI original não abria.

Não foram apagados dados reais. O teste entre contas não substitui homologação em
dois computadores físicos. O formato não passou por auditoria criptográfica externa.
Detalhes: [RECUPERACAO-TECNICA.md](RECUPERACAO-TECNICA.md).

Se você já perdeu o PC original e não exportou esse backup, a limitação anterior
continua: baixar o GitHub ou copiar um `.dpapi` isolado não reconstrói a chave.
