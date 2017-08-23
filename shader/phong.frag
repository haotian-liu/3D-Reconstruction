#version 330

in vec4 viewVector;
out vec4 FragColor;

void main() {
    FragColor = vec4(vec3(viewVector), 1.f);
	//FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
