#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"

// One seven-segment digit for the score, drawn as rounded capsules with a
// glow - smooth vector shapes at any size. The unlit segments show faintly,
// like an old LED display.
//   bits.x: segments a..g in bits 0..6 (top, top-right, bottom-right, bottom,
//           bottom-left, top-left, middle)
//   color = lit segments, color2 = glow
//   params.x segment thickness, params.y glow width, params.z unlit alpha

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

void main()
{
    vec2 p = spriteLocal(inUv);
    float glow = sprite.params.y;
    vec2 halfSize = sprite.size * 0.5 - glow;
    p.x += p.y * 0.1; // a slight italic slant

    float t = sprite.params.x;
    float hx = halfSize.x - t * 0.5;
    float hy = halfSize.y - t * 0.5;
    float gap = t * 0.65; // segments do not touch at the corners

    vec2 from[7] = vec2[7](vec2(-hx + gap, -hy), vec2(hx, -hy + gap), vec2(hx, gap), vec2(-hx + gap, hy),
                           vec2(-hx, gap), vec2(-hx, -hy + gap), vec2(-hx + gap, 0.0));
    vec2 to[7] = vec2[7](vec2(hx - gap, -hy), vec2(hx, -gap), vec2(hx, hy - gap), vec2(hx - gap, hy),
                         vec2(-hx, hy - gap), vec2(-hx, -gap), vec2(hx - gap, 0.0));

    float lit = 1e6;
    float unlit = 1e6;
    for (int i = 0; i < 7; ++i) {
        float d = sdSegment(p, from[i], to[i], t * 0.5);
        if (((sprite.bits.x >> uint(i)) & 1u) != 0u) {
            lit = min(lit, d);
        } else {
            unlit = min(unlit, d);
        }
    }

    vec4 body = vec4(sprite.color.rgb, sprite.color.a * coverage(lit));
    vec4 ghost = vec4(sprite.color.rgb, sprite.color.a * sprite.params.z * coverage(unlit));
    vec4 halo = vec4(sprite.color2.rgb, sprite.color2.a * glowFalloff(lit, glow));
    outColor = over(body, over(ghost, halo));
}
