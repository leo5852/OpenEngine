#include "collisionSystem.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>

static constexpr int SOLVER_ITERATIONS = 10;     // 접촉점 충격량을 반복해서 맞추는 횟수. 많을수록 안정적
static constexpr float CONTACT_MARGIN = 0.005f;  // 표면에서 이만큼 떨어진 꼭짓점까지 접촉으로 본다 (착지 떨림 방지)

void CollisionSystem::registerObject(GameObject* obj){
    if (!obj->collider) {
        std::cout << "CollisionSystem::registerObject - collider가 없는 오브젝트는 등록할 수 없습니다.\n";
        return;
    }
    this->objects.push_back(obj);
}

void CollisionSystem::unregisterObject(GameObject* obj){
    this->objects.erase(std::remove(this->objects.begin(), this->objects.end(), obj), this->objects.end());
}

void CollisionSystem::update(){
    for (size_t i = 0; i < objects.size(); i++) {
        for (size_t j = i + 1; j < objects.size(); j++) {
            GameObject* a = objects[i];
            GameObject* b = objects[j];

            // static끼리는 검사 불필요
            if (a->isStatic && b->isStatic) continue;

            // a를 b로부터 밀어내는 MTV 계산
            auto mtv = getMTV(*a, *b);
            if (!mtv) continue;

            // 1. 속도 반응: 접촉점마다 충격량을 줘서 멈추고, 마찰을 받고, 회전(넘어짐)이 생긴다
            float depth = glm::length(*mtv);
            if (depth > 0.0f) {
                glm::vec3 normal = (*mtv) / depth;
                resolveVelocity(*a, *b, normal, getContactPoints(*a, *b, normal));
            }

            // 2. 위치 보정: 겹친 만큼 밀어냄
            if (!a->isStatic && !b->isStatic) {
                // 둘 다 dynamic: 절반씩 나눠서 밀어냄
                a->onCollision( (*mtv) / 2.0f);
                b->onCollision(-(*mtv) / 2.0f);
            } else if (!a->isStatic) {
                a->onCollision( (*mtv));
            } else {
                b->onCollision(-(*mtv));
            }
        }
    }
}

std::optional<glm::vec3> CollisionSystem::getMTV(GameObject& a, GameObject& b) {
    // Box vs Box Collision detection
    if (a.collider->type == ColliderType::Box && b.collider->type == ColliderType::Box) {
        auto* boxA = static_cast<BoxCollider*>(a.collider);
        auto* boxB = static_cast<BoxCollider*>(b.collider);
        return getBoxVSBoxMTV(*boxA, a.position, a.rotation, *boxB, b.position, b.rotation);
    }
    return std::nullopt;
}

std::optional<glm::vec3> CollisionSystem::getBoxVSBoxMTV(BoxCollider& a, glm::vec3 posA, glm::quat rotA, BoxCollider& b, glm::vec3 posB, glm::quat rotB) {
    // 1. AABB끼리 먼저 검사
    if (!AABB::checkCollision(a.getWorldAABB(posA, rotA), b.getWorldAABB(posB, rotB))) return std::nullopt;

    // 2. AABB검사 통과했으면 SAT검사
    OBB obbA = a.getWorldOBB(posA, rotA);
    OBB obbB = b.getWorldOBB(posB, rotB);

    // b -> a direction
    glm::vec3 dir = obbA.center - obbB.center;

    // 박스 두 개의 후보 분리축 15개: A의 면 법선 3개, B의 면 법선 3개, 모서리끼리의 외적 9개
    // 면 법선을 먼저 넣어서, 겹침량이 같으면 면 법선이 MTV 방향으로 선택되도록 한다
    glm::vec3 axes[15];
    int count = 0;
    for (int i = 0; i < 3; i++) axes[count++] = obbA.axes[i];
    for (int i = 0; i < 3; i++) axes[count++] = obbB.axes[i];
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            axes[count++] = glm::cross(obbA.axes[i], obbB.axes[j]);

    float minOverlap = std::numeric_limits<float>::max();
    glm::vec3 mtvAxis(0.0f);

    for (int k = 0; k < count; k++) {
        // 두 모서리가 평행하면 외적이 0벡터라 축으로 쓸 수 없다
        float lenSq = glm::dot(axes[k], axes[k]);
        if (lenSq < 1e-6f) continue;
        glm::vec3 axis = axes[k] / std::sqrt(lenSq);

        float dist = glm::dot(dir, axis);
        float overlap = obbA.projectRadius(axis) + obbB.projectRadius(axis) - std::abs(dist);

        // 한 축이라도 투영이 겹치지 않으면 분리축이 존재 = 충돌 아님
        if (overlap < 0.0f) return std::nullopt;

        if (overlap < minOverlap) {
            minOverlap = overlap;
            mtvAxis = dist < 0.0f ? -axis : axis;
        }
    }

    // return the MTV (가장 적게 겹친 축 방향으로 밀어냄)
    return mtvAxis * minOverlap;
}

