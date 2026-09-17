#ifndef COLLISIONSYSTEM_HPP
#define COLLISIONSYSTEM_HPP

#include <vector>
#include <optional>
#include "glm/glm.hpp"
#include "gameObject.h"
#include "boxCollider.h"

class CollisionSystem {
public:
    void registerObject(GameObject* obj);
    void unregisterObject(GameObject* obj);
    void update();

private:
    std::vector<GameObject*> objects;

    static std::optional<glm::vec3> getMTV(GameObject& a, GameObject& b);
    static std::optional<glm::vec3> getBoxVSBoxMTV(BoxCollider& a, glm::vec3 posA, glm::quat rotA, BoxCollider& b, glm::vec3 posB, glm::quat rotB);
    static std::optional<glm::vec3> getBoxVSSphereMTV(GameObject& a, GameObject& b);

    static std::vector<glm::vec3> getContactPoints(GameObject& a, GameObject& b, glm::vec3 normal);
    static void resolveVelocity(GameObject& a, GameObject& b, glm::vec3 normal, const std::vector<glm::vec3>& points);
    static glm::mat3 getInverseInertia(GameObject& obj);
};

#endif