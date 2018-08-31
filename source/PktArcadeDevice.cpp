#include "GameStateManager.h"

using namespace codal;

PktArcadeDevice::PktArcadeDevice(PktDevice d, uint8_t playerNumber, GameStateManager& manager, uint16_t id) :
        PktSerialDriver(d, PKT_DRIVER_CLASS_ARCADE, id),
        manager(manager)
{
    memset(this->controllingSprites, 0, PKT_ARCADE_DEVICE_MAX_CONTROL_SPRITES * sizeof(Sprite*));
    this->playerNumber = playerNumber;
    next = NULL;

    if (PktSerialProtocol::instance)
        PktSerialProtocol::instance->add(*this);
}

void PktArcadeDevice::findControllingSprites()
{
    if (GameEngine::instance)
    {
        // reset the array (in case sprites have been removed).
        memset(this->controllingSprites, 0, PKT_ARCADE_DEVICE_MAX_CONTROL_SPRITES * sizeof(Sprite*));

        int controlSpriteIdx = 0;
        for (int i = 0; i < GAME_ENGINE_MAX_SPRITES; i++)
        {
            if (controlSpriteIdx >= PKT_ARCADE_DEVICE_MAX_CONTROL_SPRITES - 1)
            {
                DMESG("MAX SPRITES reached");
                break;
            }

            Sprite* s = GameEngine::instance->sprites[i];

            // set the slot in the array.
            if (s && s->owner == this->playerNumber)
                this->controllingSprites[controlSpriteIdx++] = s;
        }
    }
}

void PktArcadeDevice::safeSend(uint8_t* data, int len)
{
    // queue for sending, and reset count
    int ret = -1;

    // should move to pktserialprotocol send (drivers shouldn't have to guarantee sending...)
    while((ret = PktSerialProtocol::send(data, len, this->device.address)) != DEVICE_OK)
        // queue full, sleep a lil.
        fiber_sleep(10);
}

void PktArcadeDevice::sendMovingSprites()
{
    GameStatePacket gsp;
    gsp.type = GAME_STATE_PKT_TYPE_MOVING_SPRITE_DATA;
    gsp.owner = this->playerNumber; // we're the owner...
    gsp.count = 0;

    MovingSpriteData msd;

    int spritesPerPacket = GAME_STATE_PKT_DATA_SIZE / sizeof(MovingSpriteData);
    uint8_t* dataPointer = gsp.data;

    for (int i = 0; i < PKT_ARCADE_DEVICE_MAX_CONTROL_SPRITES; i++)
    {
        Sprite* sprite = controllingSprites[i];

        if (sprite && sprite->isDirty())
        {
            // fill out the initial sprite data struct.
            msd.sprite_id = sprite->getHash();
            msd.x = sprite->getX();
            msd.y = sprite->getY();
            msd.dx = sprite->getXVelocity();
            msd.dy = sprite->getYVelocity();

            sprite->setDirty(false);

            // copy the struct into the game state packet.
            memcpy(dataPointer, &msd, sizeof(MovingSpriteData));

            dataPointer += sizeof(MovingSpriteData);
            gsp.count++;

            // if the packet is full, send packet and reset spriteCount.
            if (gsp.count % spritesPerPacket == 0)
            {
                safeSend((uint8_t *)&gsp, sizeof(GameStatePacket));
                dataPointer = gsp.data;
                gsp.count = 0;
            }
        }
    }

    if (gsp.count > 0)
        safeSend((uint8_t *)&gsp, sizeof(GameStatePacket));
}

