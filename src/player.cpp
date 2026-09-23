#include "player.h"
#include "physicsWorld.h"
#include "joltConversions.h"

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

// 발바닥이 원점이 되도록 캡슐을 위로 올려둔 모양을 만든다. scale은 inner body용으로 조금 작게 만들 때 쓴다
static JPH::RefConst<JPH::Shape> makeCharacterShape(float scale) {
    float radius = Player::RADIUS * scale;
    float halfCylinder = 0.5f * Player::HEIGHT * scale - radius; // 캡슐 = 원기둥 + 위아래 반구
    return new JPH::RotatedTranslatedShape(JPH::Vec3(0.0f, 0.5f * Player::HEIGHT, 0.0f), JPH::Quat::sIdentity(),
                                           new JPH::CapsuleShape(halfCylinder, radius));
}

Player::Player() {
    position = glm::vec3(0.0f, 1.0f, 3.0f);
}

Player::Player(glm::vec3 pos){
    setPos(pos);
}

void Player::createCharacter(PhysicsWorld& world) {
    JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
    settings->mShape = makeCharacterShape(1.0f);
    settings->mMaxSlopeAngle = JPH::DegreesToRadians(maxSlopeAngle); // 등반 가능 각도 설정
    settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -RADIUS);  // 캡슐 아래쪽 반구에 닿도록 설정

    // inner body: 다른 물체들이 캐릭터와 부딪힐 수 있게 해주는 몸체.
    // CharacterVirtual 자체는 Jolt 세계에 등록되지 않아서, 이게 없으면 떨어지는 큐브가 플레이어를 통과한다.
    // 캐릭터 자신의 충돌이 먼저 처리되도록 조금 작게 만든다 (Jolt 샘플과 같은 방식)
    settings->mInnerBodyShape = makeCharacterShape(0.9f);
    settings->mInnerBodyLayer = Layers::MOVING;

    character = new JPH::CharacterVirtual(settings, toJolt(position), JPH::Quat::sIdentity(), 0, &world.getSystem());
}

void Player::destroyCharacter() {
    character = nullptr;
}

void Player::setMoveInput(glm::vec3 direction) {
    moveInput = direction;
}

void Player::jump() {
    jumpRequested = true;
}

bool Player::isOnGround() const {
    return character && character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;
}

// Jolt 샘플(CharacterVirtualTest)의 입력 처리 방식을 따른다
void Player::updateCharacter(float dt, PhysicsWorld& world) {
    if (!character) return;
    JPH::PhysicsSystem& system = world.getSystem();

    // 1. 입력 -> 속도
    character->UpdateGroundVelocity(); // 움직이는 물체 위에 서 있으면 그 속도를 반영
    JPH::Vec3 up = character->GetUp();
    JPH::Vec3 verticalVelocity = character->GetLinearVelocity().Dot(up) * up;
    JPH::Vec3 groundVelocity = character->GetGroundVelocity();
    bool movingTowardsGround = (verticalVelocity.GetY() - groundVelocity.GetY()) < 0.1f;

    JPH::Vec3 newVelocity;
    if (character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround && movingTowardsGround) {
        // 땅 위: 떨어지던 속도는 버리고 땅과 함께 움직인다
        newVelocity = groundVelocity;
        if (jumpRequested)
            newVelocity += jumpSpeed * up;
    } else {
        // 공중: 수직 속도를 유지한다
        newVelocity = verticalVelocity;
    }
    newVelocity += system.GetGravity() * dt;      // 중력
    newVelocity += toJolt(moveInput * moveSpeed); // 수평 이동 (관성 없이 입력이 곧 속도)
    character->SetLinearVelocity(newVelocity);
    jumpRequested = false;

    // 2. 이동 + 충돌 처리. 계단 오르기, 내리막에서 바닥에 붙어 내려가기, 부딪힌 물체 밀기까지 해준다
    JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
    character->ExtendedUpdate(dt, system.GetGravity(), updateSettings,
        system.GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
        system.GetDefaultLayerFilter(Layers::MOVING),
        {}, {}, world.getTempAllocator());

    // 3. 결과를 position에 반영 (카메라가 이 값을 쓴다)
    position = toGlm(character->GetPosition());
}

void Player::setPos(glm::vec3 pos){
    position = pos;
    if (character)
        character->SetPosition(toJolt(pos));
}

void Player::translate(glm::vec3 vec){
    setPos(position + vec);
}
