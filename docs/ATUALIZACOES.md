# Rotina de atualização — agentes e desenvolvedores

1. Trabalhar a partir de `D:/Projeto/ZB2 Menu`. Ler AGENTS.md e verificar alterações
   staged/unstaged antes de editar. Não usar uma cópia antiga como fonte da verdade.
2. Preservar o material de `.local` e os dados em Documentos. Não regenerar a chave
   de emissão nem mudar o identificador assinado `Menu-ZB2` numa atualização visual.
3. Se o runtime do jogo mudou, compilar/validar o menu e registrar seu commit e hashes.
   Não atualizar offsets ou declarar a injeção validada apenas por compilar.
4. Compilar e testar o loader com `scripts/build_loader.py`; a chave DPAPI deve
   pertencer ao usuário da compilação. Nunca copiar a chave privada para dist.
5. Compilar `scripts/build_admin.py --tests` DEPOIS do loader, pois o Admin incorpora
   esse executável para gerar todos os ZIPs de cliente, iniciais ou ativados.
6. Validar renderizações e fluxo real de emissão com os testes existentes, incluindo
   ZIP inicial sem ID, retorno do ID/solicitação e ZIP final assinado. Usar perfis
   temporários de teste dentro de build, nunca criar clientes fictícios no perfil real.
7. Gerar os dois ZIPs iniciais com `scripts/package_delivery.py`. Conferir conteúdo
   e hashes. Não duplicar os executáveis em subpastas descompactadas de entrega.
8. Fechar o Admin normalmente, substituir os executáveis locais e reabrir. Verificar
   papel de proprietário e preservação do histórico. Reempacotar os ZIPs que serão enviados
   depois da atualização; ZIPs já enviados não se atualizam sozinhos.
9. Fazer inventário antes da limpeza. Apagar apenas a lista revisada de outputs
   obsoletos ou duplicados, conferindo caminhos, hashes, arquivos em uso e Git.
   Não usar limpeza recursiva abrangente em build, Documentos ou .local.
10. Para cada publicação, confirmar a autorização explícita do proprietário.
    Não presumir que aprovar uma alteração local autoriza publicar em main.

Versões: Admin e loader podem ter números diferentes. Uma melhoria de ajuda/ZIP no
Admin não exige recompilar o runtime do jogo nem trocar a licença do cliente.

## Distribuição de atualizações

- Cliente novo: ZIP inicial → ID → ZIP ativado.
- Cliente já licenciado: ícone ZIP na licença válida do histórico, usando Admin atualizado.
- Integrante novo: ZIP inicial → solicitação → ZIP autorizado.
- Integrante existente: reexportar o ZIP da estação autorizada pelo menu de contexto.

Somente os executáveis de produto entram nos ZIPs iniciais. Dados privados, histórico,
dependências de desenvolvimento, fontes, logs e arquivos de compilação ficam de fora.

Para o passo a passo de quem recebe uma atualização, consulte [ATUALIZAR-USUARIOS.md](ATUALIZAR-USUARIOS.md). Ao concluir uma entrega, atualize [CONTINUIDADE.md](CONTINUIDADE.md) e registre os limites da limpeza em [RETENCAO-E-LIMPEZA.md](RETENCAO-E-LIMPEZA.md).
