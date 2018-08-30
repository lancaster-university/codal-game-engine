#include "GameStateManager.h"
#include "CodalDmesg.h"
#include "Timer.h"

using namespace codal;

PktArcadeHost::PktArcadeHost(PktDevice d, uint8_t playerNumber, GameStateManager& manager) :
               PktArcadeDevice(d, playerNumber, manager, DEVICE_ID_PKT_ARCADE_HOST)
{
    games = NULL;

    // we are always in charge of player 0.
    // only set if we're local (otherwise other players could control)
    if (device.flags & PKT_DEVICE_FLAGS_LOCAL)
        Player::playerNumber = 0;

    if (!(device.flags & PKT_DEVICE_FLAGS_BROADCAST))
        findControllingSprites();

    this->playerNumber = 0;

    if (EventModel::defaultEventBus)
    {
        EventModel::defaultEventBus->listen(DEVICE_ID_PLAYER_SPRITE, PLAYER_SPRITE_EVT_BASE + this->playerNumber, (PktArcadeDevice*)this, &PktArcadeDevice::updateSprite, MESSAGE_BUS_LISTENER_IMMEDIATE);
        EventModel::defaultEventBus->listen(this->id, GS_EVENT_SEND_GAME_STATE, this, &PktArcadeHost::sendState);
    }
}

GameAdvertListItem* PktArcadeHost::getGamesList()
{
    return this->games;
}


// this will only be called if local, which is brilliant -- single driver multiple uses.
int PktArcadeHost::queueControlPacket()
{
    // DMESG("HOST: addr %d flags %d sn %d",device.address, device.flags, device.serial_number);
    ControlPacket cp;
    memset(&cp, 0, sizeof(ControlPacket));

    cp.packet_type = GS_CONTROL_PKT_TYPE_ADVERTISEMENT;
    cp.address = device.address;
    cp.flags = 0;
    cp.driver_class = this->driver_class;
    cp.serial_number = device.serial_number;

    GameAdvertisement* advert = (GameAdvertisement*)cp.data;
    cp.flags |= GS_CONTROL_FLAG_SPECTATORS_ALLOWED;

    if (manager.engine.isRunning())
        cp.flags |= GS_CONTROL_FLAG_GAME_IN_PROGRESS;

    advert->playerNumber = 0;
    advert->slots_available = manager.engine.getAvailableSlots();
    advert->game_id = manager.engine.getIdentifier();

    ManagedString name = manager.engine.getName();
    memcpy(advert->game_name, name.toCharArray(), min(name.length(), GAME_STATE_NAME_SIZE));
    // set the null terminator
    advert->game_name[GAME_STATE_NAME_SIZE] = 0;

    PktSerialProtocol::send((uint8_t *)&cp, sizeof(ControlPacket), 0);

    return DEVICE_OK;
}

PktArcadeHost::~PktArcadeHost()
{
    // ensure we delete any allocated memory.
    if (games == NULL)
        return;

    GameAdvertListItem* current = games;
    GameAdvertListItem* temp = NULL;

    while (current)
    {
        temp = current;
        current = current->next;
        delete temp->item;
        delete temp;
    }

    games = NULL;
}

