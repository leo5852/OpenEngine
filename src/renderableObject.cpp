#include "renderableObject.h"

RenderableObject::~RenderableObject() {
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
}

void RenderableObject::setupMesh(unsigned int programID, const std::vector<glm::vec4>& points, const std::vector<glm::vec4>& colors) {
    this->modelLoc = glGetUniformLocation(programID, "model");
    this->vertexCount = (int)points.size();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    size_t pointsBytes = points.size() * sizeof(glm::vec4);
    size_t colorsBytes = colors.size() * sizeof(glm::vec4);
    glBufferData(GL_ARRAY_BUFFER, pointsBytes + colorsBytes, NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, pointsBytes, points.data());
    glBufferSubData(GL_ARRAY_BUFFER, pointsBytes, colorsBytes, colors.data());

    GLuint vPosition = glGetAttribLocation(programID, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    GLuint vColor = glGetAttribLocation(programID, "vColor");
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)pointsBytes);

    glBindVertexArray(0);
}

void RenderableObject::draw() {
    // 이동은 position, 회전은 rotation, 크기는 localMatrix에서 가져와 매 프레임 조립
    glm::mat4 model = glm::translate(glm::mat4(1.0f), this->position) * glm::mat4_cast(this->rotation) * this->localMatrix;
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
