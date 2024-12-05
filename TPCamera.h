#ifndef TP_CAMERA_H
#define TP_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class TPCamera {
public:
    TPCamera();

    void update(const glm::vec3& targetPosition, float targetHeading);
    glm::mat4 getViewMatrix() const;

    glm::vec3 getPosition() const;

private:
    glm::vec3 _position; // Camera position
    glm::vec3 _target;   // Camera target
    float _distance;     // Distance from target
    float _heightOffset; // Vertical offset
};

#endif // TP_CAMERA_H
