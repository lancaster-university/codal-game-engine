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

void PktArcadeDevice::updateSprite(Event)
{
    DMESG("US ADDR: %d PN: %d", this->device.address, this->playerNumber);
    if (GameEngine::instance)
    {
        Sprite* sprite = GameEngine::instance->playerSprites[this->playerNumber];

        GameStatePacket gsp;
        gsp.type = GAME_STATE_PKT_TYPE_INITIAL_SPRITE_DATA; // lets just get inital sprite data working.
        gsp.owner = this->playerNumber; // we're the owner...
        gsp.count = 1; // 0 for now

        InitialSpriteData isd;

        // fill out the initial sprite data struct.
        isd.owner = sprite->owner;
        isd.sprite_id = sprite->getHash();
        isd.x = sprite->getX();
        isd.y = sprite->getY();

        // copy the struct into the game state packet.
        memcpy(gsp.data, &isd, sizeof(InitialSpriteData));

        PktSerialProtocol::send((uint8_t*)&gsp, sizeof(GameStatePacket), this->device.address);
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
                        GameEngine::instance->sprites[j]->body.position.x = isd[i].x;
                        GameEngine::instance->sprites[j]->body.position.y = isd[i].y;
                    }
                }
            }
        }

        if (gsp->type == GAME_STATE_PKT_TYPE_MOVING_SPRITE_DATA)
        {

        }

        if (gsp->type == GAME_STATE_PKT_TYPE_EVENT)
        {

        }

        return DEVICE_OK;
    }

    return DEVICE_CANCELLED;
}