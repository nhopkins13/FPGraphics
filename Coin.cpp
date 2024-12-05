#include "Coin.h"

Coin::Coin(const glm::vec3& position, float size)
    : _position(position), _size(size) {}

const glm::vec3& Coin::getPosition() const {
    return _position;
}

float Coin::getSize() const {
    return _size;
}
