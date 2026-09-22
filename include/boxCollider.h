#ifndef BOXCOLLIDER_HPP
#define BOXCOLLIDER_HPP

#include "collider.h"

class BoxCollider : public Collider {
public:
    glm::vec3 size; // length, width, height info of box
    BoxCollider(glm::vec3 s) : Collider(ColliderType::Box), size(s) {}
};

#endif
