#version 450
// Equivalente ao pipeline fixo do OpenGL + o shader de luz do mundo (ciclo dia/noite, luz empacotada no alfa).
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aCol;
layout(set = 0, binding = 0) uniform U { mat4 mvp; vec4 mvz; vec4 cur; vec4 fogC; vec4 fogP; vec4 alphaP; vec4 sky; vec4 env; vec4 cam; } u;
layout(location = 0) out vec2 vUV;
layout(location = 1) out vec4 vCol;
layout(location = 2) out float vFog;
layout(location = 3) out vec3 vWorld;
vec3 chroma(float c) { float r = floor(c / 64.0); float g = floor((c - r * 64.0) / 8.0); float b = c - r * 64.0 - g * 8.0; return vec3(r, g, b) / 7.0; }
void main() {
    vec4 p = u.mvp * vec4(aPos, 1.0);
    gl_Position = vec4(p.x, -p.y, (p.z + p.w) * 0.5, p.w);   // GL: Y para cima, z em [-w,w] -> Vulkan: Y para baixo, z em [0,w]
    vUV = aUV;
    vWorld = aPos;   // terreno: o vertice ja esta em coordenadas do mundo
    vFog = abs(dot(u.mvz, vec4(aPos, 1.0)));
    int fl = int(u.alphaP.z + 0.5);
    vec4 gc = ((fl & 8) != 0) ? aCol : u.cur;
    vec4 c = gc;
    if (u.sky.x > 0.5) {
        float mode = u.sky.y, scale = u.sky.z, rb = u.sky.w;
        float v = floor(gc.a * 2097152.0 + 0.5);
        float cc = floor(v / 4096.0); v -= cc * 4096.0; if (cc > 511.5) { cc = 509.0; v = 4096.0; }
        float sk4 = floor(v / 64.0);
        float sk = sk4 * 0.25;
        float bl = (v - sk4 * 64.0) * 0.25;
        if (mode > 0.5) sk = min(15.0, sk + 2.0);
        float sv = 0.94 * pow(0.8, 15.0 - sk) * scale;
        float bf = 0.0;
        if (bl > 0.01) bf = 0.06 + 0.94 * pow(0.8, 15.0 - bl);
        vec3 bc = bf * chroma(cc);
        vec3 L = vec3(max(0.06 + sv * rb, bc.r), max(0.06 + sv * rb, bc.g), max(0.06 + sv, bc.b));
        if (bl > 15.5) L = vec3(1.0);
        c = vec4(gc.rgb * L, mode > 0.5 ? 0.62 : 1.0);
    }
    vCol = c;
}
