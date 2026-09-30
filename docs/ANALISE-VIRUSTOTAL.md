# Análise do relatório VirusTotal

Consulta por API autenticada em 30/09/2026. Nenhuma amostra foi enviada nesta consulta. A credencial não foi incluída nos arquivos deste relatório.

## Identificação das amostras

| Amostra | SHA-256 | Resultado |
|---|---|---|
| ZIP fornecido | `c899291221777d2160614dc0bec13cf7398fefea6077c2bde82aa9b64ec37e80` | 5 malicious, 62 undetected |
| ZB2Menu.exe contido no ZIP | `5d108d235105b54806298c5a942f95cec539e6e70fa34a598eb7ef28d20a654c` | 7 malicious, 64 undetected |
| Cliente atual 1.14 | `3090deb7a5e3632770160782a2b9b1b708ab25f13716f0f9c5d2aa53a57fb28b` | API retornou 404; sem relatório disponível |

Análise do ZIP: 2026-09-30T21:03:13+00:00. Análise do executável: 2026-09-30T21:03:34+00:00.

O ZIP contém um executável de 3.870.720 bytes. O hash dele coincide com o cliente anterior registrado em build/defender-v113.json, analisado localmente sem detecção pelo Defender. O ZIP atual possui outro hash; uma alteração de hash não demonstra melhora nem piora de segurança.

## Detecções do executável

| Mecanismo | Rótulo |
|---|---|
| Bkav | W32.Malware.60D82761 |
| Symantec | ML.Attribute.HighConfidence |
| Elastic | malicious (high confidence) |
| Kaspersky | VHO:Trojan.Win32.Convagent.gen |
| Ikarus | Trojan.Win64.Krypt |
| Google | Detected |
| APEX | Malicious |

Microsoft consta como undetected tanto para o ZIP quanto para o executável. Undetected significa que aquele mecanismo não sinalizou a amostra nessa análise; não é um certificado de segurança. A soma da tela exclui motores sem resultado útil, como type-unsupported.

## Triagem dos indicadores

- O indicador CAPA chamado "log keystrokes via polling" apresenta como correspondência apenas a API GetKeyState. No código inspecionado, ela aparece no backend Win32 do ImGui para eventos/modificadores de teclado (imgui/imgui_impl_win32.cpp). Essa correspondência isolada não comprova gravação de teclas.
- A leitura de MachineGuid corresponde à função DeviceId em apps/loader/license.cpp, usada para vincular a licença ao PC.
- Consulta de processos e módulos corresponde à detecção do jogo em apps/loader/services.cpp.
- Recursos compactados correspondem ao empacotamento MSZIP em scripts/build_loader.py. Essa associação é uma hipótese para alguns indicadores genéricos, não a causa comprovada de cada detecção.
- Um indicador de assinatura inválida traz, no detalhe, erro de arquivo não encontrado no ambiente do analisador. Não deve ser tratado isoladamente como prova de adulteração da assinatura. A inspeção local anterior classificou o executável como não assinado.
- Outros rótulos, como referência a Xen, não foram suficientemente atribuídos nesta triagem. Não foram descartados nem usados como prova conclusiva.

O resumo inclui indicadores estáticos CAPA junto dos dados de execução. Não se deve apresentar todos eles como ações que ocorreram efetivamente. A análise sem o jogo e sem uma licença válida também não exercita necessariamente todo o carregamento.

## Conclusão e encaminhamento

Há detecções reais de múltiplos mecanismos. Ainda não há base suficiente para afirmar que todas são falsos positivos. O resultado limpo do Defender é limitado às amostras e ao momento registrados.

Não foi alterado o código para tentar diminuir a contagem de detecções. Não foram criadas exclusões, removidas proteções ou enviados arquivos adicionais.

Próximos passos: preservar estas amostras/hashes, revisar os componentes sinalizados e solicitar revisão da classificação aos fabricantes, anexando evidências do propósito e da compilação. Só classificar como falso positivo após fundamentar a conclusão ou obter resposta do fornecedor. Assinatura Authenticode trata identidade/reputação e não elimina automaticamente detecções.

## Fontes

- ZIP: https://www.virustotal.com/gui/file/c899291221777d2160614dc0bec13cf7398fefea6077c2bde82aa9b64ec37e80/detection
- Executável: https://www.virustotal.com/gui/file/5d108d235105b54806298c5a942f95cec539e6e70fa34a598eb7ef28d20a654c/detection
- API: https://docs.virustotal.com/reference/file-info
