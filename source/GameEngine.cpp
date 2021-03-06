#include "GameEngine.h"
#include "EventModel.h"
#include "Timer.h"
#include "CodalDmesg.h"
#include "Display.h"

using namespace codal;

GameEngine* GameEngine::instance = NULL;

GameEngine::GameEngine(Image& displayBuffer, ManagedString gameName, uint32_t identifier, int maxPlayers, uint16_t id) : displayBuffer(displayBuffer)
{
    DMESG("GE CONS");
    instance = this;

    memset(sprites, 0, GAME_ENGINE_MAX_SPRITES * sizeof(Sprite*));
    memset(playerSprites, 0, GAME_ENGINE_MAX_PLAYER_SPRITES * sizeof(Sprite*));

    this->status = GAME_ENGINE_STATUS_STOPPED;

    this->maxPlayers = maxPlayers;

    this->playerCount = 1;
    this->gameIdentifier = identifier;

    this->name = gameName;

    if (EventModel::defaultEventBus)
        EventModel::defaultEventBus->listen(DEVICE_ID_DISPLAY, DISPLAY_EVT_RENDER_START, this, &GameEngine::update, MESSAGE_BUS_LISTENER_IMMEDIATE);
}

int GameEngine::getMaxPlayers()
{
    return this->maxPlayers;
}

int GameEngine::getAvailableSlots()
{
    return this->maxPlayers - this->playerCount;
}

int GameEngine::start()
{
    this->status = GAME_ENGINE_STATUS_IN_PROGRESS;
    return DEVICE_OK;
}

int GameEngine::stop()
{
    this->status = GAME_ENGINE_STATUS_STOPPED;
    return DEVICE_OK;
}

int GameEngine::reset()
{
    for (int i = 0; i < GAME_ENGINE_MAX_SPRITES; i++)
    {
        if (sprites[i] == NULL)
            continue;

        sprites[i]->reset();
    }

    return DEVICE_OK;
}

bool GameEngine::isRunning()
{
    return (this->status & GAME_ENGINE_STATUS_IN_PROGRESS) ? true : false;
}

ManagedString GameEngine::getName()
{
    return this->name;
}

uint32_t GameEngine::getIdentifier()
{
    return this->gameIdentifier;
}

int GameEngine::setDisplayBuffer(Image& i)
{
    this->displayBuffer = i;
    return DEVICE_OK;
}

int GameEngine::add(Sprite& s)
{
    int i = 0;
    for (i = 0; i < GAME_ENGINE_MAX_SPRITES; i++)
    {
        if (sprites[i] == NULL)
        {
            sprites[i] = &s;
            break;
        }
    }

    if (i == GAME_ENGINE_MAX_SPRITES)
        return DEVICE_NO_RESOURCES;

    return DEVICE_OK;
}

int GameEngine::remove(Sprite& s)
{
    int i = 0;
    for (i = 0; i < GAME_ENGINE_MAX_SPRITES; i++)
    {
        if (sprites[i] == &s)
        {
            sprites[i] = NULL;
            break;
        }
    }

    if (i == GAME_ENGINE_MAX_SPRITES)
        return DEVICE_INVALID_PARAMETER;

    return DEVICE_OK;
}

void GameEngine::update(Event)
{
    displayBuffer.clear();

    // the engine can be stopped, but sprites can still be drawn.
    if (!(status & GAME_ENGINE_STATUS_STOPPED))
    {
        for (int i = 0; i < GAME_ENGINE_MAX_SPRITES; i++)
        {
            if (sprites[i] == NULL)
                continue;

            sprites[i]->body.apply();
        }

        for (int i = 0; i < GAME_ENGINE_MAX_SPRITES; i++)
        {
            if (sprites[i] == NULL)
                continue;

            for (int j = i + 1; j < GAME_ENGINE_MAX_SPRITES; j++)
            {
                if (sprites[j] == NULL || sprites[j] == sprites[i])
                    continue;

                if (sprites[i]->body.intersectsWith(sprites[j]->body))
                    sprites[i]->body.collideWith(sprites[j]->body);
            }
        }
    }

    for (int i = 0; i < GAME_ENGINE_MAX_SPRITES; i++)
        sprites[i]->draw(displayBuffer);

    Event(DEVICE_ID_GAME_ENGINE, GAME_ENGINE_EVT_UPDATE);
}

