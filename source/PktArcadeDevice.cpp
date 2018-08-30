#include "GameStateManager.h"

using namespace codal;

PktArcadeDevice::PktArcadeDevice(PktDevice d, uint8_t playerNumber, GameStateManager& manager, uint16_t id) :
        PktSerialDriver(d, PKT_DRIVER_CLASS_ARCADE, id),
        manager(manager)
{
    this->playerNumber = playerNumber;
    next = NULL;

    if (PktSerialProtocol::instance)
        PktSerialProtocol::instance->add(*this);
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

void PktArcadeDevice::handlePacket(PktSerialPkt* p)
{
    GameStatePacket* gsp = (GameStatePacket *)p->data;

    // if the classes are run in broadcast mode, packets could be received multiple times.
    // only process packets that we "own".
    DMESG("STD OWNER: %d %d", gsp->owner, this->playerNumber);
    if (gsp->owner == this->playerNumber)
        manager.processPacket(p);
}