#ifndef GOLDSTAR_H
#define GOLDSTAR_H

#include <glm/glm.hpp>

class Coin {
public:
    Coin(const glm::vec3& position, float size);

    const glm::vec3& getPosition() const;
    float getSize() const;

private:
    glm::vec3 _position;
    float _size;
};

#endif // GOLDSTAR_H
