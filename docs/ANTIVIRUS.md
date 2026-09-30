# Diagnóstico de antivírus

## Situação verificada

O Windows Defender foi identificado neste computador. A consulta realizada não
retornou uma detecção específica dos arquivos do projeto. Admin e cliente estavam
sem assinatura Authenticode. Isso não permite concluir que o alerta é falso positivo.

O loader contém componentes que carregam código no processo do jogo. O motivo de
uma detecção deve ser confirmado pelo relatório, não inferido apenas pelo uso do programa.
SmartScreen, reputação de publicador e uma detecção de malware são diagnósticos distintos.

## VirusTotal

A tentativa de consultar os hashes no site não retornou relatórios acessíveis pela
ferramenta web. Não havia chave de API configurada. Nenhum resultado de varredura foi
inventado e nenhum executável foi enviado nesta consulta.

O serviço público pode compartilhar arquivos enviados com parceiros e clientes.
Não envie perfis, chaves, licenças ou ZIPs personalizados. Se optar por enviar uma
amostra, use somente o executável de produto correspondente ao alerta e guarde o
link, SHA-256, versão, data e nome da detecção. Uma análise antiga não vale como
resultado automático para uma nova compilação, cujo hash pode ser diferente.

## Procedimento para resolver uma detecção

1. Identificar o arquivo e o nome exato do alerta no Histórico de proteção.
2. Conferir o SHA-256 contra a compilação conhecida e revisar origem/dependências.
3. Consultar o relatório correspondente e analisar quais comportamentos foram marcados.
4. Corrigir qualquer problema real encontrado. Se a classificação for indevida,
   encaminhar a amostra ao fornecedor usando o canal de revisão de falso positivo.
5. Usar assinatura digital consistente para identificar o publicador. Isso não
   garante ausência de alertas e não substitui a revisão de segurança.

Não criar exclusões amplas, desativar o Defender ou usar ofuscação para tentar
esconder a detecção. Não há garantia de um resultado “indetectável”.

Fontes oficiais:

- https://learn.microsoft.com/en-us/defender-xdr/developer-faq
- https://www.microsoft.com/en-us/wdsi/filesubmission
- https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/smartscreen-reputation
- https://docs.virustotal.com/docs/how-it-works

## Verificação local concluída em 30/09/2026

Foi executada uma análise personalizada, sem remediação automática, com o Defender
4.18.26080.4 e assinaturas 1.459.485.0. Os sete arquivos verificados foram
ZB2Admin.exe, ZB2Menu.exe, kiero-dx11-base.dll, injector.exe, Zb2.AimBridge.dll,
0Harmony.dll e ZB2.Readiness.dll. Todos retornaram código 0 e found no threats.
A proteção em tempo real permaneceu ativa.

Isso registra o resultado local dessas amostras, não uma garantia de segurança,
resultado do VirusTotal ou teste de comportamento durante a injeção. O alerta anterior
ainda não foi reproduzido. Hashes e saída integral estão em build/defender-v113.json.

## Consulta posterior com a API do VirusTotal

A consulta autenticada do relatório fornecido confirmou 5 detecções no ZIP e 7 no executável anterior. Microsoft aparece como undetected. O hash do cliente atual retornou 404. Consulte ANALISE-VIRUSTOTAL.md para os resultados e limites. Nenhuma amostra foi enviada nesta consulta e a credencial não foi gravada no projeto.
