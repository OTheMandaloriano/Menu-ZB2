# Retenção e limpeza

## Resposta direta

Atualizar pode deixar arquivos antigos. Não existe garantia de zero resíduos na
versão atual. Há uma limpeza limitada no Admin; não há um limpador geral do PC.

| Local | O que é | Regra atual |
|---|---|---|
| Admin/station.json | Perfil e chave protegida da estação | Nunca limpar como cache |
| Admin/licenses e autorizações | Histórico de emissão | Preservar |
| Admin/preferences.txt | Preferências | Preservar |
| Admin/runtime/<hash> | Serviço extraído do Admin | Versões antigas podem ser analisadas e limpas pelo painel |
| loader/license.dat | Ativação do cliente | Preservar |
| loader/auto-inject.txt | Preferência de carregamento | Preservar |
| loader/runtime/<versão-hash> | Componentes do menu | Versões anteriores podem permanecer; limpeza automática ainda não existe |
| configs e imgui.ini | Configurações do usuário | Preservar |
| logs e last-load.log | Diagnóstico | Não entram na limpeza do painel |
| Pacotes | ZIPs salvos pelo operador | Exclusão apenas por decisão do operador |
| BackupsPrivados/*.dbrecovery | Recuperação da chave do proprietário por senha | Preservar; nunca distribuir como pacote nem enviar ao GitHub |
| ZIPs e pastas em Downloads/Desktop | Arquivos extraídos pelo usuário | Fora do escopo da limpeza do programa |

Os caminhos da tabela são relativos a Documentos/ZB2Menu, salvo indicação diferente.

## Como a limpeza do Admin funciona

1. Analisar resíduos faz uma leitura, sem excluir nada.
2. Mostra quantidade e espaço dos serviços antigos elegíveis.
3. Limpar analisados atua apenas nos arquivos daquela análise.
4. Reconfere caminho, conteúdo e hash antes da remoção.
5. Preserva o serviço atual, arquivos em uso ou modificados, diretórios redirecionados
   e pastas que contenham arquivos desconhecidos.

Pode ser usada com o próprio Admin aberto: o serviço atual é excluído da lista.
A ferramenta não remove o runtime do jogo, não limpa Temp global, não troca a
licença e não apaga chaves ou backups. A limpeza externa manual deve ser feita
com os aplicativos envolvidos fechados e uma lista específica revisada.

## Temporários de instalação

O cliente prepara componentes em uma pasta .staging junto do runtime e só depois
conclui a instalação. Falhas tratadas removem os arquivos dessa tentativa. Queda
de energia ou encerramento forçado podem deixar uma pasta interrompida; não há
coleta automática dessas sobras na versão atual.

## Política proposta para o atualizador futuro, ainda não implementada

- Manter a versão ativa e uma versão anterior confirmada para recuperação.
- Confirmar jogo e aplicativos fechados antes de trocar ou limpar componentes.
- Verificar que nenhum processo usa a versão candidata à remoção.
- Remover somente downloads/staging pertencentes ao próprio atualizador, por manifesto.
- Nunca excluir arquivos desconhecidos, dados pessoais ou pastas escolhidas livremente.
- Registrar o resultado e informar o que ficou preservado.
- Testar falta de espaço, interrupção, arquivo bloqueado, falha de assinatura e recuperação.

Esta política é requisito de desenvolvimento, não descrição de uma função pronta.
