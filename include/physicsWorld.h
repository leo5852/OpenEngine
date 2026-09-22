#ifndef PHYSICSWORLD_HPP
#define PHYSICSWORLD_HPP

// Jolt 헤더는 반드시 Jolt.h를 가장 먼저 include해야 한다
#include <Jolt/Jolt.h>

#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

// 오브젝트 레이어: 무엇과 무엇이 충돌할지 결정한다
namespace Layers {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;  // 움직이지 않는 것 (바닥, 벽)
    static constexpr JPH::ObjectLayer MOVING = 1;      // 움직이는 것
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}

// 브로드페이즈 레이어: 레이어마다 별도의 AABB 트리가 생긴다.
// 정적 물체를 따로 묶어두면 매 프레임 그 트리를 갱신하지 않아도 된다
namespace BroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint NUM_LAYERS(2);
}

// 오브젝트 레이어 -> 브로드페이즈 레이어 매핑
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface 
{
    public:
        // 생성자-배열 초기화
        BPLayerInterfaceImpl() 
        {
            objectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            objectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }

        virtual JPH::uint GetNumBroadPhaseLayers() const override 
        {
            return BroadPhaseLayers::NUM_LAYERS;
        }

        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
            JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
            return objectToBroadPhase[inLayer];
        }

        #if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        // 프로파일러가 켜진 빌드에서는 디버깅용으로 BP레이어 이름을 반환하는 기능 필요(Jolt 기본값이 ON)
        virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
            switch ((JPH::BroadPhaseLayer::Type)inLayer) {
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:     return "MOVING";
            default: JPH_ASSERT(false); return "INVALID";
            }
        }
        #endif

    private:
        // object의 레이어 index를 통해 BP레이어 찾기 위한 배열
        JPH::BroadPhaseLayer objectToBroadPhase[Layers::NUM_LAYERS];
};

// 오브젝트 레이어가 어떤 브로드페이즈 트리를 검사할지 결정한다
class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
        switch (inLayer1) {
        case Layers::NON_MOVING: return inLayer2 == BroadPhaseLayers::MOVING; // 정적 물체는 움직이는 것만 검사
        case Layers::MOVING:     return true;
        default: JPH_ASSERT(false); return false;
        }
    }
};

// 실제로 두 오브젝트 레이어가 충돌하는지 결정한다
class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
        switch (inObject1) {
        case Layers::NON_MOVING: return inObject2 == Layers::MOVING; // 정적끼리는 검사하지 않는다
        case Layers::MOVING:     return true;
        default: JPH_ASSERT(false); return false;
        }
    }
};

// Jolt 물리 세계의 초기화/갱신/정리를 한 곳에 모아둔 래퍼
class PhysicsWorld {
public:
    ~PhysicsWorld() { shutdown(); }

    void init();
    void shutdown();

    // dt를 누적해서 60Hz 고정 스텝으로 물리를 진행시킨다
    void update(float dt);

    // 정적 물체를 다 만든 뒤 한 번 호출하면 브로드페이즈 트리가 최적화된다
    void optimizeBroadPhase();

    JPH::BodyInterface& getBodyInterface();
    JPH::PhysicsSystem& getSystem() { return *physicsSystem; }

private:
    JPH::PhysicsSystem* physicsSystem = nullptr;
    JPH::TempAllocatorImpl* tempAllocator = nullptr;
    JPH::JobSystemThreadPool* jobSystem = nullptr;

    BPLayerInterfaceImpl bpLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseFilter;
    ObjectLayerPairFilterImpl objectLayerPairFilter;

    float accumulator = 0.0f;
    bool initialized = false;
};

#endif
