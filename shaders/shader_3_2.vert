#version 330 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;

uniform mat4 transformation;
out vec3 normal;
out vec2 texCoord;
out vec3 fragPos;

void main()
{
	normal = vertexNormal;
    texCoord = vertexTexCoord;
    fragPos = vertexPosition; // For normal mapping
	gl_Position = transformation * vec4(vertexPosition, 1.0);
}
