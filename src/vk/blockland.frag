#version 450
layout(set = 0, binding = 0) uniform U { mat4 mvp; vec4 mvz; vec4 cur; vec4 fogC; vec4 fogP; vec4 alphaP; vec4 sky; vec4 env; vec4 cam; } u;
layout(set = 1, binding = 0) uniform sampler2D uTex;
layout(location = 0) in vec2 vUV;
layout(location = 1) in vec4 vCol;
layout(location = 2) in float vFog;
layout(location = 3) in vec3 vWorld;
layout(location = 0) out vec4 oCol;
void main() {
    int fl = int(u.alphaP.z + 0.5);
    vec4 c = vCol;
    if ((fl & 2) != 0) c *= texture(uTex, vUV);
    if ((fl & 4) != 0) {   // alpha test (glAlphaFunc)
        int f = int(u.alphaP.x + 0.5); float r = u.alphaP.y; bool ok = true;
        if (f == 0) ok = false; else if (f == 1) ok = c.a < r; else if (f == 2) ok = abs(c.a - r) < 1e-4; else if (f == 3) ok = c.a <= r;
        else if (f == 4) ok = c.a > r; else if (f == 5) ok = abs(c.a - r) >= 1e-4; else if (f == 6) ok = c.a >= r;
        if (!ok) discard;
    }
    if (u.sky.x > 0.5 && u.sky.y > 0.5) {   // agua: ondas, fresnel, reflexo do ceu e brilho do sol (sem reflexo planar)
        vec3 sun = u.env.xyz; float t = u.env.w;
        vec3 vd = normalize(vWorld - u.cam.xyz);
        float dayK = clamp(clamp(sun.y, -0.2, 1.0) * 4.0 + 0.3, 0.0, 1.0);
        vec2 w = vWorld.xz;
        vec2 g = vec2(cos(w.x * 1.7 + t * 1.1 + w.y * 0.9), cos(w.y * 2.1 - t * 0.9 + w.x * 0.6)) * 0.07;
        g += vec2(cos(w.x * 3.9 - t * 1.7 + w.y * 2.3), cos(w.y * 4.4 + t * 1.5 - w.x * 1.9)) * 0.04;
        g *= 1.0 - smoothstep(16.0, 56.0, vFog);
        vec3 nw = normalize(vec3(-g.x, 1.0, -g.y));
        float fr = 0.05 + 0.95 * pow(1.0 - clamp(dot(-vd, nw), 0.0, 1.0), 4.0);
        vec3 rf = reflect(vd, nw); rf.y = abs(rf.y);
        vec3 zen = u.fogC.rgb * vec3(0.55, 0.75, 1.15);
        vec3 refl = mix(u.fogC.rgb, zen, clamp(rf.y * 1.6, 0.0, 1.0));
        float sd2 = max(dot(rf, sun), 0.0);
        float sp = pow(sd2, 380.0) * 1.4 + pow(sd2, 36.0) * 0.16; sp = sp / (1.0 + sp);
        float sunV = smoothstep(-0.02, 0.12, sun.y);
        vec3 wb = mix(vec3(0.06, 0.27, 0.36) * (0.3 + 0.7 * dayK), c.rgb * vec3(0.8, 1.1, 1.2), 0.3);
        c.rgb = mix(wb, refl, clamp(fr * 1.3 + 0.08, 0.0, 1.0)) + vec3(1.0, 0.82, 0.58) * sp * sunV * (0.35 + 0.65 * fr);
        c.rgb *= 0.94 + 0.12 * (g.x + g.y) * 8.0;
        c.a = clamp(c.a + fr * 0.6 + sp * 0.3, 0.0, 0.96);
    }
    if (u.env.z > 0.5 && u.env.z < 1.5 && !(u.sky.x > 0.5 && u.sky.y > 0.5)) {   // Nether: brilho pulsante de lava/fogo, sombras avermelhadas, luzes alaranjadas
        float lum = dot(c.rgb, vec3(0.299, 0.587, 0.114));
        float em = smoothstep(0.5, 0.9, lum) * clamp((c.r - c.b) * 3.0, 0.0, 1.0);
        float fl = 0.5 + 0.5 * sin(u.env.w * 2.3 + vWorld.x * 0.9 + vWorld.z * 1.1 + vWorld.y * 0.6);
        c.rgb += vec3(1.0, 0.42, 0.08) * em * (0.18 + 0.22 * fl);
        c.rgb *= mix(vec3(1.35, 0.78, 0.62), vec3(1.10, 0.96, 0.88), smoothstep(0.04, 0.45, lum));
    }
    if ((fl & 1) != 0) {   // neblina (glFog): 0 linear, 1 exp, 2 exp2
        float m = u.fogP.w, f;
        if (m < 0.5) f = (u.fogP.y - vFog) / max(u.fogP.y - u.fogP.x, 1e-4);
        else if (m < 1.5) f = exp(-u.fogP.z * vFog);
        else f = exp(-(u.fogP.z * vFog) * (u.fogP.z * vFog));
        if (u.env.z > 0.5 && u.env.z < 1.5) {   // neblina de calor ondulante
            float nz = sin(vWorld.x * 0.09 + u.env.w * 0.12) * sin(vWorld.z * 0.11 - u.env.w * 0.09) * sin(vWorld.y * 0.08 + u.env.w * 0.15) * 0.5 + 0.5;
            vec3 fc = u.fogC.rgb * (0.75 + 0.7 * nz) + vec3(0.10, 0.025, 0.0) * nz;
            c.rgb = mix(c.rgb, fc, (1.0 - exp(-vFog * 0.012)) * 0.22);
        }
        c.rgb = mix(u.fogC.rgb, c.rgb, clamp(f, 0.0, 1.0));
    }
    oCol = c;
}
