# Menu nativo

Este diretório contém os fontes que antes ficavam na raiz do repositório.
O projeto kiero-dx11-base.vcxproj permanece na raiz e aponta para estes arquivos.

- main.cpp: inicialização, hooks e ciclo de vida.
- mono.cpp: integração nativa com Unity Mono e publicação dos dados.
- gui.cpp e módulos *_ui.inl/*_draw.inl: interface e desenho do menu.
- config*, runtime_* e arquivos auxiliares: configuração, sincronização e regras.

A mudança de diretório preserva a lógica dos fontes. As dependências imgui e kiero
continuam na raiz e são resolvidas pelos diretórios de include do projeto.
Leia ../../AGENTS.md e ../../docs/ARQUITETURA.md antes de alterar o runtime.
