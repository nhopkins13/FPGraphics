// FPSCamera.cpp
#include "FPCamera.h"

FPCamera::FPCamera(float heightOffset)
    : _position(0.0f), _heading(0.0f), _heightOffset(heightOffset) {}

void FPCamera::updatePositionAndOrientation(const glm::vec3& heroPosition, float heroHeading) {
    _position = heroPosition + glm::vec3(0.0f, _heightOffset, 0.0f);
    _heading = heroHeading;
}

glm::vec3 FPCamera::getPosition() const {
    return _position;
}

glm::mat4 FPCamera::getViewMatrix() const {
    // Calculate the forward direction based on heading
    glm::vec3 forward = glm::vec3(sin(_heading), 0.0f, cos(_heading));
    // Camera looks in the forward direction from its position
    return glm::lookAt(_position, _position + forward, glm::vec3(0.0f, 1.0f, 0.0f));
}
