// ALWAYS INCLUDE GLEW -> GLFW IN ORDER //
#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include <glm/gtx/rotate_vector.hpp>
#include "glm/gtx/transform.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <player.h>
#include <shader.h>
#include <cube.h>
#include <plane.h>
#include <scene.h>
#include <physicsWorld.h>

#define BORDER_LEFT 0
#define BORDER_RIGHT 1920
#define BORDER_DOWN 0
#define BORDER_UP 1080

#define MAP_ROWS 8
#define MAP_COLS 8
#define MAP_SIZE 64

#define CLEAR_COLOR 0.31f, 0.73f, 0.87f, 0.0f
#define CAMERA_SPEED 2.5f

#define MAX_DELTA_TIME 0.1f

using namespace std;

void init();
void onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods);
void mainLoopEvent();
void cleanup(Scene& scene);
void drawDebugUI(Scene& scene);

void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

//////// Global variables ////////
// screen size
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// Shader linked program
Shader shader;
// Vertex array object
GLuint vao;
// Player
Player player;
// Jolt 물리 세계
PhysicsWorld physicsWorld;

// Matrix transformation
//GLuint pvmMatrixID; //removed to calculate in shader
glm::mat4 modelMat;
glm::mat4 projectMat;
glm::mat4 viewMat;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Debug UI (F1으로 토글)
bool showDebugUI = true;
bool showDemoWindow = false;

// Material: 지금은 모든 오브젝트가 공유한다. 디버그 UI에서 조정하므로 전역으로 둔다
glm::vec3 materialKs = glm::vec3(0.5f);
float materialShininess = 128.0f;
//////////////////////////////////

