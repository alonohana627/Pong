#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"
#include "gamelib/noise.glsl"

// The court: a dark background, brighter in the middle, with a faint grid and
// the dashed center net, which glows and pulses slowly.
//   color = background, color2 = net and grid color
//   params.x dash length, params.y net width (canvas units)

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

const float kGridSpacing = 96.0;

void main()
{
    vec2 p = spriteLocal(inUv);

    // Background with a round falloff (x scaled so it is not stretched to 16:9).
    vec2 v = (inUv - 0.5) * vec2(sprite.size.x / sprite.size.y, 1.0);
    float r = length(v) * 1.15;
    vec3 background = sprite.color.rgb * mix(1.35, 0.4, smoothstep(0.15, 1.1, r));

    // A faint grid: distance to the nearest grid line in either direction.
    vec2 cell = abs(fract(p / kGridSpacing + 0.5) - 0.5) * kGridSpacing;
    background += sprite.color2.rgb * 0.045 * coverage(min(cell.x, cell.y) - 0.75);
    background = dither(background, gl_FragCoord.xy, 0.05);

    // The net: rounded dashes repeating down the center line.
    float dash = sprite.params.x;
    float width = sprite.params.y;
    float y = mod(p.y, 2.0 * dash) - dash;
    float net = sdRoundBox(vec2(p.x, y), vec2(width * 0.5, dash * 0.5), width * 0.5);
    float pulse = 0.8 + 0.2 * sin(sprite.time * 2.0 - p.y * 0.012);

    vec4 netBody = vec4(sprite.color2.rgb * pulse, 0.85 * coverage(net));
    vec4 netGlow = vec4(sprite.color2.rgb, 0.22 * pulse * glowFalloff(net, 26.0));
    vec4 color = over(netBody, over(netGlow, vec4(background, 1.0)));
    outColor = vec4(color.rgb, sprite.color.a);
}
