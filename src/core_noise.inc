// ---------- ruído ----------
static int64_t WORLD_SEED = 0x5EED1234LL;   // long de 64 bits com sinal, como a seed do Minecraft Java
static inline uint32_t seedMix(uint32_t s) {
    s ^= (uint32_t)(WORLD_SEED ^ (WORLD_SEED >> 32)) + 0x9E3779B9u + (s << 6) + (s >> 2);
    s ^= s >> 16; s *= 0x7feb352du; s ^= s >> 15;
    s *= 0x846ca68bu; s ^= s >> 16;
    return s;
}
static float hash2(int x, int z, uint32_t s = 1337) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)z * 668265263u + seedMix(s) * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return h / 4294967295.0f;
}
static float vnoise(float x, float z) {
    int xi = (int)floorf(x), zi = (int)floorf(z);
    float fx = x - xi, fz = z - zi;
    fx = fx * fx * (3 - 2 * fx);
    fz = fz * fz * (3 - 2 * fz);
    float a = hash2(xi, zi), b = hash2(xi + 1, zi), c = hash2(xi, zi + 1), d = hash2(xi + 1, zi + 1);
    return a + (b - a) * fx + (c - a) * fz + (a - b - c + d) * fx * fz;
}
static float fbm(float x, float z) {
    float s = 0, a = .5f;
    for (int i = 0; i < 4; i++) { s += a * vnoise(x, z); x *= 2; z *= 2; a *= .5f; }
    return s;
}

static float lerpf(float a, float b, float t) { return a + (b - a) * t; }
static float vnoise3(float x, float y, float z, uint32_t s) {
    int xi = (int)floorf(x), yi = (int)floorf(y), zi = (int)floorf(z);
    float fx = x - xi, fy = y - yi, fz = z - zi;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy); fz = fz * fz * (3 - 2 * fz);
    auto h = [&](int a, int b, int c) { return hash2((xi + a) * 31 + (yi + b) * 7919, (zi + c) * 17 + (yi + b) * 104729, s); };
    float x00 = lerpf(h(0,0,0), h(1,0,0), fx), x10 = lerpf(h(0,1,0), h(1,1,0), fx);
    float x01 = lerpf(h(0,0,1), h(1,0,1), fx), x11 = lerpf(h(0,1,1), h(1,1,1), fx);
    return lerpf(lerpf(x00, x10, fy), lerpf(x01, x11, fy), fz);
}

