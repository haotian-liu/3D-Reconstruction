#version 330
#extension GL_ARB_explicit_attrib_location : require

precision highp float;

uniform mat4 transform;

layout(location = 0) in vec3 vertPos;

//const vec2 f = vec2(525.f, 525.f);
//const vec2 c = vec2(319.5f, 239.5f);
const vec2 f = vec2(1050.f, 1050.f);
const vec2 c = vec2(639.5f, 511.5f);

void main() {
    vec4 g = transform * vec4(vertPos, 1.0);
    vec4 u = vec4(g.x * f.x / g.z, g.y * f.y / g.z, g.z / 4.f + .5f, 1.f);
    u.xy /= c;
	gl_Position = u;
}