////////// MAIN function - program entry point ////////////
int main() {
    
    if(!glfwInit()){
        cout << "GLFW init failed";
        return -1;
    }
    
    // Create glfw window(openGL context)
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); 
    
    GLFWwindow* window;
    window = glfwCreateWindow((int)(SCR_WIDTH * main_scale), (int)(SCR_HEIGHT * main_scale), "OpenEngine", NULL, NULL);
    
    if(!window){
        glfwTerminate();
        cout << "Window creation failed";
        return -1;
    }

    // set created context current
    glfwMakeContextCurrent(window);
    
    // load OpenGL function pointers by glew
    // * ALWAYS SHOULD BE AFTER setting context current *
    glewInit();

    // setting callback functions
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, onKeyEvent);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // ImGui intialize
    // * ALWAYS SHOULD BE AFTER setting personal callbacks *
    // ImGui가 자기 콜백을 설치하면서 기존 콜백을 기억했다가 이어서 호출해주기 때문에,
    // 순서가 반대면 내가 등록한 콜백들 미실행
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(main_scale); // 고해상도 모니터 대응
    ImGui_ImplGlfw_InitForOpenGL(window, true);  // true = 콜백 설치(체인 유지)
    ImGui_ImplOpenGL3_Init("#version 330");

    // Load and link shaders into program and use it
    shader.addToDefine("MAX_LIGHTS", MAX_LIGHTS); // should be injected before shader.setup()
    // TODO: set different path for release
    shader.setup("../src/vShader.glsl", "../src/fShader.glsl");
    shader.use();

    init();

    // initialize phyicsWorld(uses Jolt)
    physicsWorld.init();
    
    player.createCharacter(physicsWorld);

    //======================= Generate Game Objects =======================
    Scene scene(physicsWorld);
    
    // Light Settings
    // ambient is set once
    scene.setAmbient(glm::vec3(0.2f));
    // individual lights
    scene.addLight({ glm::vec4(3.0f, 5.0f, 0.0f, 1.0f), glm::vec3(0.5f) });

    // TODO: 나중엔 object마다 다른 material 사용 가능하게
    // 값은 전역 materialKs/materialShininess에 있고, UI에서 바뀔 수 있어 매 프레임 올린다

    // spawn & initialize game objects
    scene.spawn<Cube>(shader.programID);

    Cube& cube2 = scene.spawn<Cube>(shader.programID);
    cube2.translate(glm::vec3(1.5f, 0.0f, 0.0f));

    Plane& floor = scene.spawn<Plane>(shader.programID);
    floor.scale(glm::vec3(30.0f, 1.0f, 30.0f));
    floor.translate(glm::vec3(0.0f, -1.0f, 0.0f));

    Cube& fallingCube = scene.spawn<Cube>(shader.programID);
    fallingCube.isStatic = false;
    fallingCube.useGravity = true;
    fallingCube.translate(glm::vec3(-2.0f, 5.0f, 0.0f));
    // 비스듬히 기울여서 떨어뜨림: AABB가 아니라 실제 기울어진 모양(OBB)대로 바닥에 닿는다
    fallingCube.rotate(glm::vec3(1.0f, 1.0f, 1.0f), glm::radians(60.0f));

    Cube& fallingCube2 = scene.spawn<Cube>(shader.programID);
    fallingCube2.isStatic = false;
    fallingCube2.useGravity = true;
    fallingCube2.translate(glm::vec3(-1.0f, 10.0f, 0.0f));
    // 비스듬히 기울여서 떨어뜨림: AABB가 아니라 실제 기울어진 모양(OBB)대로 바닥에 닿는다
    fallingCube2.rotate(glm::vec3(0.0f, 1.0f, 1.0f), glm::radians(45.0f));
    //=====================================================================

    // lastFrame이 0으로 초기화된 채면, 셰이더 컴파일/오브젝트 생성 등 여기까지 걸린 시간이
    // 전부 첫 프레임의 deltaTime으로 들어가서 중력이 한 번에 크게 튀는 문제가 있음.
    // 루프 진입 직전에 다시 맞춰준다.
    lastFrame = (float)glfwGetTime();

    // =========================THE MAIN LOOP=========================
    while(!glfwWindowShouldClose(window))
    {   
        // delta time calculation
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // 창을 드래그하거나 브레이크포인트 등으로 프레임이 멈췄다 재개되는 경우를 대비해
        // 한 프레임에 물리가 너무 크게 튀지 않도록 delta time 상한을 둔다.
        if (deltaTime > MAX_DELTA_TIME) deltaTime = MAX_DELTA_TIME;
        
        glfwPollEvents();

        // ImGui 프레임 시작 (입력 상태가 갱신된 뒤에 호출해야 한다)
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. proccess inputs
        // UI가 키보드를 쓰는 중이면 게임 입력을 막는다 (입력창에 타이핑하면서 플레이어가 움직이는 것 방지)
        if (!ImGui::GetIO().WantCaptureKeyboard)
            processInput(window);
        else
            player.setMoveInput(glm::vec3(0.0f)); // 마지막 입력이 남아 계속 움직이지 않도록
        
        // 2. calculate physics and collisions
        player.updateCharacter(deltaTime, physicsWorld); // 입력 -> 캐릭터 이동/충돌 (Jolt CharacterVirtual)
        scene.updatePhysics(deltaTime);                  // body 생성 -> 물리 진행 -> 결과를 position/rotation에 반영
        scene.update(deltaTime);                         // 오브젝트별 게임 로직 훅
        
        // 3. calculate view matrix
        viewMat = glm::lookAt(player.position + player.cameraOffset, 
            player.position + player.cameraOffset + player.cameraFront, 
            player.cameraUp);
            
        // 4. clear the frame and buffer
        glClearColor(CLEAR_COLOR);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
        // 5-1. set view/projection matrices
        // model 행렬은 오브젝트마다 RenderableObject::draw()에서 따로 설정
        shader.use(); // frame마다 어떤 shader 쓸지 지정해줘야 함(ImGui가 자기꺼로 바꿈)
        shader.setUniform("view", viewMat);
        shader.setUniform("projection", projectMat);

        // 재질 값은 UI에서 바뀔 수 있으므로 매 프레임 올린다
        shader.setUniform("Material.Ks", materialKs);
        shader.setUniform("Material.Shininess", materialShininess);

        // 5-2. 광원 위치를 View Space로 바꿔서 올린다 (LightCount도 함께 설정됨)
        scene.uploadLights(shader, viewMat);

        // 6. the actual drawing part
        scene.draw();

        // 6-2. 디버그 UI는 3D 장면 위에 덮어 그린다.
        // NewFrame을 불렀으면 Render도 반드시 불러야 하므로 토글과 무관하게 호출한다
        if (showDebugUI) 
            drawDebugUI(scene);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // 7. swap the buffers and show on screen!
        glfwSwapBuffers(window);
    }
    // ========================= MAIN LOOP END =========================

    cleanup(scene);
    return 0;
}
//////////////////// MAIN function END ////////////////////

// initialize function
void init(){
    //glfwSetTime(0.0);

    // initialize MVP matrices
    projectMat = glm::perspective(glm::radians(65.0f), 1.0f, 0.1f, 100.0f);
    viewMat = glm::lookAt(player.position + player.cameraOffset, 
            player.position + player.cameraOffset + player.cameraFront, 
            player.cameraUp);
    modelMat = glm::mat4(1.0f);
    
    framebuffer_size_callback(NULL, SCR_WIDTH, SCR_HEIGHT);

    glEnable(GL_DEPTH_TEST);
    glClearColor(CLEAR_COLOR);
}

