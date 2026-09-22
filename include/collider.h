#ifndef COLLIDER_HPP
#define COLLIDER_HPP

#include <glm/glm.hpp>

enum class ColliderType {Box, Sphere};

// 오브젝트의 물리 모양 설명서. 충돌 계산 자체는 Jolt가 하고,
// PhysicsWorld::addBody()가 이 정보로 Jolt 모양(Shape)을 만든다
class Collider {
public:
    ColliderType type;
    glm::vec3 offset; // collider offset from object center position

    Collider(ColliderType t) : type(t), offset(0.0f) {}
    virtual ~Collider() {}

    // for visualize collsion bounds for debugging
    // virtual void DebugDraw() = 0;
};

#endif
