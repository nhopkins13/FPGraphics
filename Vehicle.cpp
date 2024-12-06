#include "Vehicle.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Vehicle::Vehicle(GLuint shaderProgramHandle, GLint mvpMatrixLocation, GLint normalMatrixLocation,
                 GLint materialAmbientLocation, GLint materialDiffuseLocation,
                 GLint materialSpecularLocation, GLint materialShininessLocation)
    : _shaderProgramHandle(shaderProgramHandle),
      _mvpMatrixLocation(mvpMatrixLocation),
      _normalMatrixLocation(normalMatrixLocation),
      _materialAmbientLocation(materialAmbientLocation),
      _materialDiffuseLocation(materialDiffuseLocation),
      _materialSpecularLocation(materialSpecularLocation),
      _materialShininessLocation(materialShininessLocation),
      _position(0.0f, 0.0f, 0.0f),
      _heading(0.0f),
      _wheelRotation(0.0f)
{}

void Vehicle::drawVehicle(glm::mat4 viewMtx, glm::mat4 projMtx) const {
    if (!_isVisible){return;}
    glm::mat4 modelMtx = glm::translate(glm::mat4(1.0f), _position + glm::vec3(0.0f, 0.85f, 0.0f));
    glm::mat4 rotatedMtx = glm::rotate(glm::mat4(1.0f), _heading + glm::radians(90.0f), glm::vec3(0, 1, 0));
    glm::mat4 finalModelMtx = modelMtx * rotatedMtx;

    _drawBody(finalModelMtx, viewMtx, projMtx);

    _drawRoof(finalModelMtx, viewMtx, projMtx);
    _drawWheels(finalModelMtx, viewMtx, projMtx);
}

void Vehicle::driveForward() {
    float speed = 0.2f;
    // Updated movement vector to align with +Z-axis front
    _position += glm::vec3(sin(_heading), 0.0f, cos(_heading)) * speed;

    // Calculate rotation based on movement and wheel radius
    float wheelRadius = 0.5f;
    float rotationSpeed = (speed / (2.0f * M_PI * wheelRadius)) * 2.0f * M_PI; // radians
    _wheelRotation += rotationSpeed;

    if (_wheelRotation > 2.0f * M_PI) _wheelRotation -= 2.0f * M_PI;
}

void Vehicle::driveBackward() {
    float speed = 0.2f;
    // Updated movement vector to align with +Z-axis front
    _position -= glm::vec3(sin(_heading), 0.0f, cos(_heading)) * speed;

    // Calculate rotation based on movement and wheel radius
    float wheelRadius = 0.5f; // Ensure this matches your wheel scaling
    float rotationSpeed = (speed / (2.0f * M_PI * wheelRadius)) * 2.0f * M_PI; // radians
    _wheelRotation -= rotationSpeed;

    // Keep rotation angle within 0 to 2*PI
    if (_wheelRotation < 0.0f) _wheelRotation += 2.0f * M_PI;
}

void Vehicle::turnLeft() {
    float turnSpeed = glm::radians(2.0f);
    _heading += turnSpeed;

    if (_heading >= 2.0f * M_PI) _heading -= 2.0f * M_PI;
}

void Vehicle::turnRight() {
    float turnSpeed = glm::radians(2.0f);
    _heading -= turnSpeed;

    if (_heading < 0.0f) _heading += 2.0f * M_PI;
}


void Vehicle::_drawBody(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    glm::mat4 bodyMtx = modelMtx * glm::scale(glm::mat4(1.0f), glm::vec3(3.0f, 0.5f, 1.5f));

    glm::mat4 mvpMtx = projMtx * viewMtx * bodyMtx;
    glm::mat3 normalMtx = glm::transpose(glm::inverse(glm::mat3(bodyMtx)));

    glUniformMatrix4fv(_mvpMatrixLocation, 1, GL_FALSE, glm::value_ptr(mvpMtx));
    glUniformMatrix3fv(_normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMtx));

    glm::vec3 ambient(0.6f, 0.0f, 0.6f);    // Increased ambient to match vibrant color
    glm::vec3 diffuse(1.0f, 0.0f, 1.0f);    // Hot Pink
    glm::vec3 specular(1.0f, 1.0f, 1.0f);   // Specular remains white
    float shininess = 32.0f;

    glUniform3fv(_materialAmbientLocation, 1, glm::value_ptr(ambient));
    glUniform3fv(_materialDiffuseLocation, 1, glm::value_ptr(diffuse));
    glUniform3fv(_materialSpecularLocation, 1, glm::value_ptr(specular));
    glUniform1f(_materialShininessLocation, shininess);

    CSCI441::drawSolidCube(1.0f);
}

