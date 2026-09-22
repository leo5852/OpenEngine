# CMake minimum version requirement
cmake_minimum_required(VERSION 3.10)

# fix encoding to UTF-8 for MSVC compiler
if(MSVC)
    add_compile_options(/utf-8)
endif()

# project info
project(
  OpenEngine
  VERSION 0.1.0
  DESCRIPTION "OpenGL사용 3D엔진 프로젝트"
)
set(CMAKE_CXX_STANDARD 17)

# 1.Build GLFW library(exclude doc and examples)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(dependencies/glfw-3.4)

# 2.GLEW include header files path
include_directories(dependencies/glew-2.1.0/include)
# define GLEW to use as static library
add_definitions(-DGLEW_STATIC)

# 3.include header files path
include_directories(dependencies/glm)
include_directories(include)

# 4.Jolt Physics (git submodule)
# submodule 받지 않은 상태에서 빌드하면 원인을 알기 어려운 에러가 나므로 미리 안내
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/dependencies/JoltPhysics/Build/CMakeLists.txt")
  message(FATAL_ERROR "Jolt submodule이 비어 있습니다. 먼저 실행하세요:\n  git submodule update --init --recursive")
endif()

# 아래 옵션들은 반드시 add_subdirectory 전에 설정해야 적용된다
# - USE_STATIC_MSVC_RUNTIME_LIBRARY: MSVC에서 기본값이 ON(/MT)이라 그대로 두면
#   동적 런타임(/MD)을 쓰는 우리 코드/GLFW와 런타임이 어긋나 링크 에러가 남
# - CPP_RTTI_ENABLED: Jolt는 기본적으로 RTTI 없이(/GR-) 빌드되는데
#   MSVC 기본값은 RTTI 사용(/GR)이라 맞춰줌
# - OVERRIDE_CXX_FLAGS: 기본값 ON이면 Jolt가 CMAKE_CXX_FLAGS_DEBUG/RELEASE를 통째로 덮어쓴다.
#   이 프로젝트는 런타임 라이브러리(/MDd)가 그 플래그 문자열에 들어있어서, 덮어쓰면 함께 사라지고
#   MSVC 기본값인 /MT로 컴파일돼 링크 에러(LNK2038)가 난다
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
set(CPP_RTTI_ENABLED ON CACHE BOOL "" FORCE)
set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "" FORCE)
add_subdirectory(dependencies/JoltPhysics/Build)

# generate executable
add_executable (OpenEngine 
  dependencies/glew-2.1.0/src/glew.c
  src/main.cpp
  src/player.cpp
  src/renderableObject.cpp
  src/cube.cpp
  src/plane.cpp
  src/wireframe.cpp
  src/physicsWorld.cpp

  include/player.h
  include/renderableObject.h
  include/scene.h
  include/cube.h
  include/plane.h
  # include/aabb.h
  include/collider.h
  include/boxCollider.h
  include/sphereCollider.h
  include/gameObject.h
  include/wireframe.h
  include/physicsWorld.h
  include/joltConversions.h
  )

# link libraries
# opengl32는 윈도우 기본 시스템 라이브러리입니다. GLEW는 명시적으로 연결해주는 게 좋습니다.
# Jolt 타깃은 include 경로와 JPH_* define을 PUBLIC으로 내보내므로 링크만 하면 된다
target_link_libraries(OpenEngine glfw opengl32 Jolt)

# 셰이더 파일을 소스 폴더(src)에서 빌드 폴더(build)로 복사
file(COPY ${CMAKE_SOURCE_DIR}/src/vShader.glsl DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/src/fShader.glsl DESTINATION ${CMAKE_BINARY_DIR})