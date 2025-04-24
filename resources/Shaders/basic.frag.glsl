#version 330 core
in vec3 fragNormal;
in vec3 fragPos;

out vec4 fragColor;

void main() {
    vec3 lightPos = vec3(10.0, 10.0, 10.0);
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    vec3 objectColor = vec3(0.7, 0.7, 0.9);
    
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    vec3 result = (ambient + diffuse) * objectColor;
    fragColor = vec4(result, 1.0);
}