#include "Player.h"
#include "ErrorNo.h"

using namespace codal;

int Player::playerNumber = -1;

int Player::setX(int x)
{
    if (GameEngine::instance && playerNumber > -1)
    {
        GameEngine::instance->playerSprites[playerNumber]->setX(x);
        return DEVICE_OK;
    }

    return DEVICE_CANCELLED;
}

int Player::setY(int y)
{
    if (GameEngine::instance && playerNumber > -1)
    {
        GameEngine::instance->playerSprites[playerNumber]->setY(y);
        return DEVICE_OK;
    }

    return DEVICE_CANCELLED;
}

int Player::getX()
{
    if (GameEngine::instance && playerNumber > -1)
        return GameEngine::instance->playerSprites[playerNumber]->getX();

    return DEVICE_CANCELLED;
}

int Player::getY()
{
    if (GameEngine::instance && playerNumber > -1)
       return GameEngine::instance->playerSprites[playerNumber]->getY();

    return DEVICE_CANCELLED;
}