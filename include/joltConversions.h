#ifndef JOLTCONVERSIONS_HPP
#define JOLTCONVERSIONS_HPP

// GLM(렌더링/게임 로직) <-> Jolt(물리) 타입 변환
// 둘 다 오른손 좌표계에 Y-up이라 좌표 변환 없이 타입만 바꿔줌

#include <Jolt/Jolt.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// 위치는 Jolt에서 RVec3를 쓰는데, 단정밀도 빌드(기본값)에서는 RVec3 == Vec3라 이 함수로 충분하다
inline JPH::Vec3 toJolt(const glm::vec3& v) { return JPH::Vec3(v.x, v.y, v.z); }
inline glm::vec3 toGlm(JPH::Vec3Arg v)      { return glm::vec3(v.GetX(), v.GetY(), v.GetZ()); }

// 주의: 쿼터니언 성분 순서가 서로 다르다. GLM 생성자는 (w, x, y, z), Jolt 생성자는 (x, y, z, w)
inline JPH::Quat toJolt(const glm::quat& q) { return JPH::Quat(q.x, q.y, q.z, q.w); }
inline glm::quat toGlm(JPH::QuatArg q)      { return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ()); }

#endif