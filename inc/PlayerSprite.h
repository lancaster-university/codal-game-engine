#ifndef CODAL_PLAYER_SPRITE_H
#define CODAL_PLAYER_SPRITE_H

#include "CodalConfig.h"
#include "Image.h"
#include "CoordinateSystem.h"
#include "PhysicsBody.h"
#include "Sprite.h"

#define PLAYER_SPRITE_EVT_UPDATE        2

namespace codal
{
    class PlayerSprite : public Sprite
    {
        public:
        PlayerSprite(ManagedString name, PhysicsBody& body, Image& i, uint8_t owner);

        virtual int setX(int x);

        virtual int setY(int y);
    };
}

#endif