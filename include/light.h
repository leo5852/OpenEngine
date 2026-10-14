#ifndef LIGHT_HPP
#define LIGHT_HPP

#include <string>
#include <glm/glm.hpp>
#include <shader.h>

// Limit for n of lights in a scene. 
// Injected using Shader::addDefine()
inline constexpr int MAX_LIGHTS = 4;

struct Light{
    glm::vec4 position = glm::vec4{0,5,0,1}; // w=1이면 점광원, w=0이면 방향광(xyz가 방향)
    glm::vec3 color = glm::vec3{1.0f}; // 확산/반사 세기(shader의 L)
};

// 셰이더가 카메라 좌표 기준으로 조명을 계산하므로 올릴 때 view를 곱한다.
// 방향광(w=0)은 평행이동이 적용되지 않아 같은 코드로 처리된다
inline void uploadLight(Shader& shader, const Light& light, const glm::mat4& view, int index) {
    std::string prefix = "Lights[" + std::to_string(index) + "].";
    shader.setUniform((prefix + "Position").c_str(), view * light.position);
    shader.setUniform((prefix + "L").c_str(),        light.color);
}

#endif