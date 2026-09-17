#include "player.h"

Player::Player() {
    isStatic = false;
    useGravity = true;
    freezeRotation = true;
    setCollider(new BoxCollider(glm::vec3(0.6f, 1.0f, 0.6f)));
    collider->offset = glm::vec3(0.0f, 0.5f, 0.0f);
    position = glm::vec3(0.0f, 1.0f, 3.0f);
}

Player::Player(glm::vec3 pos){
    isStatic = false;
    useGravity = true;
    freezeRotation = true;
    setCollider(new BoxCollider(glm::vec3(0.6f, 1.0f, 0.6f)));
    collider->offset = glm::vec3(0.0f, 0.5f, 0.0f);
    setPos(pos);
}

void Player::setPos(glm::vec3 pos){
    position = pos;
}

void Player::jump() {
    if (this->isGrounded) {
        this->velocity.y = jumpSpeed;
        this->isGrounded = false;
    }
}

void Player::translate(glm::vec3 vec){
    this->position += vec;
}
