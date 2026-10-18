# Instalador Windows (setup.exe)

Gera `dist/setup.exe`: instala se nao existir, atualiza se ja existir (Inno Setup reconhece pelo AppId no registro e reaproveita a pasta anterior).

## Como gerar
1. Compile o jogo: `scripts/build-windows.sh` (ou tudo de uma vez: `TARGET=windows ./build.sh`, ou `scripts/build-windows.sh --installer`).
2. Instale o **Inno Setup 6** (https://jrsoftware.org/isdl.php).
3. `installer/build-installer.sh`
   - **Windows (MSYS2/Git Bash):** `ISCC_EXE="/c/Program Files (x86)/Inno Setup 6/ISCC.exe" installer/build-installer.sh`
   - **Linux (Wine):** uma vez `wine innosetup-6.x.x.exe /VERYSILENT`; depois `installer/build-installer.sh` (usa `~/.wine/drive_c/Program Files (x86)/Inno Setup 6/ISCC.exe`; outro caminho em `ISCC_EXE`).
   - Manual: `ISCC.exe /DAppVersion=0.1.0 /DStage=..\dist\stage installer\blockland.iss` (depois de montar `dist/stage` = exe + DLLs + assets).
A versao vem de `BLOCKLAND_VERSION` em `src/main.cpp`.

## O que faz
- Proximo > Proximo > Concluir; idioma do sistema (PT-BR, EN, ES). Pasta padrao `%LOCALAPPDATA%\Blockland`, sem admin/UAC.
- Atalhos na Area de Trabalho (marcado) e no Menu Iniciar; icone `installer/blockland.ico` no exe, atalhos e desinstalador.
- `unins000.exe` aparece em Configuracoes > Aplicativos / Painel de Controle. Ao desinstalar pergunta se apaga saves e configuracoes (padrao: manter).
- Atualizar NAO sobrescreve `saves/`, `logs/`, `profile.cfg`, `options.cfg`, `gamepad.cfg` e arquivos ja existentes em `assets/resourcepacks/`.

## Alertas do Defender/SmartScreen
Feito: exe com manifesto `asInvoker` (`installer/blockland.manifest`, embutido via windres no `build-windows.sh`), instalacao em `%LOCALAPPDATA%`, compressao padrao LZMA do Inno (sem packer), sem admin.
Definitivo so com **assinatura de codigo** (OV/EV). Com o certificado (.pfx): `signtool sign /f cert.pfx /p SENHA /tr http://timestamp.digicert.com /td sha256 /fd sha256 blockland.exe` antes de gerar o setup, e o mesmo comando em `dist\setup.exe` depois (ou `SignTool=` no `[Setup]`). EV remove o aviso na hora; OV ganha reputacao com os downloads.

## Limite
Nada foi executado aqui (sem Wine/Inno/mingw no ambiente): so conferi sintaxe do shell, XML do manifesto, icone e deteccao de versao. (VIS)

## Jeito mais facil (Windows)
De dois cliques em `installer\gerar-setup.bat`: instala MSYS2 e Inno Setup (via winget) se faltarem, compila o jogo e cria `dist\setup.exe`.
O `setup.exe` e um assistente: Avancar -> Avancar -> Instalar -> Concluir (idioma do sistema, sem admin, atalhos, desinstalador).

## Gerar no Linux (sem Wine)
`scripts/build-tudo-linux.sh` (Debian/Ubuntu) instala g++, SDL2, mingw e NSIS, e gera `./blockland`, `./blockland.exe` e `dist/setup.exe`
(o instalador NSIS em `installer/blockland.nsi` faz o mesmo que o Inno: Avancar -> Instalar -> Concluir, sem admin, atalhos, desinstalador, preserva saves).
