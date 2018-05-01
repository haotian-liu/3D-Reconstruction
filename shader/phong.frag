#version 330

uniform vec3 LightDirection;

//in vec3 worldCoord;
in vec3 eyeCoord;
in vec3 Normal;

in vec4 color;

out vec4 FragColor;

void main() {
//    float Shininess = 1.f;
//    float Strength = 1.f;

    vec3 KaColor = color.xyz * 0.2f;
    vec3 KdColor = color.xyz;
//    vec3 KsColor = vec3(1.f);

    vec3 N = Normal;
    vec3 L = normalize(LightDirection - eyeCoord);
//    vec3 R = reflect(-L, N);
//    vec3 E = normalize(eyeCoord);

    float NdotL = clamp(dot(N, L), 0, 1);
//    float EdotR = clamp(dot(-E, R), 0, 1);

    float diffuse = max(NdotL, 0.f);
//    float specular = max(pow(EdotR, Shininess), 0.f);

//    FragColor = vec4(KaColor + KdColor * diffuse + KsColor * specular, 1.f);
//    FragColor = vec4(KaColor + KdColor * diffuse, 1.f);
    FragColor = color;
//    FragColor = vec4(vec3(dot(Normal, vec3(0.f, 0.f, 1.f))), 1.f);
}
