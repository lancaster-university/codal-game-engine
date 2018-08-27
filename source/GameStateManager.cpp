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
        status &= ~(GAME_STATE_STATUS_LISTING_GAMES);

    // clean host list, there should only ever be one, but lets future proof code just in case.
    clearList((PktArcadeDevice**)&host);
    addToList((PktArcadeDevice**)&host, new PktArcadeHost(PktDevice(0, 0, PKT_DEVICE_FLAGS_LOCAL, this->serial_number), 0, *this));

    this->status |= GAME_STATE_STATUS_ADVERTISING;
    return DEVICE_OK;
}

int GameStateManager::listGames()
{
    if (status & GAME_STATE_STATUS_ADVERTISING)
        status &= ~(GAME_STATE_STATUS_ADVERTISING);

    // clean host list, there should only ever be one, but lets future proof code just in case.
    clearList((PktArcadeDevice**)&host);
    // sniff all packets by becoming a remote host in broadcast mode (addressing is done by class rather than device address).
    addToList((PktArcadeDevice**)&host, new PktArcadeHost(PktDevice(0, 0, PKT_DEVICE_FLAGS_REMOTE | PKT_DEVICE_FLAGS_BROADCAST, this->serial_number), 0, *this));

    this->status |= GAME_STATE_STATUS_LISTING_GAMES;
    return DEVICE_OK;
}

int GameStateManager::addPlayer(PktDevice player)
{
    return addToList((PktArcadeDevice**)&players, new PktArcadePlayer(player, 0, *this));
}

int GameStateManager::addSpectator(PktDevice player)
{
    return addToList((PktArcadeDevice**)&players, new PktArcadePlayer(player, 0, *this));
}

int GameStateManager::joinGame(ManagedString name)
{
    if (!(status & GAME_STATE_STATUS_LISTING_GAMES) || host == NULL)
        return DEVICE_CANCELLED;

    GameAdvertListItem* advert = host->getGamesList();
    GameAdvertisement* game = NULL;

    // walk the list of games, comparing the names, if match is found set game pointer
    while(advert)
    {
        if (name == ManagedString((char *)advert->item->game_name))
        {
            game = advert->item;
            break;
        }

        advert = advert->next;
    }

    // no match -- game gone?
    if (game == NULL)
        return DEVICE_NO_RESOURCES;

    // we have a match
    // take a local copy of the ad before we delete current host
    GameAdvertisement ad = *game;
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
}