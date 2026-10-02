#include "renderableObject.h"

RenderableObject::~RenderableObject() {
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
}

// vShader.glsl의 layout(location = ...)과 같은 값.
// glGetAttribLocation을 쓰면, 셰이더가 안 쓰는 속성을 컴파일러가 제거했을 때 -1이 돌아와서
// GLuint로 변환되며 GL 에러가 난다. 위치를 양쪽에 고정해두면 그 문제가 없다
static constexpr GLuint ATTRIB_POSITION = 0;
static constexpr GLuint ATTRIB_COLOR    = 1;
static constexpr GLuint ATTRIB_NORMAL   = 2;

void RenderableObject::setupMesh(
    unsigned int programID,
    const std::vector<glm::vec4>& points,
    const std::vector<glm::vec4>& colors,
    const std::vector<glm::vec3>& normals) {
    this->modelLoc = glGetUniformLocation(programID, "model");
    this->vertexCount = (int)points.size();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // 한 버퍼에 [위치들][색들][법선들] 순서로 이어 붙인다
    size_t pointsBytes = points.size() * sizeof(glm::vec4);
    size_t colorsBytes = colors.size() * sizeof(glm::vec4);
    size_t normalsBytes = normals.size() * sizeof(glm::vec3);

    glBufferData(GL_ARRAY_BUFFER, pointsBytes + colorsBytes + normalsBytes, NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, pointsBytes, points.data());
    glBufferSubData(GL_ARRAY_BUFFER, pointsBytes, colorsBytes, colors.data());
    glBufferSubData(GL_ARRAY_BUFFER, pointsBytes + colorsBytes, normalsBytes, normals.data());

    // 각 속성이 버퍼의 어디서 시작하는지 알려준다 (stride 0 = 빈틈없이 이어진 배열)
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glEnableVertexAttribArray(ATTRIB_COLOR);
    glVertexAttribPointer(ATTRIB_COLOR, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)pointsBytes);

    glEnableVertexAttribArray(ATTRIB_NORMAL);
    glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(pointsBytes + colorsBytes));

    glBindVertexArray(0);
}

void RenderableObject::draw() {
    // 이동은 position, 회전은 rotation, 크기는 localMatrix에서 가져와 매 프레임 조립
    glm::mat4 model = glm::translate(glm::mat4(1.0f), this->position) * glm::mat4_cast(this->rotation) * this->mScale;
    glUniformMatrix4fv(this->modelLoc, 1, GL_FALSE, &model[0][0]);

    glBindVertexArray(this->vao);
    glDrawArrays(GL_TRIANGLES, 0, this->vertexCount);
    glBindVertexArray(0);
}

void RenderableObject::translate(glm::vec3 vec) {
    this->position += vec;
}

void RenderableObject::rotate(glm::vec3 axis, float elapsedTime) {
    // 기존 glm::rotate(localMatrix, ...)와 같은 곱셈 순서 (오브젝트 로컬 축 기준 회전)
    this->rotation = glm::normalize(this->rotation * glm::angleAxis(elapsedTime, glm::normalize(axis)));
}
