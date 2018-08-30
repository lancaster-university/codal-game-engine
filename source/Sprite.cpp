#include "Sprite.h"
#include "CodalDmesg.h"
#include "PearsonHash.h"

using namespace codal;

int Sprite::fill(uint8_t colour)
{
    DMESG("FILL");
    memset(image.getBitmap(), ((colour & 0x0f) << 4 | (colour & 0x0f)), image.getSize());
    return DEVICE_OK;
}

Sprite::Sprite(ManagedString name, PhysicsBody& body, Image& i, int8_t owner) : body(body)
{
    this->owner = owner;
    this->variableHash = PearsonHash::hash16(name);
    this->image = i;
    this->startX = body.position.x;
    this->startY = body.position.y;
}

uint16_t Sprite::getHash()
{
    return this->variableHash;
}

int Sprite::setX(int x)
{
    body.position.x = x;
    return DEVICE_OK;
}

int Sprite::setY(int y)
{
    body.position.y = y;
    return DEVICE_OK;
}

int Sprite::getX()
{
    return body.position.x;
}

int Sprite::getY()
{
    return body.position.y;
}

int Sprite::reset()
{
    body.position.x = this->startX;
    body.position.y = this->startY;

    return DEVICE_OK;
}

int Sprite::setImage(Image& newImage)
{
    this->image = newImage;
    return DEVICE_OK;
}

Image Sprite::getImage()
{
    return this->image;
}

int Sprite::drawCircle(uint16_t x, uint16_t y, int radius, int colour)
{
    return DEVICE_NOT_IMPLEMENTED;
}

int Sprite::drawRectangle(uint16_t x, uint16_t y, int width, int height, int colour)
{
    if (width == 0 || height == 0 || x >= image.getWidth() || y >= image.getHeight())
        return DEVICE_INVALID_PARAMETER;

    int x2 = x + width - 1;
    int y2 = y + height - 1;

    if (x2 < 0 || y2 < 0)
        return DEVICE_INVALID_PARAMETER;

    width = x2 - x + 1;
    height = y2 - y + 1;

    if (x == 0 && y == 0 && width == image.getWidth() && height == image.getHeight())
    {
        fill(colour);
        return DEVICE_OK;
    }

    return DEVICE_OK;
}

void Sprite::draw(Image& display)
{
    // DMESG("DRAW x: %d y: %d z: %d",  body.position.x, body.position.y, body.position.z);
    display.paste(this->image, body.position.x, body.position.y);
}

void Sprite::setDirty(bool dirty)
{
    if (dirty)
    {
        status |= SPRITE_STATUS_DIRTY_BIT;
        Event(DEVICE_ID_SPRITE, SPRITE_EVT_BASE + this->owner);
    }
    else
        status &= ~SPRITE_STATUS_DIRTY_BIT;
}

bool Sprite::isDirty(bool dirty)
{
    return (status & SPRITE_STATUS_DIRTY_BIT) ? true : false;
}