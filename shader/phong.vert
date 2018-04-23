#version 330

uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform mat4 projMatrix;

in vec3 vertPos;
in vec3 vertNormal;
in vec4 vertColor;

out vec4 color;
out vec3 eyeCoord;
out vec3 Normal;
//out vec3 worldCoord;

void main() {
    vec4 position = vec4(vertPos, 1.0f);

    mat4 modelViewMatrix = viewMatrix * modelMatrix;

//    vec4 worldPos = modelMatrix * position;
//    vec4 eyePos = viewMatrix * worldPos;
    vec4 eyePos = modelViewMatrix * position;
    vec4 clipPos = projMatrix * eyePos;

//    worldCoord = worldPos.xyz;
    eyeCoord = eyePos.xyz;

	Normal = mat3(modelViewMatrix) * vertNormal;
	color = vertColor;

    gl_Position = clipPos;
}
