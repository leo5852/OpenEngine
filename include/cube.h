#ifndef CUBE_HPP
#define CUBE_HPP

#include <vector>
#include <glm/glm.hpp>

#include "renderableObject.h"
#include "boxCollider.h"

class Cube: public RenderableObject {
public:
    Cube(unsigned int programID, glm::vec3 size = glm::vec3(1.0f));

private:
    void colorcube(std::vector<glm::vec4>& points, std::vector<glm::vec4>& colors, std::vector<glm::vec3>& normals);
    void quad(std::vector<glm::vec4>& points, std::vector<glm::vec4>& colors, std::vector<glm::vec3>& normals,
              int a, int b, int c, int d);
};

#endif
