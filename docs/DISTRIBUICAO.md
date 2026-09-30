# Distribuição

O [loader nativo offline](LOADER.md) empacota o runtime em um único executável,
valida licença e manifesto assinados e instala uma versão local antes de chamar
o injetor para um PID específico. A atualização é manual. A versão local requer
validação em partida e em outro PC antes de distribuição comercial.

Para o cliente, distribua `dist/entrega/CLIENTE.zip` e envie a licença por
um canal separado. Não envie emissor, chave privada, diretório build ou símbolos.
O pacote técnico legado contém `injector.exe`, `config.ini`, `kiero-dx11-base.dll`,
`Zb2.AimBridge.dll`, `0Harmony.dll` e `Harmony.LICENSE`; continua disponível para
diagnóstico interno. Símbolos PDB/MAP são preservados para diagnóstico.

Atualizações são manuais. O injetor não exige token GitHub. Configurações e logs do menu permanecem em Documentos/ZB2Menu. NoClip usa N por padrão e permite personalizar a tecla.

Fontes permanecem no repositório privado Menu-ZB2. As prévias antigas com launcher não representam a distribuição atual.
