
#include "ArcballCamera.h"

// Constructor initializes the camera with default parameters
ArcballCamera::ArcballCamera()
    : _target(0.0f, 0.0f, 0.0f),
      _radius(10.0f),
      _theta(0.0f),
      _phi(glm::radians(45.0f))
{
    updatePosition();
}

//returns the view matrix using glm::lookAt
glm::mat4 ArcballCamera::getViewMatrix() const {
    return glm::lookAt(_position, _target, glm::vec3(0, 1, 0));
}

//rotates the camera by adjusting theta and phi
void ArcballCamera::rotate(float deltaTheta, float deltaPhi) {
    _theta += deltaTheta;
    _phi += deltaPhi;

    //(preventing the camera from flipping over)
    _phi = std::clamp(_phi, glm::radians(1.0f), glm::radians(179.0f));

    updatePosition();
}

void ArcballCamera::setTarget(const glm::vec3& target) {
    _target = target;
    updatePosition();
}

void ArcballCamera::zoom(float deltaRadius) {
    _radius += deltaRadius;

    // Clamp the radius to prevent the camera from zooming too far in or out
    _radius = std::clamp(_radius, MIN_RADIUS, MAX_RADIUS);

    updatePosition();
}

void ArcballCamera::zoomIn(float deltaRadius) {
    zoom(-deltaRadius);
}

void ArcballCamera::zoomOut(float deltaRadius) {
    zoom(deltaRadius);
}

// Updates the camera's position based on current parameters
void ArcballCamera::updatePosition() {
    _position.x = _target.x + _radius * sin(_phi) * sin(_theta);
    _position.y = _target.y + _radius * cos(_phi);
    _position.z = _target.z + _radius * sin(_phi) * cos(_theta);
}
