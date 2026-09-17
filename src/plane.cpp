#include "plane.h"

glm::vec4 planeVertices[4] = {
    glm::vec4(-0.5f, 0.0f, -0.5f, 1.0f), // 0
    glm::vec4( 0.5f, 0.0f, -0.5f, 1.0f), // 1
    glm::vec4( 0.5f, 0.0f,  0.5f, 1.0f), // 2
    glm::vec4(-0.5f, 0.0f,  0.5f, 1.0f) // 3
    /*
    0---1
    | \ |
    3---2
    */
}; 

//TODO: substitute programID param
Plane::Plane(unsigned int programID){
    isStatic = true;
    // 콜라이더 두께를 1.0으로 잡되, offset을 -size.y/2만큼 내려서 
    // 콜라이더의 윗면은 항상 렌더링되는 평면(position.y)과 정확히 맞닿게 한다.
    setCollider(new BoxCollider(glm::vec3(1.0f, 1.0f, 1.0f)));
    collider->offset = glm::vec3(0.0f, -0.5f, 0.0f);

    std::vector<glm::vec4> points;
    std::vector<glm::vec4> colors;
    points.reserve(6);
    colors.reserve(6);

    colorPlane(points, colors);

    setupMesh(programID, points, colors);
}

void Plane::scale(glm::vec3 factor) {
    this->localMatrix = glm::scale(this->localMatrix, factor);
    static_cast<BoxCollider*>(this->collider)->size *= factor;
}

void Plane::colorPlane(std::vector<glm::vec4>& points, std::vector<glm::vec4>& colors) {
    glm::vec4 grassGreen(0.13f, 0.55f, 0.13f, 1.0f);

    int order[6] = { 0, 2, 1, 0, 3, 2 };
    for (int i : order) {
        points.push_back(planeVertices[i]);
        colors.push_back(grassGreen);
    }
}
