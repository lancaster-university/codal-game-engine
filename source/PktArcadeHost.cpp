#include "GameStateManager.h"
#include "CodalDmesg.h"
#include "Timer.h"

using namespace codal;

void PktArcadeDevice::handlePacket(PktSerialPkt* p)
{
    manager.processPacket(p);
}

PktArcadeHost::PktArcadeHost(PktDevice d, uint8_t playerNumber, GameStateManager& manager) :
               PktArcadeDevice(d, playerNumber, manager, DEVICE_ID_PKT_ARCADE_HOST)
{
    games = NULL;

    if (EventModel::defaultEventBus)
        EventModel::defaultEventBus->listen(this->id, GS_EVENT_SEND_GAME_STATE, this, &PktArcadeHost::sendState);
}

GameAdvertListItem* PktArcadeHost::getGamesList()
{
    return this->games;
}


// this will only be called if local, which is brilliant -- single driver multiple uses.
int PktArcadeHost::queueControlPacket()
{
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

    advert->max_players = manager.engine.getMaxPlayers();
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

void PktArcadeHost::handleControlPacket(ControlPacket* cp)
{
    DMESG("CTRL");
    GameAdvertisement* advert = (GameAdvertisement*)cp->data;

    // we're advertising and are receiving a join request.                                          we need probably a less intensive way of verifying a join
    if (cp->packet_type == GS_CONTROL_PKT_TYPE_JOIN && advert->game_id == manager.engine.getIdentifier() && ManagedString((char *)advert->game_name) == manager.engine.getName())
    {
        PktDevice d;

        d.address = cp->address;
        d.flags = cp->flags;
        d.rolling_counter = 0;
        d.serial_number = cp->serial_number;

        // if we have a player slot available and the arcade has requested to play...
        // if arcade has requested to spectate and we allow spectate mode
        if ((manager.engine.getAvailableSlots() > 0 && cp->flags & GS_CONTROL_FLAG_PLAY))
        {
            // communicate game state
            manager.addPlayer(d);
            cp->flags |= GS_CONTROL_FLAG_JOIN_ACK;

            PktSerialProtocol::send((uint8_t *)&cp, sizeof(ControlPacket), 0);

            // communicate the game state in a little while.
            Event(DEVICE_ID_PKT_ARCADE_HOST, GS_EVENT_SEND_GAME_STATE);
        }
        else if (cp->flags & GS_CONTROL_FLAG_SPECTATE)
        {
            manager.addSpectator(d);
            cp->flags |= GS_CONTROL_FLAG_JOIN_ACK;
        }
    }

    // if we're a host in broadcast mode, the user is looking for games.
    if (cp->packet_type == GS_CONTROL_PKT_TYPE_ADVERTISEMENT && device.flags & PKT_DEVICE_FLAGS_BROADCAST)
    {
        GameAdvertListItem* gameListItem = games;

        while(gameListItem)
        {
            // don't add duplicates.
            if (gameListItem->serial_number == cp->serial_number && gameListItem->item->game_id == advert->game_id)
                return;

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
    }
}

void PktArcadeHost::sendState(Event)
{
    int spritesPerPacket = GAME_STATE_PKT_DATA_SIZE / sizeof(InitialSpriteData);

    GameStatePacket gsp;
    gsp.type = GAME_STATE_PKT_TYPE_INITIAL_SPRITE_DATA; // lets just get inital sprite data working.
    gsp.owner = 0; // we're the owner...
    gsp.flags = 0; // 0 for now

    InitialSpriteData isd;

    int spriteCount = 0;
    uint8_t* dataPointer = gsp.data;

    for (int i = 0; i < GAME_ENGINE_MAX_SPRITES; i++)
    {
        // an empty sprite requires no action.
        if (manager.engine.sprites[i] == NULL)
            continue;

        // fill out the initial sprite data struct.
        isd.sprite_id = manager.engine.sprites[i]->getHash();
        isd.x = manager.engine.sprites[i]->getX();
        isd.y = manager.engine.sprites[i]->getY();

        // copy the struct into the game state packet.
        memcpy(dataPointer, &isd, sizeof(InitialSpriteData));
        dataPointer += sizeof(InitialSpriteData);
        spriteCount++;

        // if the packet is full, send packet and reset spriteCount.
        if (spriteCount == spritesPerPacket)
        {
            // queue for sending, and reset count
            int ret = PktSerialProtocol::send((uint8_t*)&gsp, sizeof(GameStatePacket), this->device.address);

            // queue full, sleep a lil.
            if (ret != DEVICE_OK)
            {
                fiber_sleep(10);
                PktSerialProtocol::send((uint8_t*)&gsp, sizeof(GameStatePacket), this->device.address);
            }

            spriteCount = 0;
        }
    }
}