// 접촉점들을 찾아 반환하는 함수. 어디를 밀어내야할지 판단하는데 사용.
std::vector<glm::vec3> CollisionSystem::getContactPoints(GameObject& a, GameObject& b, glm::vec3 normal) {
    std::vector<glm::vec3> points;
    if (a.collider->type != ColliderType::Box || b.collider->type != ColliderType::Box) return points;

    OBB obbA = static_cast<BoxCollider*>(a.collider)->getWorldOBB(a.position, a.rotation);
    OBB obbB = static_cast<BoxCollider*>(b.collider)->getWorldOBB(b.position, b.rotation);

    // 상대 박스 안으로 파고든 꼭짓점이 접촉점 (꼭짓점으로 닿으면 1개, 모서리면 2개, 면이면 4개)
    for (int i = 0; i < 8; i++) {
        glm::vec3 cornerA = obbA.corner(i);
        if (obbB.contains(cornerA, CONTACT_MARGIN)) points.push_back(cornerA);
        glm::vec3 cornerB = obbB.corner(i);
        if (obbA.contains(cornerB, CONTACT_MARGIN)) points.push_back(cornerB);
    }

    // 모서리끼리 엇갈려 닿으면 파고든 꼭짓점이 없다: 서로를 향해 가장 튀어나온 지점의 중간을 쓴다
    if (points.empty())
        points.push_back((obbA.supportPoint(-normal) + obbB.supportPoint(normal)) * 0.5f);

    return points;
}

glm::mat3 CollisionSystem::getInverseInertia(GameObject& obj) {
    if (obj.isStatic || obj.freezeRotation || obj.collider->type != ColliderType::Box)
        return glm::mat3(0.0f);

    // 속이 꽉 찬 직육면체의 관성 모멘트: 축마다 m/12 * (나머지 두 변 길이 제곱의 합)
    glm::vec3 s = static_cast<BoxCollider*>(obj.collider)->size;
    glm::vec3 inertia = obj.mass / 12.0f * glm::vec3(s.y * s.y + s.z * s.z,
                                                     s.x * s.x + s.z * s.z,
                                                     s.x * s.x + s.y * s.y);
    glm::mat3 invLocal(0.0f);
    invLocal[0][0] = 1.0f / inertia.x;
    invLocal[1][1] = 1.0f / inertia.y;
    invLocal[2][2] = 1.0f / inertia.z;

    // 로컬 -> 월드: R * I^-1 * R^T
    glm::mat3 r = glm::mat3_cast(obj.rotation);
    return r * invLocal * glm::transpose(r);
}

// normal은 b -> a 방향. 접촉점마다 normal 방향(밀어내기)과 접선 2방향(마찰)으로 충격량을 준다.
// 충격량이 무게중심에서 벗어난 지점에 걸리면 각속도가 생기므로, 꼭짓점으로 착지한 큐브는 면이 닿을 때까지 넘어간다.
void CollisionSystem::resolveVelocity(GameObject& a, GameObject& b, glm::vec3 normal, const std::vector<glm::vec3>& points) {
    float invMassA = a.isStatic ? 0.0f : 1.0f / a.mass;
    float invMassB = b.isStatic ? 0.0f : 1.0f / b.mass;
    glm::mat3 invInertiaA = getInverseInertia(a);
    glm::mat3 invInertiaB = getInverseInertia(b);
    float friction = std::sqrt(a.friction * b.friction);

    glm::vec3 tangent1 = glm::normalize(glm::cross(normal, std::abs(normal.y) < 0.9f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f)));
    glm::vec3 tangent2 = glm::cross(normal, tangent1);
    glm::vec3 directions[3] = { normal, tangent1, tangent2 };

    struct ContactState {
        glm::vec3 rA, rB;         // 각 물체 중심 -> 접촉점
        float effectiveMass[3];   // 방향별로 충격량 1을 줬을 때 상대속도 변화량의 역수
        float accumulated[3];     // 지금까지 누적된 충격량
    };

    std::vector<ContactState> contacts(points.size());
    for (size_t k = 0; k < points.size(); k++) {
        ContactState& c = contacts[k];
        c.rA = points[k] - a.position;
        c.rB = points[k] - b.position;
        for (int d = 0; d < 3; d++) {
            glm::vec3 rAxD = glm::cross(c.rA, directions[d]);
            glm::vec3 rBxD = glm::cross(c.rB, directions[d]);
            float k_ = invMassA + invMassB + glm::dot(rAxD, invInertiaA * rAxD) + glm::dot(rBxD, invInertiaB * rBxD);
            c.effectiveMass[d] = k_ > 0.0f ? 1.0f / k_ : 0.0f;
            c.accumulated[d] = 0.0f;
        }
    }

    // 한 접촉점의 충격량이 다른 접촉점의 속도를 바꾸므로 여러 번 반복해서 맞춘다
    for (int iter = 0; iter < SOLVER_ITERATIONS; iter++) {
        for (ContactState& c : contacts) {
            for (int d = 0; d < 3; d++) {
                glm::vec3 relVel = (a.velocity + glm::cross(a.angularVelocity, c.rA))
                                 - (b.velocity + glm::cross(b.angularVelocity, c.rB));
                float impulse = -glm::dot(relVel, directions[d]) * c.effectiveMass[d];

                // 누적값 제한: 수직 충격량은 밀어내기만(>= 0), 마찰은 (수직 충격량 * 마찰계수) 이내
                float oldSum = c.accumulated[d];
                if (d == 0) {
                    c.accumulated[d] = std::max(oldSum + impulse, 0.0f);
                } else {
                    float maxFriction = friction * c.accumulated[0];
                    c.accumulated[d] = glm::clamp(oldSum + impulse, -maxFriction, maxFriction);
                }
                impulse = c.accumulated[d] - oldSum;

                glm::vec3 p = directions[d] * impulse;
                a.velocity        += p * invMassA;
                a.angularVelocity += invInertiaA * glm::cross(c.rA, p);
                b.velocity        -= p * invMassB;
                b.angularVelocity -= invInertiaB * glm::cross(c.rB, p);
            }
        }
    }
}