void Vehicle::_drawRoof(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    // Position the roof exactly at the top of the car body
    glm::mat4 roofMtx = modelMtx * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));
    roofMtx = glm::scale(roofMtx, glm::vec3(1.0f, 0.5f, 1.0f)); // Adjust scale as needed

    // Compute MVP and Normal matrices
    glm::mat4 mvpMtx = projMtx * viewMtx * roofMtx;
    glm::mat3 normalMtx = glm::transpose(glm::inverse(glm::mat3(roofMtx)));

    // Send matrices to shader
    glUniformMatrix4fv(_mvpMatrixLocation, 1, GL_FALSE, glm::value_ptr(mvpMtx));
    glUniformMatrix3fv(_normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMtx));

    // Set material properties for roof (Light Pink)
    glm::vec3 ambient(0.4f, 0.3f, 0.3f);    // Increased ambient for lighter base
    glm::vec3 diffuse(1.0f, 0.75f, 0.8f);   // Light Pink
    glm::vec3 specular(1.0f, 1.0f, 1.0f);   // Specular remains white
    float shininess = 16.0f;

    glUniform3fv(_materialAmbientLocation, 1, glm::value_ptr(ambient));
    glUniform3fv(_materialDiffuseLocation, 1, glm::value_ptr(diffuse));
    glUniform3fv(_materialSpecularLocation, 1, glm::value_ptr(specular));
    glUniform1f(_materialShininessLocation, shininess);

    // Draw the roof as a cube using CSCI441
    CSCI441::drawSolidCube(1.0f);
}

void Vehicle::_drawWheels(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    // Restore original wheel positions
    glm::vec3 wheelOffsets[4] = {
        glm::vec3(-1.0f, -0.5f, -0.82f), // Front Left (FL)
        glm::vec3(1.0f, -0.5f, -0.82f),  // Front Right (FR)
        glm::vec3(-1.0f, -0.5f, 0.62f),  // Rear Left (RL)
        glm::vec3(1.0f, -0.5f, 0.62f)    // Rear Right (RR)
    };

    for(int i = 0; i < 4; ++i) {
        // Position each wheel
        glm::mat4 wheelMtx = modelMtx * glm::translate(glm::mat4(1.0f), wheelOffsets[i]);

        // Rotate the wheel by 90 degrees around the X-axis to align horizontally
        wheelMtx = glm::rotate(wheelMtx, glm::radians(90.0f), glm::vec3(1, 0, 0));

        // Rotate the wheel around the Y-axis for animation (spinning)
        wheelMtx = glm::rotate(wheelMtx, _wheelRotation, glm::vec3(0, 1, 0));


        wheelMtx = glm::scale(wheelMtx, glm::vec3(0.5f, 0.2f, 0.5f));

        glm::mat4 mvpMtx = projMtx * viewMtx * wheelMtx;
        glm::mat3 normalMtx = glm::transpose(glm::inverse(glm::mat3(wheelMtx)));

        // Send matrices to shader
        glUniformMatrix4fv(_mvpMatrixLocation, 1, GL_FALSE, glm::value_ptr(mvpMtx));
        glUniformMatrix3fv(_normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMtx));

        // Set material properties for wheels
        glm::vec3 ambient(0.2f, 0.2f, 0.2f);
        glm::vec3 diffuse(0.1f, 0.1f, 0.1f); // Dark Gray
        glm::vec3 specular(0.5f, 0.5f, 0.5f);
        float shininess = 8.0f;

        glUniform3fv(_materialAmbientLocation, 1, glm::value_ptr(ambient));
        glUniform3fv(_materialDiffuseLocation, 1, glm::value_ptr(diffuse));
        glUniform3fv(_materialSpecularLocation, 1, glm::value_ptr(specular));
        glUniform1f(_materialShininessLocation, shininess);

        CSCI441::drawSolidCylinder(0.5f, 0.5f, 1.0f, 16, 16);
    }

}
void Vehicle::setPosition(glm::vec3 &vec) {
    _position = vec;
}


void Vehicle::animateFall(float time) {
    // Simulate spinning during the fall
    float spinSpeed = 5.0f;  // Spin speed
    float fallSpeed = -2.0f; // Falling speed

    // Rotate and move down
    _heading += spinSpeed * time;               // Rotate around Y-axis
    _position.y += fallSpeed * 0.016f * time;  // Gradual downward movement
}

