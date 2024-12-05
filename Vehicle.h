#ifndef VEHICLE_H
#define VEHICLE_H

#include <glm/glm.hpp>
#include <CSCI441/objects.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Vehicle {
public:
    Vehicle(GLuint shaderProgramHandle, GLint mvpMatrixLocation, GLint normalMatrixLocation,
            GLint materialAmbientLocation, GLint materialDiffuseLocation,
            GLint materialSpecularLocation, GLint materialShininessLocation);

    void drawVehicle(glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void driveForward();
    void driveBackward();
    void turnLeft();
    void turnRight();
    void updateAnimation();
    void animateFall(float time);

    glm::vec3 getPosition() const { return _position; }
    void setPosition(glm::vec3 &vec);
    float getBoundingRadius() const {return _boundingRadius; }
    float getHeading() const { return _heading; }
    void setPosition(const glm::vec3& pos) { _position = pos; }
    void setHeading(float heading) { _heading = heading; }
    int getCoinCount() {
        return coinCount;
    }
    void setCoinCount(size_t coinCount) {
        this->coinCount = coinCount;
    }
    void toggleVisibility() { _isVisible = !_isVisible; }
    bool isVisible() const { return _isVisible; }

private:
    bool _isVisible = true;
    GLuint _shaderProgramHandle;
    GLint _mvpMatrixLocation;
    GLint _normalMatrixLocation;
    GLint _materialAmbientLocation;
    GLint _materialDiffuseLocation;
    GLint _materialSpecularLocation;
    GLint _materialShininessLocation;

    glm::vec3 _position;
    float _boundingRadius = 1.0;
    float _heading; // Y-axis rotation in radians

    // Animation state
    float _wheelRotation;

    int coinCount;

    void _drawBody(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void _drawRoof(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void _drawWheels(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const;
};

#endif // VEHICLE_H
