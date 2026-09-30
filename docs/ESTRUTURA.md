# Estrutura do projeto e dados de uso

## Projeto — D:/Projeto/ZB2 Menu

- `apps/`: código dos aplicativos; `apps/shared/brand` é a única fonte dos ícones.
- `scripts/`, `tests/`, `docs/`, `packaging/`: desenvolvimento, validação e documentação.
- `build/`: arquivos intermediários, testes e diagnósticos de compilação. Não enviar.
- `dist/admin/ZB2Admin.exe`: Admin atual para uso.
- `dist/loader/ZB2Menu.exe`: cliente atual para uso.
- `dist/entrega/`: ZIPs iniciais sem licenças pessoais.
- `.local/private/`: material de emissão protegido por DPAPI, ignorado pelo Git.
- `.local/backups/`: cópias de recuperação pessoais, ignoradas pelo Git.

Não colocar ícones originais, executáveis de desenvolvimento ou documentação
duplicada em Documentos. Não enviar o repositório inteiro ou `.local` a ninguém.

## Documentos/ZB2Menu — somente dados de uso

- `Admin/`: estação, histórico, autorizações, preferências e serviço local necessário.
- `loader/`: ativação, preferências, runtime do cliente e último log de carga.
- `configs/`, `imgui.ini`, `logs/`: configurações e logs do menu existente.
- `Pacotes/`: destino inicial dos ZIPs personalizados que o usuário decide salvar.

O serviço local extraído em `Admin/runtime/<hash>` faz parte do funcionamento,
não é lixo por existir. Não apagar o serviço em uso. Logs e backups não são
descartados automaticamente. Escritas atômicas usam `.tmp` junto ao destino,
sem depender da pasta TEMP global.

## Configurações do Admin

A aba permite reduzir movimento (persistido em Admin/preferences.txt), abrir
dados/pacotes/aplicativo/logs e exportar diagnóstico mínimo: versão, plataforma,
papel e contagens. O diagnóstico não inclui nomes, IDs, tokens ou chaves.
Nenhum botão apaga licenças/histórico ou limpa Temp indiscriminadamente.

## Limpeza realizada

Duplicatas produzidas por esta implementação só são excluídas após comparação
SHA-256 com o destino correto. Distribuições versionadas antigas em `dist/admin-1.*`
e `dist/loader-1.*` são outputs anteriores, preservados nesta rodada para recuperação. Fontes,
commits e artefatos de diagnóstico não relacionados permanecem intactos.

Referência: [separação de estado e conteúdo da aplicação, Microsoft](https://learn.microsoft.com/en-us/windows/apps/develop/windows-app-restore).

Limpeza comprovada nesta rodada: 18.619.367 bytes em dez duplicatas idênticas
retiradas de Documentos. Chave e backups pessoais foram transferidos individualmente
para .local com comparação de hash, sem apagar o conteúdo recuperável.
