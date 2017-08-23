#version 330

uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform mat4 projMatrix;

in vec3 vertPos;
out vec4 viewVector;

void main() {
    //gl_Position = vec4(vertPos, 1.f);
    viewVector = viewMatrix[0];
	gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(vertPos, 1.0);
}
