#version 330 core

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vColor;
layout(location = 2) in vec3 vNormal;

out vec4 color;
out vec3 fragPos; // view space
out vec3 fragNormal; // world space

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 viewPos = view * model * vPosition;
    gl_Position = projection * viewPos;

    fragPos = viewPos.xyz;

    // use inverse & transposed matrix for normals(affine transformation can affect normal) 
    fragNormal = normalize(mat3(transpose(inverse(model))) * vNormal);

    color = vColor;
}
