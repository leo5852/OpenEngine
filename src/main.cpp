// ALWAYS INCLUDE GLEW -> GLFW IN ORDER //
#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include <glm/gtx/rotate_vector.hpp>
#include "glm/gtx/transform.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

#include <player.h>
#include <shader.h>
#include <cube.h>
#include <plane.h>
#include <collisionSystem.h>
#include <scene.h>
#include <physicsWorld.h>

#define BORDER_LEFT 0
#define BORDER_RIGHT 1280
#define BORDER_DOWN 0
#define BORDER_UP 720

#define MAP_ROWS 8
#define MAP_COLS 8
#define MAP_SIZE 64

#define CLEAR_COLOR 0.31f, 0.73f, 0.87f, 0.0f
#define CAMERA_SPEED 2.5f

using namespace std;

void init();
void onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods);
void mainLoopEvent();

void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

//////// Global variables ////////
// screen size
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Shader linked program
Shader shader;
// Vertex array object
GLuint vao;
// Player
Player player;
// Collision system
CollisionSystem collisionSystem;
// Jolt 물리 세계 (아직 물체는 등록하지 않고 초기화/정리만 확인하는 단계)
PhysicsWorld physicsWorld;
// Matrix transformation
//GLuint pvmMatrixID; //removed to calculate in shader
glm::mat4 modelMat;
glm::mat4 projectMat;
glm::mat4 viewMat;

float deltaTime = 0.0f; 
float lastFrame = 0.0f;
//////////////////////////////////

////////// MAIN function - program entry point //////////
int main() {
    GLFWwindow* window;
    
    if(!glfwInit()){
        cout << "GLFW init failed";
        return -1;
    }

    window = glfwCreateWindow(1280, 720, "OpenEngine", NULL, NULL);
    if(!window){
        glfwTerminate();
        cout << "Window creation failed";
        return -1;
    }

    glfwMakeContextCurrent(window);

    // load OpenGL function pointers by glew
    glewInit();

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, onKeyEvent);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Load and link shaders into program and use it
    shader.setup("../src/vShader.glsl", "../src/fShader.glsl");
    shader.use();

    init();

    physicsWorld.init();

    //======================= Generate Game Objects =======================
    Scene scene(collisionSystem);

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

    collisionSystem.registerObject(&player); //player는 별개로 취급
    //=====================================================================

    // lastFrame이 0으로 초기화된 채면, 셰이더 컴파일/오브젝트 생성 등 여기까지 걸린 시간이
    // 전부 첫 프레임의 deltaTime으로 들어가서 중력이 한 번에 크게 튀는 문제가 있었다.
    // 루프 진입 직전에 다시 맞춰준다.
    lastFrame = (float)glfwGetTime();

    // The main loop
    while(!glfwWindowShouldClose(window))
    {
        // delta time calculation
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 창을 드래그하거나 브레이크포인트 등으로 프레임이 멈췄다 재개되는 경우를 대비해
        // 한 프레임에 물리가 너무 크게 튀지 않도록 delta time 상한을 둔다.
        const float MAX_DELTA_TIME = 0.1f;
        if (deltaTime > MAX_DELTA_TIME) deltaTime = MAX_DELTA_TIME;

        // 1. proccess inputs
        processInput(window);
        
        // 2. calculate physics and collisions
        player.update(deltaTime);
        scene.update(deltaTime);
        
        collisionSystem.update();
        physicsWorld.update(deltaTime); // 아직 등록된 물체가 없어서 빈 계산만 돈다
        
        // 3. calculate view matrix
        viewMat = glm::lookAt(player.position + player.cameraOffset, 
            player.position + player.cameraOffset + player.cameraFront, 
            player.cameraUp);
            
        // 4. clear the frame and buffer
        glClearColor(CLEAR_COLOR);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
        // 5. set MVP matrices
        // TODO: use fixed location for MVP uniforms and delete these lines
        glUniformMatrix4fv(glGetUniformLocation(shader.programID, "model"), 1, GL_FALSE, &modelMat[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shader.programID, "view"), 1, GL_FALSE, &viewMat[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shader.programID, "projection"), 1, GL_FALSE, &projectMat[0][0]);

        // 6. the actual drawing part
        scene.draw();

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    physicsWorld.shutdown();
    glfwTerminate();
    return 0;
}
////////////////////////////////////////////////////////////

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

// onKeyEvent() is called only once while pressed
// processInput() 
// Keyboard process function
void processInput(GLFWwindow* window){
    float cameraSpeed = player.moveSpeed * deltaTime;
    float rotateSpeed = player.rotateSpeed * deltaTime;
    
    glm::vec3 flatFront = player.cameraFront;
    flatFront.y = 0.0f; 
    flatFront = glm::normalize(flatFront);

    // W
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        player.position += cameraSpeed * flatFront;
    // S
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        player.position -= cameraSpeed * flatFront;
    // A
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        player.position -= cameraSpeed * glm::normalize(glm::cross(flatFront, player.cameraUp));
    // D
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        player.position += cameraSpeed * glm::normalize(glm::cross(flatFront, player.cameraUp));
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
