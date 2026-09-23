#pragma once
#include <iostream>
#include <vector>
#include <glm/glm.hpp>

#include "gameObject.h"
#include <Jolt/Physics/Character/CharacterVirtual.h>

using std::vector;

class PhysicsWorld;

// 1인칭 플레이어. 이동과 충돌은 Jolt의 CharacterVirtual이 담당한다.
// position은 발바닥 위치이고, 카메라는 position + cameraOffset에 있다
class Player: public GameObject {
public:
    Player();
    Player(glm::vec3);

    // PhysicsWorld::init() 이후에 호출, Jolt 캐릭터 생성
    void createCharacter(PhysicsWorld& world);
    
    // PhysicsWorld::shutdown() 전에 호출해야 함
    void destroyCharacter();

    // 이번 프레임에 움직일 수평 방향 (길이 1 이하). 매 프레임 다시 설정
    void setMoveInput(glm::vec3 direction);
    // 점프 요청
    void jump();
    // 입력을 속도로 바꾸고 캐릭터를 이동시킨 뒤 position을 갱신
    void updateCharacter(float dt, PhysicsWorld& world);
    bool isOnGround() const;

    void setPos(glm::vec3);
    void translate(glm::vec3 vec);

    glm::vec3 cameraOffset = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float moveSpeed = 5.0f;
    float rotateSpeed = 2.0f;
    float jumpSpeed = 4.0f;
    
    static constexpr float maxSlopeAngle = 45.0f; // 등바 가능한 각도

    // 캐릭터 모양 
    static constexpr float HEIGHT = 1.0f;
    static constexpr float RADIUS = 0.3f;

private:
    JPH::Ref<JPH::CharacterVirtual> character;
    glm::vec3 moveInput = glm::vec3(0.0f);
    bool jumpRequested = false;
};
