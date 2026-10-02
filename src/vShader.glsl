#version 330 core

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vColor;
layout(location = 2) in vec3 vNormal;

out vec4 color;
out vec3 fragPos;    // 월드 좌표
out vec3 fragNormal; // 월드 기준 법선

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 worldPos = model * vPosition;
    gl_Position = projection * view * worldPos;

    fragPos = vec3(worldPos);

    // use inverse & transposed matrix for normals(affine transformation could change normal) 
    fragNormal = normalize(mat3(transpose(inverse(model))) * vNormal);

    color = vColor;
}
