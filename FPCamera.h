#ifndef FP_CAMERA_H
#define FP_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class FPCamera {
public:
    FPCamera(float heightOffset = 2.0f);

    void updatePositionAndOrientation(const glm::vec3& heroPosition, float heroHeading);
    glm::mat4 getViewMatrix() const;
    glm::vec3 getPosition() const;

private:
    glm::vec3 _position;
    float _heading;
    float _heightOffset;
};

#endif // FP_CAMERA_H
