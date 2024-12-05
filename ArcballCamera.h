#ifndef ARCBALLCAMERA_H
#define ARCBALLCAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

class ArcballCamera {
public:
    ArcballCamera();

    glm::mat4 getViewMatrix() const;
    void rotate(float deltaTheta, float deltaPhi);
    void setTarget(const glm::vec3& target);
    void updatePosition();

    void zoom(float deltaRadius);          // General zoom by delta
    void zoomIn(float deltaRadius = 0.5f);   // Zoom in by decreasing radius
    void zoomOut(float deltaRadius = 0.5f);  // Zoom out by increasing radius

    float getTheta() const { return _theta; }
    float getPhi() const { return _phi; }
    glm::vec3 getPosition() const { return _position; }

private:
    glm::vec3 _target;
    float _radius;
    float _theta;
    float _phi;
    glm::vec3 _position;

    const float MIN_RADIUS = 2.0f;
    const float MAX_RADIUS = 50.0f;
};

#endif // ARCBALLCAMERA_H
