# data/android — tudo do Android que e do PROJETO (a pasta `android/` e so o projeto Gradle gerado: nao precisa ir no zip)
- `manifest.cfg`  — permissoes/atributos do AndroidManifest.xml (uma linha por alteracao; ver comentarios no arquivo).
- `overlay/`      — (opcional) espelho de `android/app/src/main/`: o que estiver aqui e copiado por cima (ex.: `overlay/AndroidManifest.xml` substitui o manifesto inteiro, `overlay/res/...`, `overlay/java/...`).
- `apply.py`      — chamado por `build/android.sh` depois do `android/setup.sh`.
- `android-galeria/`, `android-voice/` — notas e codigo Java de apoio (galeria nativa de skins, voz).