void PktArcadeDevice::sendInitialSprites(/* dirty filter? */)
{
    GameStatePacket gsp;
    gsp.type = GAME_STATE_PKT_TYPE_INITIAL_SPRITE_DATA;
    gsp.owner = this->playerNumber; // we're the owner...
    gsp.count = 0;

    InitialSpriteData isd;

    int spritesPerPacket = GAME_STATE_PKT_DATA_SIZE / sizeof(InitialSpriteData);
    uint8_t* dataPointer = gsp.data;

    for (int i = 0; i < PKT_ARCADE_DEVICE_MAX_CONTROL_SPRITES; i++)
    {
        Sprite* sprite = controllingSprites[i];

        if (sprite && sprite->isDirty())
        {
            // fill out the initial sprite data struct.
            isd.sprite_id = sprite->getHash();
            isd.owner = this->playerNumber;
            isd.x = sprite->getX();
            isd.y = sprite->getY();

            sprite->setDirty(false);

            // copy the struct into the game state packet.
            memcpy(dataPointer, &isd, sizeof(InitialSpriteData));

            dataPointer += sizeof(InitialSpriteData);
            gsp.count++;

            // if the packet is full, send packet and reset spriteCount.
            if (gsp.count % spritesPerPacket == 0)
            {
                safeSend((uint8_t *)&gsp, sizeof(GameStatePacket));
                dataPointer = gsp.data;
                gsp.count = 0;
            }
        }
    }

    if (gsp.count > 0)
        safeSend((uint8_t *)&gsp, sizeof(GameStatePacket));
}

void PktArcadeDevice::updateSprite(Event)
{
    DMESG("US ADDR: %d PN: %d", this->device.address, this->playerNumber);
    if (GameEngine::instance)
    {

        if (GameEngine::instance->isRunning())
            // if the engine is running we also need to set velocities...
            sendMovingSprites();
        else
            // if the physics engine isn't running, we can send more simple sprite structs
            sendInitialSprites();
    }
}

int PktArcadeDevice::handlePacket(PktSerialPkt* p)
{
    GameStatePacket* gsp = (GameStatePacket *)p->data;

    // if the classes are run in broadcast mode, packets could be received multiple times.
    // only process packets that we "own".
    DMESG("STD OWNER: %d %d", gsp->owner, this->playerNumber);
    if (gsp->owner == this->playerNumber)
    {
        GameStatePacket* gsp = (GameStatePacket *)p->data;

        if (gsp->type == GAME_STATE_PKT_TYPE_INITIAL_SPRITE_DATA)
        {
            int spriteCount = gsp->count;

            InitialSpriteData* isd = (InitialSpriteData *)gsp->data;
            for (int i = 0; i < spriteCount; i++)
            {
                for (int j = 0; j < GAME_ENGINE_MAX_SPRITES; j++)
                {
                    // an empty sprite requires no action.
                    if (GameEngine::instance->sprites[j] == NULL)
                        continue;

                    // if match perform state sync.
                    if (GameEngine::instance->sprites[j]->getHash() == isd[i].sprite_id)
                    {
                        GameEngine::instance->sprites[j]->body.setX(isd[i].x);
                        GameEngine::instance->sprites[j]->body.setY(isd[i].y);
                    }
                }
            }
        }

        if (gsp->type == GAME_STATE_PKT_TYPE_MOVING_SPRITE_DATA)
        {
            int spriteCount = gsp->count;

            MovingSpriteData* msd = (MovingSpriteData *)gsp->data;
            for (int i = 0; i < spriteCount; i++)
            {
                for (int j = 0; j < GAME_ENGINE_MAX_SPRITES; j++)
                {
                    // an empty sprite requires no action.
                    if (GameEngine::instance->sprites[j] == NULL)
                        continue;

                    // if match perform state sync.
                    if (GameEngine::instance->sprites[j]->getHash() == msd[i].sprite_id)
                    {
                        GameEngine::instance->sprites[j]->body.setX(msd[i].x);
                        GameEngine::instance->sprites[j]->body.setY(msd[i].y);
                        GameEngine::instance->sprites[j]->body.setXVelocity(msd[i].dx);
                        GameEngine::instance->sprites[j]->body.setYVelocity(msd[i].dy);
                    }
                }
            }
        }

        if (gsp->type == GAME_STATE_PKT_TYPE_EVENT)
        {

        }

        return DEVICE_OK;
    }

    return DEVICE_CANCELLED;
}