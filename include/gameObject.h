#ifndef GAMEOBJECT_HPP
#define GAMEOBJECT_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include "collider.h"

class GameObject {
public:
    glm::vec3 position;
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // (w, x, y, z) 항등 회전. 렌더링과 Jolt body가 모두 이 값을 따른다
    Collider* collider;   // 물리 모양 설명서. 없으면 Jolt body를 만들지 않는다
    bool isStatic;        // true면 Jolt의 정적(Static) body, false면 동적(Dynamic) body가 된다

    // Jolt body를 만들 때 쓰는 값들 (body가 만들어진 뒤에 바꿔도 반영되지 않는다)
    bool useGravity = false;   // false면 동적 body라도 중력을 받지 않는다
    float mass = 1.0f;         // 동적 body의 질량(kg)
    float friction = 0.5f;     // 마찰 계수. Jolt가 두 물체 값을 곱한 뒤 제곱근을 쓴다

    // Jolt body. 유효하면 이 오브젝트의 위치/회전은 Jolt가 결정한다 (Scene이 생성/해제)
    // body가 만들어진 뒤에는 translate/rotate로 바꿔도 Jolt에는 반영되지 않는다
    JPH::BodyID bodyID;
    bool hasBody() const { return !bodyID.IsInvalid(); }

    GameObject() : position(0.0f), collider(nullptr), isStatic(false) {};

    virtual ~GameObject() {
        if(collider)
            delete collider;
    }

    // set the collider of this GameObject
    void setCollider(Collider* c) {
        if(collider)
            delete collider;
        this->collider = c;
    }

    // 매 프레임 호출되는 게임 로직용 훅. 이동/충돌은 Jolt가 담당한다
    virtual void update(float dt) {}
};


#endif
