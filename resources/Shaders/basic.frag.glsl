#version 330 core

uniform vec3 uForegroundColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(uForegroundColor, 1.0);
}
