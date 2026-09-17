#ifndef GAMEOBJECT_HPP
#define GAMEOBJECT_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "collider.h"

class GameObject {
public:
    static constexpr float GRAVITY = 9.8f;
    static constexpr float GROUND_NORMAL_MIN_Y = 0.7f; // 충돌 법선의 y가 이보다 크면(경사 약 45도 이하) 바닥으로 취급

    glm::vec3 position;
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // (w, x, y, z) 항등 회전. 렌더링과 콜라이더(OBB)가 모두 이 값을 따른다
    Collider* collider;
    bool isStatic;

    bool useGravity = false;               // true면 매 프레임 중력이 velocity에 누적된다
    glm::vec3 velocity = glm::vec3(0.0f);
    bool isGrounded = false;               // 직전 프레임에 바닥(위쪽 방향 mtv)과 충돌했는지

    // 회전 물리: 충돌 지점에 충격량을 받으면 각속도가 생겨 기울어지거나 넘어진다
    glm::vec3 angularVelocity = glm::vec3(0.0f); // 월드 기준 회전축 방향 * 초당 회전량(rad/s)
    bool freezeRotation = false;                 // true면 충돌해도 회전하지 않는다 (예: Player)
    float mass = 1.0f;
    float friction = 0.5f;                       // 마찰 계수. 두 물체 값을 곱한 뒤 제곱근을 쓴다

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

    // 중력을 velocity에 적용하고 velocity/angularVelocity만큼 이동·회전시킨다.
    // isGrounded는 여기서 일단 false로 리셋되고, 이번 프레임에 바닥과 충돌하면 onCollision에서 다시 true가 된다.
    virtual void update(float dt) {
        isGrounded = false;
        if (isStatic) return;

        if (useGravity)
            velocity.y -= GRAVITY * dt;
        position += velocity * dt;

        // 각속도는 월드 기준이므로 기존 회전의 왼쪽에 곱한다
        float angularSpeed = glm::length(angularVelocity);
        if (!freezeRotation && angularSpeed > 1e-6f) {
            glm::quat delta = glm::angleAxis(angularSpeed * dt, angularVelocity / angularSpeed);
            rotation = glm::normalize(delta * rotation);
        }
    }

    // 겹친 만큼 위치만 밀어낸다. 속도/회전 반응은 CollisionSystem이 접촉점 충격량으로 처리한다.
    virtual void onCollision(glm::vec3 mtv) {
        position += mtv;

        // mtv 방향 = 충돌면 법선. 충분히 위를 향하면 바닥으로 취급
        float len = glm::length(mtv);
        if (useGravity && len > 0.0f && mtv.y / len > GROUND_NORMAL_MIN_Y)
            isGrounded = true;
    }
};


#endif 