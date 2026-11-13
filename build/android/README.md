# Blockland no Android
`./build-android.sh [debug|release] [abis]` na raiz gera `blockland-android-<modo>.apk`. Requisitos: JDK 17, Android SDK (`ANDROID_HOME`), git (rede so na 1a vez).
- **OpenGL**: o jogo usa GL 1.x; no Android o `gl4es` (libGL.so) traduz tudo para GLES2. O SDL cria o contexto GLES2 e o jogo nao muda nenhuma chamada `gl*` (ver `src/android.inc`).
- **Controles**: HUD touch (setas, pular, agachar, camera, chat, pausa, inventario) ligado automaticamente; botao Voltar = Esc; teclado virtual so no chat.
- **Dados**: assets sao extraidos do APK para `Android/data/com.blockland.game/files/` (mundos em `saves/`, logs em `logs/`). Render distance padrao 6.
- Nao testado em aparelho: o gl4es converte os shaders `#version 120` do ciclo dia/noite; se algo ficar errado, veja `logs/latest.log`.
