#!/usr/bin/env bash
# Gera /tmp/t2/t.jar a partir de tests/jvm/T.java e compara a VM do Blockland com o java real.
set -e; cd "$(dirname "$0")"; rm -rf /tmp/t2; mkdir -p /tmp/t2/src/demo /tmp/t2/out; cp T.java T2.java /tmp/t2/src/demo/
java -m jdk.compiler/com.sun.tools.javac.Main -source 8 -target 8 -nowarn -d /tmp/t2/out /tmp/t2/src/demo/T.java /tmp/t2/src/demo/T2.java 2>&1 | grep -i error || true
(cd /tmp/t2/out && python3 -c "import zipfile,glob;z=zipfile.ZipFile('/tmp/t2/t.jar','w',zipfile.ZIP_DEFLATED);[z.write(f) for f in glob.glob('**/*.class',recursive=True)]")
for C in T T2; do java -cp /tmp/t2/out demo.$C > /tmp/t2/exp_$C.txt; done
g++ -std=c++17 -O1 -Wno-unused -Wno-sign-compare -Wno-misleading-indentation ../test_jvm2.cpp -o /tmp/tj2
for C in T T2; do /tmp/tj2 /tmp/t2/t.jar demo/$C > /tmp/t2/got_$C.txt && diff /tmp/t2/exp_$C.txt /tmp/t2/got_$C.txt && echo "jvm $C: IGUAL ao java real"; done
