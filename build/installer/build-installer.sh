#!/usr/bin/env bash
# Gera o setup.exe a partir de uma pasta "stage" (exe + DLLs + assets).   Uso: build/installer/build-installer.sh STAGE SAIDA.exe VERSAO 64|32
# Linux: NSIS (apt install nsis / pacman -S nsis) ou, sem ele, Inno Setup via wine (ISCC_EXE=...).  Windows (MSYS2): ISCC no PATH ou ISCC_EXE.
set -euo pipefail
STAGE="$(cd "$1" && pwd)"; OUTF="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"; VER="$3"; BITS="${4:-64}"
cd "$(dirname "$0")"
if [ "${INSTALLER:-}" = nsis ] || { [ -z "${ISCC_EXE:-}" ] && ! command -v iscc >/dev/null && ! command -v wine >/dev/null && command -v makensis >/dev/null; } || { [ "${INSTALLER:-}" != inno ] && command -v makensis >/dev/null; }; then
  command -v makensis >/dev/null || { echo "ERRO: instale NSIS (Debian/Ubuntu: apt install nsis | Arch: pacman -S nsis)"; exit 1; }
  makensis -V2 "-DAppVersion=$VER" "-DStage=$STAGE" "-DOutExe=$OUTF" blockland.nsi; exit 0
fi
ISCC="${ISCC_EXE:-}"; D32=(); [ "$BITS" = 32 ] && D32=("/DArch32=1")
OD="$(dirname "$OUTF")"; ON="$(basename "$OUTF" .exe)"
if [ -z "$ISCC" ]; then command -v iscc >/dev/null && ISCC=iscc || ISCC="$HOME/.wine/drive_c/Program Files (x86)/Inno Setup 6/ISCC.exe"; fi
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) "$ISCC" "/DAppVersion=$VER" "/DStage=$(cygpath -w "$STAGE")" "/DOutDir=$(cygpath -w "$OD")" "/DOutName=$ON" "${D32[@]}" blockland.iss ;;
  *) command -v wine >/dev/null || { echo "ERRO: instale NSIS (recomendado) ou wine + Inno Setup 6 (veja build/installer/README.md)"; exit 1; }
     wine "$ISCC" "/DAppVersion=$VER" "/DStage=Z:$STAGE" "/DOutDir=Z:$OD" "/DOutName=$ON" "${D32[@]}" blockland.iss ;;
esac
