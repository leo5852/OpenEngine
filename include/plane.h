#ifndef PLANE_HPP
#define PLANE_HPP

#include <vector>
#include <glm/glm.hpp>

#include "renderableObject.h"
#include "boxCollider.h"

class Plane : public RenderableObject {
public:
    Plane(unsigned int programID);
    void scale(glm::vec3 factor);

private:
    void colorPlane(std::vector<glm::vec4>& points, std::vector<glm::vec4>& colors, std::vector<glm::vec3>& normals);
};

#endif
