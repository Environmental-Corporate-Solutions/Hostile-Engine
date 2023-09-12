#version 330 core
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 viewPos;

void main()
{
  FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}