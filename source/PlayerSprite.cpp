#include "PlayerSprite.h"
#include "CodalDmesg.h"
#include "PearsonHash.h"

using namespace codal;

PlayerSprite::PlayerSprite(ManagedString name, PhysicsBody& body, Image& i, uint8_t owner) : Sprite(name, body, i, owner)
{
}

int PlayerSprite::setX(int x)
{
    body.position.x = x;
    Event(DEVICE_ID_PLAYER_SPRITE, this->owner);
    return DEVICE_OK;
}

int PlayerSprite::setY(int y)
{
    body.position.y = y;
    Event(DEVICE_ID_PLAYER_SPRITE, this->owner);
    return DEVICE_OK;
}