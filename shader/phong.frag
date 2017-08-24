#version 330

uniform vec3 LightDirection;

in vec3 worldCoord;
in vec3 eyeCoord;
in vec3 Normal;

in vec4 color;

out vec4 FragColor;

void main() {
    float Shininess = 1.f;
    float Strength = 1.f;

    vec3 KaColor = vec3(color);
    vec3 KdColor = vec3(0.8f);
    vec3 KsColor = vec3(0.8f);

    vec3 N = Normal;
    vec3 L = normalize(LightDirection - worldCoord);
    vec3 R = reflect(-L, N);
    vec3 E = normalize(eyeCoord);

    float NdotL = abs(dot(N, L));
    float EdotR = dot(-E, R);

    float diffuse = max(NdotL, 0.f);
    //float specular = max(pow(EdotR, Shininess), 0.f);
    float specular = 0.f;

    FragColor = vec4(KaColor + KdColor * diffuse + KsColor * specular, 1.f);
}
