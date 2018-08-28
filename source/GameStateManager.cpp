#include "GameStateManager.h"
#include "CodalDmesg.h"
#include "Timer.h"

using namespace codal;

int GameStateManager::clearList(PktArcadeDevice** list)
{
    PktArcadeDevice* head = *list;

    if (head == NULL)
        return DEVICE_OK;

    PktArcadeDevice* temp = NULL;

    while (head)
    {
        temp = head;
        head = temp->next;
        delete temp;
    }

    *list = NULL;

    return DEVICE_OK;
}

int GameStateManager::addToList(PktArcadeDevice** list, PktArcadeDevice* item)
{
    PktArcadeDevice* iterator = *list;
    item->next = NULL;

    if (iterator == NULL)
        *list = item;
    else
    {
        while (iterator->next)
        iterator = iterator->next;

        iterator->next = item;
    }

    return DEVICE_OK;
}

GameStateManager::GameStateManager(GameEngine& ge, uint32_t serial, uint16_t id) :
    engine(ge),
    host(NULL),
    players(NULL)
{
    this->id = id;
    this->serial_number = serial;
}

int GameStateManager::hostGame()
{
    if (status & GAME_STATE_STATUS_LISTING_GAMES)
        status &= (GAME_STATE_STATUS_LISTING_GAMES);

    if (!(status & GAME_STATE_STATUS_ADVERTISING))
    {
        // clean host list, there should only ever be one, but lets future proof code just in case.
        clearList((PktArcadeDevice**)&host);
        addToList((PktArcadeDevice**)&host, new PktArcadeHost(PktDevice(0, 0, PKT_DEVICE_FLAGS_LOCAL, this->serial_number), 0, *this));

        this->status |= GAME_STATE_STATUS_ADVERTISING;
    }

    return DEVICE_OK;
}

GameAdvertListItem* GameStateManager::listGames()
{
    if (status & GAME_STATE_STATUS_ADVERTISING)
        status &= ~(GAME_STATE_STATUS_ADVERTISING);

    if (!(status & GAME_STATE_STATUS_LISTING_GAMES))
    {
        // clean host list, there should only ever be one, but lets future proof code just in case.
        DMESG("CLEAR");
        clearList((PktArcadeDevice**)&host);
        DMESG("ADD");
        // sniff all packets by becoming a remote host in broadcast mode (addressing is done by class rather than device address).
        addToList((PktArcadeDevice**)&host, new PktArcadeHost(PktDevice(0, 0, PKT_DEVICE_FLAGS_REMOTE | PKT_DEVICE_FLAGS_BROADCAST, this->serial_number), 0, *this));

        this->status |= GAME_STATE_STATUS_LISTING_GAMES;
    }

    return host->getGamesList();
}

int GameStateManager::addPlayer(PktDevice player)
{
    return addToList((PktArcadeDevice**)&players, new PktArcadePlayer(player, 0, *this));
}

int GameStateManager::addSpectator(PktDevice player)
{
    return addToList((PktArcadeDevice**)&players, new PktArcadePlayer(player, 0, *this));
}

int GameStateManager::joinGame(GameAdvertListItem* advert)
{
    if (!(status & GAME_STATE_STATUS_LISTING_GAMES) || host == NULL)
        return DEVICE_CANCELLED;

    // take a local copy of the ad before we delete current host
    GameAdvertisement ad = *(advert->item);
    uint32_t serial = advert->serial_number;

    // create a remote host, delete current host (will include the deletion of the games list)
    clearList((PktArcadeDevice**)&host);
    addToList((PktArcadeDevice**)&host, new PktArcadeHost(PktDevice(0, 0, PKT_DEVICE_FLAGS_REMOTE, serial), 0, *this));

    // create ourselves as a local driver
    clearList((PktArcadeDevice**)&players);
    addToList((PktArcadeDevice**)&players, new PktArcadePlayer(PktDevice(0, 0, PKT_DEVICE_FLAGS_LOCAL, this->serial_number), 0, *this));

    // join
    this->players->join(&ad);

    return DEVICE_OK;
}

void GameStateManager::processPacket(PktSerialPkt* p)
{
    // do some stuff with the packet...
    GameStatePacket* gsp = (GameStatePacket *)p->data;

    if (gsp->type = GAME_STATE_PKT_TYPE_INITIAL_SPRITE_DATA)
    {
        int spritesPerPacket = GAME_STATE_PKT_DATA_SIZE / sizeof(InitialSpriteData);

        InitialSpriteData* isd = (InitialSpriteData *)gsp->data;
        for (int i = 0; i < spritesPerPacket; i++)
        {
            for (int j = 0; j < GAME_ENGINE_MAX_SPRITES; j++)
            {
                // an empty sprite requires no action.
                if (engine.sprites[j] == NULL)
                    continue;

                // if match perform state sync.
                if (engine.sprites[j]->getHash() == isd[i].sprite_id)
                {
                    engine.sprites[j]->setX(isd[i].x);
                    engine.sprites[j]->setY(isd[i].y);
                }
            }
        }
    }
}