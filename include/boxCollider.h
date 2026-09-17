#ifndef BOXCOLLIDER_HPP
#define BOXCOLLIDER_HPP

#include <glm/gtc/quaternion.hpp>
#include "collider.h"

class BoxCollider : public Collider {
public:
    glm::vec3 size; // length, width, height info of box
    BoxCollider(glm::vec3 s) : Collider(ColliderType::Box), size(s) {}

    // returns collision box in OBB struct form
    // pos: center of object, rot: rotation of object (offset도 함께 회전한다)
    OBB getWorldOBB(glm::vec3 pos, glm::quat rot){
        glm::mat3 r = glm::mat3_cast(rot);
        return { pos + r * this->offset, { r[0], r[1], r[2] }, size / 2.0f };
    }

    // 회전된 박스를 감싸는 AABB. SAT 정밀 검사 전에 빠르게 걸러내는 용도
    AABB getWorldAABB(glm::vec3 pos, glm::quat rot){
        OBB obb = getWorldOBB(pos, rot);
        glm::vec3 extent = glm::abs(obb.axes[0]) * obb.halfSize.x +
                           glm::abs(obb.axes[1]) * obb.halfSize.y +
                           glm::abs(obb.axes[2]) * obb.halfSize.z;
        return {(obb.center - extent), (obb.center + extent)};
    }
};

#endif
