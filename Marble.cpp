#include "Marble.h"
#include <glm/gtc/matrix_transform.hpp>
#include <CSCI441/OpenGLUtils.hpp>
#include <CSCI441/objects.hpp>
#include <cstdlib>

Marble::Marble()
    : _rotation(0.0f), _location(glm::vec3(0, 0, 0)), _direction(glm::vec3(1, 0, 0)),
      _color(glm::vec3(_genRandColor(), _genRandColor(), _genRandColor())) {}

Marble::Marble(glm::vec3 location, glm::vec3 direction, GLfloat radius)
    : _rotation(0.0f), _location(location), _direction(glm::normalize(direction)),
      _color(glm::vec3(_genRandColor(), _genRandColor(), _genRandColor())) {}

Marble::Marble(Marble&& other) noexcept
    : _rotation(other._rotation),
      _location(std::move(other._location)),
      _direction(std::move(other._direction)),
      _color(std::move(other._color)) {}

Marble& Marble::operator=(Marble&& other) noexcept {
    if (this != &other) {
        _rotation = other._rotation;
        _location = std::move(other._location);
        _direction = std::move(other._direction);
        _color = std::move(other._color);
    }
    return *this;
}

glm::vec3 Marble::getLocation() const { return _location; }
void Marble::setLocationX(GLfloat x) { _location.x = x; }
void Marble::setLocationZ(GLfloat z) { _location.z = z; }
glm::vec3 Marble::getDirection() const { return _direction; }
void Marble::setDirection(glm::vec3 newDirection) { _direction = glm::normalize(newDirection); }

void Marble::draw(CSCI441::ShaderProgram* shaderProgram, GLint mvpUniformLocation, GLint colorUniformLocation, glm::mat4 modelMtx, glm::mat4 projViewMtx) const {
    modelMtx = glm::translate(modelMtx, _location);
    modelMtx = glm::translate(modelMtx, glm::vec3(0, RADIUS, 0));

    glm::vec3 rotationAxis = glm::cross(_direction, CSCI441::Y_AXIS);
    modelMtx = glm::rotate(modelMtx, _rotation, rotationAxis);

    glm::mat4 mvpMatrix = projViewMtx * modelMtx;
    shaderProgram->setProgramUniform(mvpUniformLocation, mvpMatrix);

    shaderProgram->setProgramUniform(colorUniformLocation, _color);

    CSCI441::drawSolidSphere(RADIUS, 16, 16);
}

void Marble::moveForward() {
    _location += _direction * SPEED;
    _rotation -= SPEED;
    if (_rotation < 0.0f) {
        _rotation += 6.28f;
    }
}

void Marble::moveBackward() {
    _location -= _direction * SPEED;
    _rotation += SPEED;
    if (_rotation > 6.28f) {
        _rotation -= 6.28f;
    }
}

GLfloat Marble::_genRandColor() {
    return static_cast<GLfloat>(rand() % 50) / 100.0f + 0.5f;
}
