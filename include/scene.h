#ifndef SCENE_HPP
#define SCENE_HPP

#include <vector>
#include <memory>
#include <algorithm>
#include <type_traits>
#include <utility>

#include "renderableObject.h"
#include "physicsWorld.h"

// 렌더링되는 월드 오브젝트(Cube, Plane 등)의 소유, Jolt body 생성/해제, draw를 한 곳에서 관리
// spawn()으로 한 번에 생성하도록
class Scene {
public:
    Scene(PhysicsWorld& physicsWorld) : physicsWorld(physicsWorld) {}

    // T는 RenderableObject를 상속해야 한다. 생성자 인자를 그대로 전달한다.
    template<typename T, typename... Args>
    T& spawn(Args&&... args) {
        static_assert(std::is_base_of<RenderableObject, T>::value, "Scene::spawn requires a RenderableObject");

        auto obj = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *obj;

        // Jolt body는 여기가 아니라 다음 updatePhysics()에서 만든다 (spawn 후 설정이 끝난 상태로 만들기 위해)
        objects.push_back(std::move(obj));
        return ref;
    }

    // Jolt body 제거 + 소유권 해제(소멸자에서 GL 리소스도 함께 정리됨)
    void destroy(RenderableObject* obj) {
        if (obj->hasBody()) {
            physicsWorld.removeBody(obj->bodyID);
            obj->bodyID = JPH::BodyID();
        }

        objects.erase(std::remove_if(objects.begin(), objects.end(),
            [obj](const std::unique_ptr<RenderableObject>& owned) { return owned.get() == obj; }),
            objects.end());
    }

    void update(float dt) {
        for (auto& obj : objects) {
            obj->update(dt);
        }
    }

    // 물리 한 스텝: body 생성 -> Jolt 진행 -> 결과 반영
    void updatePhysics(float dt) {
        // 1. 아직 body가 없는 오브젝트에 body를 만든다.
        //    spawn 직후가 아니라 여기서 만드는 이유: spawn한 뒤에 isStatic/translate/rotate/scale을
        //    설정하므로, 그 설정이 모두 끝난 상태로 만들어야 한다
        bool addedStatic = false;
        for (auto& obj : objects) {
            if (obj->collider && !obj->hasBody()) {
                obj->bodyID = physicsWorld.addBody(*obj);
                if (obj->hasBody() && obj->isStatic) addedStatic = true;
            }
        }
        // 정적 물체가 새로 들어왔을 때만 브로드페이즈 트리를 다시 최적화한다
        if (addedStatic) physicsWorld.optimizeBroadPhase();

        // 2. Jolt 진행
        physicsWorld.update(dt);

        // 3. 움직이는 물체의 결과를 position/rotation에 반영 (렌더링이 이 값을 쓴다)
        for (auto& obj : objects) {
            if (obj->hasBody() && !obj->isStatic)
                physicsWorld.getTransform(obj->bodyID, obj->position, obj->rotation);
        }
    }

    // 물리 세계를 정리하기 전에 호출해서 body를 모두 제거한다
    void removeAllBodies() {
        for (auto& obj : objects) {
            if (obj->hasBody()) {
                physicsWorld.removeBody(obj->bodyID);
                obj->bodyID = JPH::BodyID();
            }
        }
    }

    void draw() {
        for (auto& obj : objects) {
            obj->draw();
        }
    }

private:
    PhysicsWorld& physicsWorld;
    std::vector<std::unique_ptr<RenderableObject>> objects;
};

#endif
