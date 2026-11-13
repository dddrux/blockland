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

static float lerpf(float a, float b, float t) { return a + (b - a) * t; }

