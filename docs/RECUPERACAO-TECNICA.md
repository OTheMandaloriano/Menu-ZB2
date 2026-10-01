# Formato de recuperação do proprietário

O arquivo .dbrecovery carrega somente a chave de proprietário e o nome de exibição.
Não carrega histórico, autorizações, configurações, DPAPI do PC de origem ou senha.
Os dados são cifrados e autenticados. Não distribuir esse arquivo como pacote de cliente.

## Contrato criptográfico v1

- Cabeçalho: DBREC001, iterações fixas, salt de 16 bytes, IV de 16 bytes e tamanho.
- KDF: PBKDF2-HMAC-SHA256, 600.000 iterações, 64 bytes derivados.
- Cifra: AES-256-CBC com PKCS7 usando a primeira metade da derivação.
- Autenticação: HMAC-SHA256 usando a segunda metade, cobrindo cabeçalho e ciphertext.
- O tag é comparado sem saída antecipada antes de qualquer descriptografia.
- Salt e IV são gerados a cada exportação pelo RNG do sistema.
- Entrada máxima de 16 KiB, ciphertext limitado e iterações fixas para evitar parâmetros abusivos.
- O conteúdo restaurado deve ser Menu-ZB2 e a chave deve corresponder à chave pública do programa.
- No destino, a chave é protegida novamente com DPAPI CurrentUser.

A composição usa primitives do .NET Framework. Precisa de .NET Framework 4.8 conforme
o requisito publicado. Senha de 12 a 128 caracteres; recomendar frase longa e única.
Se perder o arquivo ou a senha, não existe reset remoto que recupere essa chave.

## Controles de arquivo e interface

- Exportação não sobrescreve um backup existente; restauração não substitui estação configurada.
- Escrita temporária seguida de troca/movimentação atômica; caminhos redirecionados são recusados.
- Destino sugerido BackupsPrivados, separado de Pacotes. Recomendar cópia em mídia externa protegida.
- Senha via pipe herdado entre processos locais, nunca argumentos do processo ou logs.
- Campos são apagados ao enviar ou navegar; buffers nativos são sobrescritos após o uso.
- Strings gerenciadas do .NET não permitem garantia de apagamento de todas as cópias em memória;
  não há promessa de resistência contra administrador/malware que já controle o PC.

## Evidências e limites

tests/run_recovery_validation.py cria autoridade fictícia e verifica senha errada,
adulteração, truncamento, limite de arquivo, estação existente, produto diferente,
restrição a proprietário e limpeza de temporários. A verificação independente em
Python confere KDF, HMAC e cifra. O cenário apaga somente o perfil fictício de origem
e demonstra recuperação posterior e assinatura com a mesma chave pública.

RecoveryTests cross-user foi executado sob outra conta Windows neste computador:
a DPAPI original falhou e o backup portátil restaurou corretamente o proprietário.
Isso não é ainda uma homologação em dois PCs físicos, nem uma auditoria criptográfica
independente. Não use o perfil real para testes destrutivos.
