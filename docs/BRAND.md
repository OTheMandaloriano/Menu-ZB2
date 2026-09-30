# DEADBLOCK — identidade do painel

DEADBLOCK identifica visualmente o projeto de OTheMandaloriano. O símbolo original usa
um D modular e uma diagonal azul. Admin e cliente compartilham a marca; a segunda
linha distingue `ZB2 / ADMIN` e `ZB2 / MENU`.

## Arquivos

- `apps/shared/brand/deadblock-symbol.svg`: símbolo transparente e escalável.
- `apps/shared/brand/deadblock-admin.svg` e `deadblock-menu.svg`: assinaturas com
  letras convertidas em contornos, sem dependência de fontes instaladas.
- PNGs correspondentes: 1200 × 256, fundo transparente.
- `mark.h`: desenho vetorial direto no ImGui; não carrega texturas do disco.
- `deadblock-provenance.json`: hashes dos arquivos da marca.

A alteração é visual: nomes de executáveis, chaves, produto assinado `Menu-ZB2`,
pastas e licenças continuam compatíveis. Ícones Windows existentes são preservados.

## Tipografia e compilação

Lexend Bold 18 px nos títulos, SemiBold 14 px nos controles e Medium 12 px nos
metadados. Textos e inputs preservam o avanço natural dos dígitos; datas usam medidas tabulares da fonte, sem espaçamento artificial. FontAwesome recebe
offset de baseline -2,5 px; controles personalizados alinham os limites visíveis
do glifo, e os inputs alinham o ícone ao texto realmente rasterizado.

FreeType 2.14.1 é compilado estaticamente com o adaptador oficial do ImGui 1.89.9,
usando LightHinting. `scripts/font_backend.py` verifica o SHA-256 do pacote fonte
antes de extrair. Precisa de CMake, Ninja e MSVC somente na compilação. Cache e
logs ficam em `build/freetype`; não há DLL adicional para distribuir.

O cliente não baixa dependências nem extrai arquivos de avisos. Atribuições
permanecem incorporadas nos recursos do executável. O logo do jogo não é utilizado.
