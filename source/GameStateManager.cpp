#include "GameStateManager.h"
#include "CodalDmesg.h"
#include "Timer.h"

using namespace codal;

int GameStateManager::clearHostList()
{
    PktArcadeHost* head = this->host;

    if (head == NULL)
        return DEVICE_OK;

    PktArcadeHost* temp = NULL;

    while (head)
    {
        temp = head;
        head = (PktArcadeHost *)temp->next;
        delete temp;
    }

    this->host = NULL;

    return DEVICE_OK;
}

int GameStateManager::clearPlayerList()
{
    PktArcadePlayer* head = this->players;

    if (head == NULL)
        return DEVICE_OK;

    PktArcadePlayer* temp = NULL;

    while (head)
    {
        temp = head;
        head = (PktArcadePlayer *)temp->next;
        delete temp;
    }

    this->players = NULL;

    return DEVICE_OK;
}

int GameStateManager::addHostToList(PktArcadeHost* item)
{
    PktArcadeHost* iterator = this->host;
    item->next = NULL;

    if (iterator == NULL)
        this->host = item;
    else
    {
        while (iterator->next)
            iterator = (PktArcadeHost *)iterator->next;

        iterator->next = item;
    }

    return DEVICE_OK;
}

int GameStateManager::addPlayerToList(PktArcadePlayer* item)
{
    PktArcadePlayer* iterator = this->players;
    item->next = NULL;

    if (iterator == NULL)
        this->players = item;
    else
    {
        while (iterator->next)
            iterator = (PktArcadePlayer *)iterator->next;

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
        clearHostList();
        addHostToList(new PktArcadeHost(PktDevice(0, 0, PKT_DEVICE_FLAGS_LOCAL | PKT_DEVICE_FLAGS_BROADCAST, this->serial_number), 0, *this));

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
        clearHostList();
        // sniff all packets by becoming a remote host in broadcast mode (addressing is done by class rather than device address).
        addHostToList(new PktArcadeHost(PktDevice(0, 0, PKT_DEVICE_FLAGS_BROADCAST, this->serial_number), 0, *this));

        this->status |= GAME_STATE_STATUS_LISTING_GAMES;
    }

    return host->getGamesList();
}


void GameStateManager::playerConnectionChange(PktArcadePlayer* player, bool connected)
{
    if (player == NULL)
        return;

    if (connected)
        Event(this->id, GAME_STATE_MANAGER_EVT_PLAYER_CONNECTED);
    else
    {
        // for now let's just delete the player
        // in the future we can add connection recovery etc.
        if (this->players == player)
        {
            this->players = (PktArcadePlayer *)player->next;
            delete player;
        }
        else
        {
            PktArcadePlayer* iterator = this->players;
            PktArcadePlayer* previous = this->players;

            while(iterator->next != player)
            {
                previous = iterator;
                iterator = (PktArcadePlayer *)iterator->next;
            }

            previous->next = (PktArcadePlayer *)iterator->next;
            delete iterator;
        }

        Event(this->id, GAME_STATE_MANAGER_EVT_PLAYER_DISCONNECTED);
    }
}

void GameStateManager::spectatorConnectionChange(PktArcadePlayer* player, bool connected)
{
    // nop
}

void GameStateManager::hostConnectionChange(PktArcadeHost* hostDriver, bool connected)
{
    if (hostDriver == NULL)
        return;

    // if we're hosting a game, this indicates that we're advertising.
    if (connected)
        Event(this->id, GAME_STATE_MANAGER_EVT_HOST_CONNECTED);
    else
    {
        // for now let's just delete the host
        // in the future we can add connection recovery etc.
        if (this->host == hostDriver)
        {
            this->host = (PktArcadeHost *)hostDriver->next;
            delete hostDriver;
        }
        else
        {
            PktArcadeHost* iterator = this->host;
            PktArcadeHost* previous = this->host;

            while(iterator->next != hostDriver)
            {
                previous = iterator;
                iterator = (PktArcadeHost *)iterator->next;
            }

            previous->next = iterator->next;
            delete iterator;
        }

        status &= ~GAME_STATE_STATUS_JOINED;
        Event(this->id, GAME_STATE_MANAGER_EVT_HOST_DISCONNECTED);
    }

}

int GameStateManager::addPlayer(PktDevice player)
{
    return addPlayerToList(new PktArcadePlayer(player, 0, *this));
}

int GameStateManager::addSpectator(PktDevice player)
{
    return addPlayerToList(new PktArcadePlayer(player, 0, *this));
}

int GameStateManager::joinGame(GameAdvertListItem* advert)
{
    if (!(status & (GAME_STATE_STATUS_LISTING_GAMES | GAME_STATE_STATUS_JOINED)) || host == NULL)
        return DEVICE_CANCELLED;

    status &= ~GAME_STATE_STATUS_LISTING_GAMES;

    // take a local copy of the ad before we delete current host
    GameAdvertisement ad = *(advert->item);
    uint32_t serial = advert->serial_number;

    // create a remote host, delete current host (will include the deletion of the games list)
    clearHostList();
    addHostToList(new PktArcadeHost(PktDevice(0, 0, PKT_DEVICE_FLAGS_REMOTE, serial), 0, *this));

    // create ourselves as a local driver
    clearPlayerList();
    addPlayerToList(new PktArcadePlayer(PktDevice(0, 0, PKT_DEVICE_FLAGS_LOCAL, this->serial_number), 0, *this));

    // join
    int ret = this->players->join(&ad);

    if (ret == DEVICE_OK)
        status |= GAME_STATE_STATUS_JOINED;

    return ret;
}

void GameStateManager::processPacket(PktSerialPkt* p)
{
    // do some stuff with the packet...
    DMESG("STD PKT RECEIVED");
    GameStatePacket* gsp = (GameStatePacket *)p->data;

    if (gsp->type == GAME_STATE_PKT_TYPE_INITIAL_SPRITE_DATA)
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