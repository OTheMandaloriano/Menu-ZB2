"""Produce explicit client/team delivery ZIPs with no station data or private keys."""
from pathlib import Path
import hashlib
import json
import shutil
import zipfile
ROOT=Path(__file__).resolve().parents[1]
OUTPUT=ROOT/'dist/entrega'
assets=ROOT/'apps/loader/assets'
notices='ZB2 Menu - avisos tecnicos das bibliotecas e fontes\n\n'+'\n\n'.join((assets/name).read_text(encoding='utf-8') for name in
    ['Lexend-OFL.txt','FontAwesome-LICENSE.txt','IconFontCppHeaders-LICENSE.txt','DearImGui-LICENSE.txt'])
bundles={
 'CLIENTE':('loader/ZB2Menu.exe','ZB2Menu.exe',
'''ZB2 MENU - CLIENTE

Voce recebe este pacote e um arquivo .zb2license separado.

1. Extraia esta pasta e abra ZB2Menu.exe.
2. Clique em Copiar ID e envie o ID completo a equipe.
3. Quando receber a licenca, use Abrir arquivo e clique em Ativar.
4. Abra o jogo e entre na partida. AUTO-INJECT aguarda a cena ficar pronta.
   Para acionar manualmente, desligue o toggle e use Injetar agora.
5. No jogo, use INSERT para abrir o menu.

O cliente nao precisa do ZB2 Admin, de uma solicitacao .zb2station ou de uma
autorizacao .zb2issuer. Mesmo um integrante da equipe, se apenas for usar o
menu, recebe este pacote de cliente.

Requisitos: Windows x64 e versao compativel do jogo.
'''),
 'EQUIPE':('admin/ZB2Admin.exe','ZB2Admin.exe',
'''ZB2 ADMIN - EQUIPE EMISSORA

Este pacote e para integrantes que vao GERAR licencas para clientes.
Se voce apenas vai usar o menu no jogo, utilize o pacote CLIENTE.

1. Extraia esta pasta e abra ZB2Admin.exe.
2. Em Minha estacao, informe seu nome e crie uma solicitacao.
3. Envie o arquivo .zb2station ao proprietario.
4. O proprietario define os limites em Equipe e devolve um .zb2issuer.
5. Importe essa autorizacao em Minha estacao.
6. Em Licencas, informe cliente, ID do PC e prazo. Gere e salve a licenca.
7. Envie ao cliente CLIENTE.zip e o arquivo .zb2license dele.

Nunca envie sua pasta de dados nem uma chave .dpapi.
Cada PC tem seu historico local; nao ha sincronizacao automatica offline.
O servico de assinatura fica instalado no perfil Windows, sem janela extra.
Ajuda e instrucoes tambem ficam na aba Ajuda do proprio Admin.
''')}
records={}
for group,(source,name,readme) in bundles.items():
    folder=OUTPUT/group;folder.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(ROOT/'dist'/source,folder/name)
    (folder/'LEIA-ME.txt').write_text(readme,encoding='utf-8')
    (folder/'AVISOS.txt').write_text(notices,encoding='utf-8')
    allowed=[name,'LEIA-ME.txt','AVISOS.txt']
    archive=OUTPUT/(group+'.zip')
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as package:
        for file in allowed:package.write(folder/file,group+'/'+file)
    with zipfile.ZipFile(archive) as package:
        assert sorted(package.namelist())==sorted(group+'/'+item for item in allowed)
        assert package.testzip() is None
    records[group]={'zip':str(archive),'sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'files':allowed}
(ROOT/'build/delivery-manifest.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
print('CLIENTE.zip: Menu + instrucoes + avisos tecnicos.')
print('EQUIPE.zip: Admin + instrucoes + avisos tecnicos.')
print('Licencas e autorizacoes pessoais devem ser enviadas separadamente.')
