#ifndef COLLIDER_HPP
#define COLLIDER_HPP

#include <glm/glm.hpp>

enum class ColliderType {Box, Sphere};

class Collider {
public:
    ColliderType type;
    glm::vec3 offset; // collider offset from object center position

    Collider(ColliderType t) : type(t), offset(0.0f) {}
    virtual ~Collider() {}

    // for visualize collsion bounds for debugging
    // virtual void DebugDraw() = 0;
};

struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    static bool checkCollision(const AABB& box1, const AABB& box2) {
        return (box1.min.x <= box2.max.x && box1.max.x >= box2.min.x) &&
               (box1.min.y <= box2.max.y && box1.max.y >= box2.min.y) &&
               (box1.min.z <= box2.max.z && box1.max.z >= box2.min.z);
    }
};

// Oriented Bounding Box
struct OBB {
    glm::vec3 center;
    glm::vec3 axes[3];  // 박스의 로컬 x/y/z 축을 월드 공간으로 옮긴 단위 벡터
    glm::vec3 halfSize;

    // 박스를 axis(단위 벡터) 위에 투영했을 때 중심~끝 길이
    float projectRadius(const glm::vec3& axis) const {
        return halfSize.x * glm::abs(glm::dot(axes[0], axis)) +
               halfSize.y * glm::abs(glm::dot(axes[1], axis)) +
               halfSize.z * glm::abs(glm::dot(axes[2], axis));
    }

    // i(0~7)번 꼭짓점. i의 각 비트가 x/y/z 방향의 부호를 정한다
    glm::vec3 corner(int i) const {
        return center + axes[0] * ((i & 1) ? halfSize.x : -halfSize.x)
                      + axes[1] * ((i & 2) ? halfSize.y : -halfSize.y)
                      + axes[2] * ((i & 4) ? halfSize.z : -halfSize.z);
    }

    // point가 박스 안에 있는지 (경계 바깥 margin까지 포함)
    bool contains(const glm::vec3& point, float margin) const {
        glm::vec3 d = point - center;
        for (int i = 0; i < 3; i++)
            if (glm::abs(glm::dot(d, axes[i])) > halfSize[i] + margin) return false;
        return true;
    }

    // dir 방향으로 가장 튀어나온 지점. 면/모서리가 dir과 수직이면 그 면/모서리의 중심을 돌려준다
    glm::vec3 supportPoint(const glm::vec3& dir) const {
        glm::vec3 p = center;
        for (int i = 0; i < 3; i++) {
            float d = glm::dot(axes[i], dir);
            if (d > 1e-4f)       p += axes[i] * halfSize[i];
            else if (d < -1e-4f) p -= axes[i] * halfSize[i];
        }
        return p;
    }
};

#endif 