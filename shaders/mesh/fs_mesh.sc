$input v_normal, v_texcoord0, v_color0

#include <bgfx_shader.sh>

SAMPLER2D(s_tex, 0);
uniform vec4 u_lightDir;
uniform vec4 u_lightColor;
uniform vec4 u_baseColor;

void main()
{
    vec3 normal = normalize(v_normal);
    vec3 lightDir = normalize(u_lightDir.xyz);
    float wrap = max(dot(normal, lightDir), 0.0);
    float lighting = 0.18 + 0.82 * wrap;
    vec4 texel = texture2D(s_tex, v_texcoord0);
    vec4 albedo = texel * v_color0 * u_baseColor;
    vec3 color = albedo.rgb * u_lightColor.rgb * lighting;
    gl_FragColor = vec4(color, albedo.a);
}
