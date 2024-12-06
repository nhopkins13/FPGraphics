#include "TPCamera.h"

TPCamera::TPCamera() {
    _distance = 13.0f;
    _heightOffset = 5.0f;
}


glm::vec3 TPCamera::getPosition() const {
    return _position;
}

void TPCamera::update(const glm::vec3& targetPosition, float targetHeading) {
    // Calculate offset position based on heading
    glm::vec3 offset = glm::vec3(
        _distance * -sin(targetHeading),  // Offset in X based on heading
        _heightOffset,                   // Offset above the target in Y
        _distance * -cos(targetHeading)  // Offset in Z based on heading
    );

    // Update camera position and target
    _position = targetPosition + offset;
    _target = targetPosition;
}

glm::mat4 TPCamera::getViewMatrix() const {
    return glm::lookAt(_position, _target, glm::vec3(0.0f, 1.0f, 0.0f)); // Y-axis as "up"
}
