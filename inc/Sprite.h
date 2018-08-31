#ifndef CODAL_SPRITE_H
#define CODAL_SPRITE_H

#include "CodalConfig.h"
#include "Image.h"
#include "CoordinateSystem.h"
#include "PhysicsBody.h"
#include "CodalComponent.h"

#define SPRITE_EVT_BASE                 2
#define SPRITE_STATUS_DIRTY_BIT         0x02

namespace codal
{
    class Sprite : public CodalComponent
    {
        Image image;
        uint16_t flags;

        int startX;
        int startY;

        uint16_t variableHash;

        protected:
        int8_t owner;
        PhysicsBody& body;

        public:
        Sprite(ManagedString name, PhysicsBody& body, Image& i, int8_t owner = -1);

        int reset();

        int setImage(Image& i);

        Image getImage();

        int fill(uint8_t colour);

        int drawRectangle(uint16_t x, uint16_t y, int width, int height, int colour);

        int drawCircle(uint16_t x, uint16_t y, int radius, int colour);

        void draw(Image& i);

        uint16_t getHash();

        virtual void setX(int x);

        virtual void setY(int y);

        int getX();

        int getY();

        virtual void setXVelocity(float xVel);

        virtual void setYVelocity(float yVel);

        virtual float getXVelocity();

        virtual float getYVelocity();

        // for optimal updates
        void setDirty(bool dirty);

        bool isDirty();

        friend class PktArcadeDevice;
        friend class PktArcadeHost;
        friend class PktArcadePlayer;
        friend class PhysicsBody;
        friend class GameEngine;
    };
}

#endif