# PIE Web — navegador próprio do Monitor Touch (desktop)

Substitui o Chrome headless/CDP. Nada externo para instalar.

Fluxo: rede (HTTPS pelo SISTEMA) → HTML → CSS → layout → rasterizador por software → textura do monitor (+ JPEG para os outros jogadores).

| Arquivo | Papel |
|---|---|
| `http.inc` | HTTP(S): Windows = WinHTTP (winhttp.dll em runtime); Linux/macOS = libcurl (SDL_LoadObject); reserva = sockets (só http). Cookies, redirects e gzip/br pelo sistema. |
| `html.inc` | Analisador de HTML tolerante + entidades + conversão de texto para a fonte 5x7. |
| `css.inc` | CSS: seletores (tag/.classe/#id/[attr]/>/+/~/:first-child/:not/:nth-child…), cascata, `var(--x)`, `@media` de largura, flex/grid simples. |
| `img.inc` | PNG, JPEG, `data:`. |
| `layout.inc` / `layout2.inc` | Bloco, linha, tabela, flex/grid, formulários, imagens, margens colapsando. |
| `raster.inc` | Retângulos, texto 5x7 (negrito/itálico/sublinhado), imagens. |
| `jpeg.inc` | Decodificador + codificador JPEG próprios (envio da tela aos outros jogadores). |
| `web.inc` | Threads (rede x2, página), histórico, cliques, formulários, teclado, zoom, API `be*` usada por `monitor.inc`. |

Android continua com o WebView (BlocklandActivity.java) — é um motor completo, com JavaScript.

## Limites
- **Sem JavaScript**: sites estáticos / renderizados no servidor funcionam (Wikipedia, DuckDuckGo html, notícias, blogs, docs). YouTube, Google, redes sociais não.
- Fonte bitmap 5x7 em escala inteira; sem WebP/SVG (GIF: só o primeiro quadro); sem `position`/float reais (tratados em fluxo).
- Atalhos: Ctrl +/-/0 e Ctrl+roda = zoom, Alt+←/→ = voltar/avançar, Espaço/PgUp/PgDn/Home/End/setas = rolar, F5 = recarregar, Ctrl+L = endereço.

Testes: `tests/test_web.cpp` (servidor local em `tests/site`, libcurl real, worker completo).
