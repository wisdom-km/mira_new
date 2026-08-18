$input v_normal, v_color0

#include <bgfx_shader.sh>

uniform vec4 u_lightDir;
uniform vec4 u_lightColor;

void main()
{
    vec3 normal = normalize(v_normal);
    vec3 lightDir = normalize(u_lightDir.xyz);
    float wrap = max(dot(normal, lightDir), 0.0);
    float lighting = 0.18 + 0.82 * wrap;
    vec3 color = v_color0.rgb * u_lightColor.rgb * lighting;
    gl_FragColor = vec4(color, v_color0.a);
}