// Main loop function
void mainLoopEvent(){
    //player.update(deltaTime);
}

// 디버그 패널. 값을 바꾸면 다음 프레임부터 바로 반영된다 (재빌드 불필요)
void drawDebugUI(Scene& scene) {
    ImGui::Begin("Debug (F1)");

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("%.1f FPS  (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
        glm::vec3 ambient = scene.getAmbient();
        if (ImGui::ColorEdit3("Ambient", &ambient[0]))
            scene.setAmbient(ambient);

        std::vector<Light>& lights = scene.getLights();
        for (size_t i = 0; i < lights.size(); i++) {
            ImGui::PushID((int)i);                 // 같은 라벨이 여러 개여도 구분되도록
            ImGui::Text("Light %d", (int)i);
            ImGui::DragFloat3("Position", &lights[i].position[0], 0.1f);
            bool isPoint = lights[i].position.w > 0.5f;
            if (ImGui::Checkbox("Point light (Directional when OFF))", &isPoint))
                lights[i].position.w = isPoint ? 1.0f : 0.0f;
            ImGui::ColorEdit3("Color", &lights[i].color[0]);
            ImGui::PopID();
        }

        if ((int)lights.size() < MAX_LIGHTS && ImGui::Button("Add Light"))
            scene.addLight({ glm::vec4(0.0f, 5.0f, 0.0f, 1.0f), glm::vec3(0.5f) });
    }

    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorEdit3("Ks (specular)", &materialKs[0]);
        ImGui::SliderFloat("Shininess", &materialShininess, 1.0f, 256.0f);
        ImGui::TextDisabled("lower -> wider highlight");
    }

    if (ImGui::CollapsingHeader("Player")) {
        ImGui::Text("position  (%.2f, %.2f, %.2f)", player.position.x, player.position.y, player.position.z);
        ImGui::Text("grounded: %s", player.isOnGround() ? "yes" : "no");
        ImGui::SliderFloat("Move Speed", &player.moveSpeed, 1.0f, 20.0f);
        ImGui::SliderFloat("Jump Speed", &player.jumpSpeed, 1.0f, 15.0f);
    }

    ImGui::Separator();
    ImGui::Checkbox("ImGui Demo Window", &showDemoWindow);
    ImGui::End();

    // imgui_demo.cpp에서 구현된 위젯 사용법 데모
    if (showDemoWindow) ImGui::ShowDemoWindow(&showDemoWindow);
}

void cleanup(Scene& scene){
    // ImGui cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    // physicsworld 정리하기 전에 캐릭터와 body를 먼저 모두 제거한다
    player.destroyCharacter();
    scene.clear();
    physicsWorld.shutdown();
    glfwTerminate();
}

// onKeyEvent() is called only once while pressed
// processInput() 
// Keyboard process function
void processInput(GLFWwindow* window){
    float rotateSpeed = player.rotateSpeed * deltaTime;
    
    glm::vec3 flatFront = player.cameraFront;
    flatFront.y = 0.0f; 
    flatFront = glm::normalize(flatFront);

    // WASD: 이번 프레임에 움직일 방향만 모아서 캐릭터에 넘긴다 (실제 이동과 충돌은 Jolt가 처리)
    glm::vec3 right = glm::normalize(glm::cross(flatFront, player.cameraUp));
    glm::vec3 move(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move += flatFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move -= flatFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move -= right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move += right;
    // 대각선으로 눌러도 vector크기 일정하게 normalize
    if (glm::length(move) > 0.0f) move = glm::normalize(move);
    player.setMoveInput(move);
    // Q
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        player.cameraFront = glm::rotate(flatFront, rotateSpeed, player.cameraUp);
    // E
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
       player. cameraFront = glm::rotate(flatFront, -rotateSpeed, player.cameraUp);
    // SPACE-JUMP
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        player.jump();   
}

// Keyboard event function
void onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods) {
    switch(key){
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;
        case GLFW_KEY_F1:
            if (action == GLFW_PRESS) showDebugUI = !showDebugUI;
            break;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

    if (height == 0) height = 1;

    float ratio = (float)width / (float)height;
    projectMat = glm::perspective(glm::radians(65.0f), ratio, 0.1f, 100.0f);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	;
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	;
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    ;
}


//----------------------------------------------------------------------------
