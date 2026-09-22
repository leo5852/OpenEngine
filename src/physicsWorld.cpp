#include "physicsWorld.h"

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include "gameObject.h"
#include "boxCollider.h"
#include "joltConversions.h"

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <thread>

// Jolt가 유발하는 경고들을 억제한다
JPH_SUPPRESS_WARNINGS

// 물리 세계의 최대 크기. 실제 사용량이 이를 초과하면 Jolt가 경고를 낸다
static constexpr JPH::uint MAX_BODIES = 1024;
static constexpr JPH::uint NUM_BODY_MUTEXES = 0; // 0이면 Jolt가 알아서 정함
static constexpr JPH::uint MAX_BODY_PAIRS = 1024;
static constexpr JPH::uint MAX_CONTACT_CONSTRAINTS = 1024;

// Jolt는 60Hz 고정 스텝에서 안정적으로 동작하도록 설계됨
static constexpr float FIXED_DELTA_TIME = 1.0f / 60.0f;
static constexpr float MAX_ACCUMULATED_TIME = 0.25f; // 프레임이 멈췄다 재개될 때 한 번에 몰아서 계산하는 것 방지
static constexpr int COLLISION_STEPS = 1;

// Jolt 내부 로그를 우리 콘솔로 연결
static void traceImpl(const char* inFMT, ...) {
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);

    std::cout << "[Jolt] " << buffer << "\n";
}

#ifdef JPH_ENABLE_ASSERTS
static bool assertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine) {
    std::cout << "[Jolt] " << inFile << ":" << inLine << ": (" << inExpression << ") "
              << (inMessage != nullptr ? inMessage : "") << "\n";
    return true; // true를 반환하면 디버거에서 멈춘다
}
#endif

void PhysicsWorld::init() {
    if (initialized) return;

    // 1. 메모리 할당자 등록. 다른 Jolt 함수보다 반드시 먼저 호출해야 한다
    JPH::RegisterDefaultAllocator();

    // 2. 로그/assert 콜백 연결
    JPH::Trace = traceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = assertFailedImpl;)

    // 3. 팩토리를 만들고 타입을 등록한다. 충돌 처리 함수들이 이때 등록된다
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    // 4. 물리 계산 중 쓰는 임시 메모리를 미리 확보해 둔다 (매 프레임 할당을 피하려고)
    tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

    // 5. 멀티스레드 작업 큐. 메인 스레드 몫을 빼고 남은 코어를 쓴다
    int numThreads = (int)std::thread::hardware_concurrency() - 1;
    if (numThreads < 1) numThreads = 1;
    jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, numThreads);

    // 6. 물리 시스템 생성. 할당자 등록 뒤에 만들어야 안전해서 포인터로 들고 있는다
    physicsSystem = new JPH::PhysicsSystem();
    physicsSystem->Init(MAX_BODIES, NUM_BODY_MUTEXES, MAX_BODY_PAIRS, MAX_CONTACT_CONSTRAINTS,
                        bpLayerInterface, objectVsBroadPhaseFilter, objectLayerPairFilter);

    initialized = true;
    std::cout << "PhysicsWorld: Jolt 초기화 완료 (작업 스레드 " << numThreads << "개)\n";
}

void PhysicsWorld::update(float dt) {
    if (!initialized) return;

    accumulator += dt;
    if (accumulator > MAX_ACCUMULATED_TIME) accumulator = MAX_ACCUMULATED_TIME;

    while (accumulator >= FIXED_DELTA_TIME) {
        physicsSystem->Update(FIXED_DELTA_TIME, COLLISION_STEPS, tempAllocator, jobSystem);
        accumulator -= FIXED_DELTA_TIME;
    }
}

void PhysicsWorld::optimizeBroadPhase() {
    if (initialized)
        physicsSystem->OptimizeBroadPhase();
}

JPH::BodyInterface& PhysicsWorld::getBodyInterface() {
    return physicsSystem->GetBodyInterface();
}

JPH::BodyID PhysicsWorld::addBody(const GameObject& obj) {
    if (!initialized || !obj.collider || obj.collider->type != ColliderType::Box)
        return JPH::BodyID(); // 아직 Box만 지원 (Sphere 콜라이더는 미구현)

    // 콜라이더 -> Jolt 모양. BoxShape는 전체 크기가 아니라 절반 크기를 받는다
    const BoxCollider& box = static_cast<const BoxCollider&>(*obj.collider);
    JPH::RefConst<JPH::Shape> shape = new JPH::BoxShape(toJolt(box.size * 0.5f));

    // 콜라이더가 오브젝트 중심에서 벗어나 있으면(예: Plane) 모양 자체를 그만큼 옮긴다.
    // 이렇게 하면 body 위치와 GameObject.position이 항상 같은 값이 된다
    if (box.offset != glm::vec3(0.0f))
        shape = new JPH::RotatedTranslatedShape(toJolt(box.offset), JPH::Quat::sIdentity(), shape);

    JPH::BodyCreationSettings settings(shape, toJolt(obj.position), toJolt(obj.rotation),
        obj.isStatic ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic,
        obj.isStatic ? Layers::NON_MOVING : Layers::MOVING);
    settings.mFriction = obj.friction;
    settings.mGravityFactor = obj.useGravity ? 1.0f : 0.0f;

    if (!obj.isStatic) {
        // 질량은 GameObject 값을 쓰고, 관성(얼마나 쉽게 도는지)은 모양에 맞춰 Jolt가 계산한다
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        settings.mMassPropertiesOverride.mMass = obj.mass;
    }

    return physicsSystem->GetBodyInterface().CreateAndAddBody(settings,
        obj.isStatic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
}

void PhysicsWorld::removeBody(JPH::BodyID id) {
    if (!initialized || id.IsInvalid()) return;

    JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
    bodyInterface.RemoveBody(id);
    bodyInterface.DestroyBody(id);
}

void PhysicsWorld::getTransform(JPH::BodyID id, glm::vec3& outPosition, glm::quat& outRotation) {
    JPH::RVec3 position;
    JPH::Quat rotation;
    physicsSystem->GetBodyInterface().GetPositionAndRotation(id, position, rotation);
    outPosition = toGlm(position);
    outRotation = toGlm(rotation);
}

// 생성의 역순으로 정리한다
void PhysicsWorld::shutdown() {
    if (!initialized) return;

    delete physicsSystem;
    physicsSystem = nullptr;
    delete jobSystem;
    jobSystem = nullptr;
    delete tempAllocator;
    tempAllocator = nullptr;

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    initialized = false;
    std::cout << "PhysicsWorld: Jolt 정리 완료\n";
}
