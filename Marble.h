#ifndef LAB11_MARBLE_H
#define LAB11_MARBLE_H

#include <glm/glm.hpp>
#include <glad/gl.h>
#include <CSCI441/ShaderProgram.hpp>

class Marble {
public:
    Marble();
    Marble(glm::vec3 location, glm::vec3 direction, GLfloat radius);
    Marble(Marble&& other) noexcept;
    Marble& operator=(Marble&& other) noexcept;

    static constexpr GLfloat RADIUS = 0.5f;  // Shared constant for all Marbles
    static constexpr GLfloat SPEED = 0.1f;   // Shared constant for movement speed

    [[nodiscard]] glm::vec3 getLocation() const;
    void setLocationX(GLfloat x);
    void setLocationZ(GLfloat z);
    [[nodiscard]] glm::vec3 getDirection() const;
    void setDirection(glm::vec3 newDirection);

    void draw(CSCI441::ShaderProgram* shaderProgram, GLint mvpUniformLocation, GLint colorUniformLocation, glm::mat4 modelMtx, glm::mat4 projViewMtx) const;
    void moveForward();
    void moveBackward();

private:
    GLfloat _rotation;
    glm::vec3 _location;
    glm::vec3 _direction;
    glm::vec3 _color;

    static GLfloat _genRandColor();
};

#endif // LAB11_MARBLE_H
