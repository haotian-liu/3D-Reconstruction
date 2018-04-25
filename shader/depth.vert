#version 330
#extension GL_ARB_explicit_attrib_location : require

precision highp float;

uniform mat4 transform;

layout(location = 0) in vec3 vertPos;

void main() {
	gl_Position = transform * vec4(vertPos, 1.0);
}
