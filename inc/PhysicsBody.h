#ifndef CODAL_PHYSICS_MODEL_H
#define CODAL_PHYSICS_MODEL_H

#include "CoordinateSystem.h"
#include "ErrorNo.h"

namespace codal
{
    enum PhysicsFlag
    {
        PhysicsStayOnScreen = 0,
        PhysicsStatic,
        PhysicsNoCollide
    };

    class PhysicsBody
    {
        public:
        int width;
        int height;

        Sample3D position;
        Sample3D oldPosition;

        uint16_t flags;

        PhysicsBody(int16_t x, int16_t y, int16_t z, int width, int height): position(x,y,z)
        {
            this->width = width;
            this->height = height;
        }

        virtual void apply(){}

        virtual bool intersectsWith(PhysicsBody& pb) { return false;}

        virtual void collideWith(PhysicsBody&) {}

        virtual void setX(int x)
        {
            this->position.x = x;
        }

        virtual void setY(int y)
        {
            this->position.y = y;
        }

        virtual int getX()
        {
            return this->position.x;
        }

        virtual int getY()
        {
            return this->position.y;
        }

        virtual void setXVelocity(float xVel) {}

        virtual void setYVelocity(float yVel) {}

        virtual float getXVelocity() { return 0; }

        virtual float getYVelocity() { return 0; }
    };
}

#endif