int PktArcadeHost::handleControlPacket(ControlPacket* cp)
{
    GameAdvertisement* advert = (GameAdvertisement*)cp->data;

    DMESG("CTRL t: %d", cp->packet_type);

    // we're advertising and are receiving a join request.                                          we need probably a less intensive way of verifying a join
    if (cp->packet_type == GS_CONTROL_PKT_TYPE_JOIN && !(this->device.flags & PKT_DEVICE_FLAGS_REMOTE) && advert->game_id == manager.engine.getIdentifier() && ManagedString((char *)advert->game_name) == manager.engine.getName())
    {
        PktDevice d;

        d.address = cp->address;
        d.flags = cp->flags | PKT_DEVICE_FLAGS_REMOTE | PKT_DEVICE_FLAGS_INITIALISED | PKT_DEVICE_FLAGS_CP_SEEN;
        d.rolling_counter = 0;
        d.serial_number = cp->serial_number;

        DMESG("PLAYER: %d %d",d.address,d.serial_number);
        // if we have a player slot available and the arcade has requested to play...
        // if arcade has requested to spectate and we allow spectate mode

        uint8_t avail = manager.engine.getAvailableSlots();
        uint8_t max = manager.engine.getMaxPlayers();

        if (avail > 0 && (cp->flags & GS_CONTROL_FLAG_PLAY))
        {
            // communicate game state
            uint8_t playerNo = max - (max - avail);
            DMESG("CPlayerNo: %d", playerNo);
            manager.addPlayer(d, playerNo);
            advert->playerNumber = playerNo;
            cp->flags |= GS_CONTROL_FLAG_JOIN_ACK;

            // communicate the game state in a little while.
            Event(DEVICE_ID_PKT_ARCADE_HOST, GS_EVENT_SEND_GAME_STATE);
        }
        else if (cp->flags & GS_CONTROL_FLAG_SPECTATE)
        {
            manager.addSpectator(d);
            cp->flags |= GS_CONTROL_FLAG_JOIN_ACK;
        }

        PktSerialProtocol::send((uint8_t *)cp, sizeof(ControlPacket), 0);
        return DEVICE_OK;
    }

    // if we're a host in broadcast mode, the user is looking for games.
    if (cp->packet_type == GS_CONTROL_PKT_TYPE_ADVERTISEMENT && device.flags & PKT_DEVICE_FLAGS_BROADCAST)
    {
        DMESG("ADVERT");

        GameAdvertListItem* gameListItem = games;

        while(gameListItem)
        {
            // don't add duplicates.
            if (gameListItem->serial_number == cp->serial_number && gameListItem->item->game_id == advert->game_id)
                return DEVICE_OK;

            gameListItem = gameListItem->next;
        }

        GameAdvertisement* newAd = (GameAdvertisement *)malloc(sizeof(GameAdvertisement));
        memcpy(newAd, advert, sizeof(GameAdvertisement));

        GameAdvertListItem* gali = (GameAdvertListItem *)malloc(sizeof(GameAdvertListItem));
        gali->serial_number = cp->serial_number;
        gali->item = newAd;
        gali->next = NULL;

        if (games == NULL)
            games = gali;
        else
        {
            GameAdvertListItem* iterator = games;

            while(iterator->next)
                iterator = iterator->next;

            iterator->next = gali;
        }

        return DEVICE_OK;
    }

    return DEVICE_CANCELLED;
}

int PktArcadeHost::handlePacket(PktSerialPkt* p)
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
        return DEVICE_OK;
    }

    return DEVICE_CANCELLED;
}

int PktArcadeHost::deviceConnected(PktDevice d)
{
    PktSerialDriver::deviceConnected(d);
    manager.hostConnectionChange(this, true);
    DMESG("H C %d %d", this->device.address, this->playerNumber);
    return DEVICE_OK;
}

int PktArcadeHost::deviceRemoved()
{
    PktSerialDriver::deviceRemoved();
    manager.hostConnectionChange(this, false);
    DMESG("H D/C %d %d", this->device.address, this->playerNumber);
    return DEVICE_OK;
}

void PktArcadeHost::safeSend(uint8_t* data, int len)
{
    // queue for sending, and reset count
    int ret = -1;

    // should move to pktserialprotocol send (drivers shouldn't have to guarantee sending...)
    while((ret = PktSerialProtocol::send(data, len, this->device.address)) != DEVICE_OK)
        // queue full, sleep a lil.
        fiber_sleep(10);
}

void PktArcadeHost::sendState(Event)
{
    DMESG("SEND SPRITES ADDR: %d", this->device.address);
    int spritesPerPacket = GAME_STATE_PKT_DATA_SIZE / sizeof(InitialSpriteData);

    GameStatePacket gsp;
    gsp.type = GAME_STATE_PKT_TYPE_INITIAL_SPRITE_DATA; // lets just get inital sprite data working.
    gsp.owner = 0; // we're the owner...
    gsp.count = 0; // 0 for now

    InitialSpriteData isd;

    int spriteCount = 0;
    int packetCount = 0;
    uint8_t* dataPointer = gsp.data;

    int i = 0;

    while (i < GAME_ENGINE_MAX_SPRITES)
    {
        Sprite* sprite = manager.engine.sprites[i];

        if (sprite)
        {
            // fill out the initial sprite data struct.
            isd.owner = sprite->owner;
            isd.sprite_id = sprite->getHash();
            isd.x = sprite->getX();
            isd.y = sprite->getY();

            // copy the struct into the game state packet.
            memcpy(dataPointer, &isd, sizeof(InitialSpriteData));
            dataPointer += sizeof(InitialSpriteData);

            spriteCount++;
            gsp.count = spriteCount;
            spriteCount %= spritesPerPacket;

            // if the packet is full, send packet and reset spriteCount.
            if (spriteCount == 0)
            {
                safeSend((uint8_t *)&gsp, sizeof(GameStatePacket));
                spriteCount = 0;
                packetCount++;
            }
        }

        i++;
    }

    if (spriteCount > 0)
        safeSend((uint8_t *)&gsp, sizeof(GameStatePacket));

    DMESG("END SEND SPRITES %d", packetCount);
}