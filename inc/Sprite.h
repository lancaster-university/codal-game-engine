#ifndef CODAL_SPRITE_H
#define CODAL_SPRITE_H

#include "CodalConfig.h"
#include "Image.h"
#include "CoordinateSystem.h"
#include "PhysicsBody.h"

namespace codal
{
    class Sprite
    {
        Image image;
        uint16_t flags;

        int startX;
        int startY;

        public:
        PhysicsBody& body;

        uint16_t variableHash;

        Sprite(ManagedString name, PhysicsBody& body, Image& i);

        int reset();

        int setImage(Image& i);

        Image getImage();

        int fill(uint8_t colour);

        int drawRectangle(uint16_t x, uint16_t y, int width, int height, int colour);

        int drawCircle(uint16_t x, uint16_t y, int radius, int colour);

        void draw(Image& i);
    };
}

#endif