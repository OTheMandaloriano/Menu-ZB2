---
name: zb2-pesquisar-antes
description: Use ONLY when about to write or change cheat code (ESP, render, injection, memory) in the ZB2 Menu project and you are unsure of the correct pattern. Forces a web search for best practices before coding. Never triggers on its own output.
---

# Pesquisar antes de codar (ZB2 Menu)

Regra anti-círculo: esta skill **nunca dispara a si mesma** nem dispara
outra skill. Ela só manda pesquisar e voltar com fonte.

## Quando usar

- Vai escrever ou mudar código de ESP, render, hook, injeção ou leitura
  de memória no projeto ZB2 Menu **e** não tem certeza do padrão correto.
- Existe técnica canônica pública (W2S, skeleton, Present hook, MinHook,
  ImGui overlay) e você está tentado a improvisar.

## Quando NÃO usar

- A resposta já está em `memory/OFFSETS.md` (offset validado) ou no
  próprio código (padrão já estabelecido no projeto).
- A dúvida é sobre decisão do operador (menu, design, prioridade).
- Você acabou de pesquisar o mesmo tema nesta sessão (reutilize o achado).

## Procedimento (1 ida, sem loop)

1. Faça **1 rodada** de `websearch` (máx. 2 queries) sobre o padrão
   (ex.: "WorldToScreen w divide by zero guard", "ImGui Present hook
   thread safety").
2. Anote a fonte (URL + trecho) e adapte ao projeto UMA vez.
3. Se a busca não resolver, pergunte ao operador em vez de pesquisar
   de novo. **Pesquisar 2x o mesmo tema na mesma sessão é proibido.**

## Proibido

- Editar código a partir desta skill sem fonte.
- Encadear esta skill com outra skill (sem cascata).
- Virar desculpa para não decidir: na dúvida medida do projeto
  (log, print, `[ZTYPE]`) vale mais que opinião da internet.
