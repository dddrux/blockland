# Opcoes que aceleram a compilacao (usado por build-linux.sh e build-windows.sh via `source`).
# Define FASTFLAGS (-pipe + linker rapido se houver) e CCACHE (prefixo se ccache estiver instalado).
FASTFLAGS="-pipe"; CCACHE=""
command -v ccache >/dev/null 2>&1 && CCACHE="ccache"
for l in mold lld gold; do   # linker mais rapido que o ld padrao (so se existir)
  if command -v "ld.$l" >/dev/null 2>&1 || { [ "$l" = lld ] && command -v ld.lld >/dev/null 2>&1; }; then FASTFLAGS="$FASTFLAGS -fuse-ld=$l"; break; fi
done
