# Silhueta e autoridade de rede

## Silhueta

O controle **VISUAL → Silhueta → Contorno do corpo** é independente do Chams.
Permite cores visível/oculta e espessura de 1 a 6 pixels, salvas no preset.
O interior do modelo permanece transparente. O contorno considera a união dos
corpos: inimigos sobrepostos podem compartilhar uma borda externa.

A thread do Unity desenha os modelos vivos em uma máscara RGBA, usando o depth
buffer da câmera sem limpá-lo ou escrevê-lo. Vermelho identifica fragmentos visíveis;
verde, ocultos. Um pixel shader D3D11 encontra a borda externa e aplica as cores
escolhidas. Não troca materiais do jogo nem usa chamadas Unity dentro de Present.
O ponteiro da textura é obtido somente na criação/recriação; o consumidor nativo
retém a view COM enquanto desenha. Ao desligar, sair de cena ou trocar a câmera,
o command buffer e as texturas do efeito são removidos.

Esta versão suporta a câmera principal no pipeline Built-in D3D11, sem MSAA,
VR ou targetTexture externo. Uma configuração incompatível gera diagnóstico;
o menu não muda a qualidade gráfica do usuário. Limite de 128 modelos ativos
e 8 submeshes por modelo por ciclo, consistente com o Chams. Entidades sem modelo
carregado não têm uma silhueta que possa ser desenhada.

### Evidência e pendência

- Build gerenciado contra o assembly do jogo e build nativo Release x64.
- Teste com shader real compilado e executado no dispositivo D3D11 WARP:
  bordas visíveis/ocultas com cores distintas e interior/fundo transparentes.
- Testes de ciclo de vida: desligamento, resize, troca de câmera, morte,
  ausência de câmera e rejeição de MSAA. Esses usam stubs Unity.
- A integração do depth buffer e a orientação da máscara **ainda precisam
  de validação em partida**, inclusive com parede, resize e Chams simultâneo.
  Compilar e passar no WARP não prova essa integração.

## Magnet sem ser host

O usuário confirmou que o amigo concorda com o teste, mas somente o computador
do usuário executará o menu. O consentimento não muda quem simula cada entidade.

Na revisão do assembly SHA256
`c41a298975d35f0dad0a05531bce6e0b6e274d0ddf265217d65ce3ac5cbc84e1`:

- `ServerListener.TreatZombieTeleport` chama `LogUnexpectedPackage`.
  Enviar esse teleporte do cliente não fornece um caminho aceito pelo servidor.
- `Zombie.TeleportTo` e `ServerSpeaker.BroadcastZombieTeleport` pertencem ao
  caminho autoritativo já usado no solo/host.
- O jogador local processa seu próprio movimento; `ReceivePosition` alimenta
  representações remotas. Os teleportes de helicóptero/vendedor auditados
  atuam em `MyPlayer`, não em um jogador remoto escolhido pelo chamador.
- A rota de saúde auditada sincroniza vivo/caído/morto, não HP numérico.

Não foi demonstrado outro mecanismo nativo para Magnet de amigos ou para mover
loot/inimigos sincronizados por um cliente comum. Não foi implementado um bypass,
não foram enviados pacotes experimentais e não se removeu a checagem de host para
exibir sucesso local falso. Essas funções continuam pendentes no cenário pedido.

No FiveM, OneSync e ownership determinam quem controla as entidades. Isso explica
o desenho da função, mas não fornece uma API transferível para ZB2. A proposta de
interface continua válida: lista de participantes, escolher alvo, trazer uma vez,
acompanhar ao lado e soltar. Sua execução exige um caminho de rede confirmado.

## Referências

- [Cfx: OneSync](https://docs.fivem.net/docs/scripting-reference/onesync/)
- [Cfx: IDs locais e de rede](https://docs.fivem.net/docs/scripting-manual/networking/ids/)
- [Unity: CommandBuffer.SetRenderTarget](https://docs.unity3d.com/ScriptReference/Rendering.CommandBuffer.SetRenderTarget.html)
- [Unity: Texture.GetNativeTexturePtr](https://docs.unity3d.com/ScriptReference/Texture.GetNativeTexturePtr.html)
