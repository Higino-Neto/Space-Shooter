#version 330

in vec3 vertexPosition;

uniform mat4 mvp;

out vec3 fragPosition;

void main()
{
    fragPosition = vertexPosition;

    vec4 position = mvp * vec4(vertexPosition, 1.0);

    gl_Position = position.xyww;
}