#ifndef CODAL_GAME_ENGINE
#define CODAL_GAME_ENGINE

#include "CodalComponent.h"
#include "CodalConfig.h"
#include "Sprite.h"
#include "Event.h"
#include "PhysicsBody.h"

#define GAME_ENGINE_MAX_SPRITES         20
#define GAME_ENGINE_EVT_UPDATE          2

#define GAME_ENGINE_STATUS_STOPPED      0x02
#define GAME_ENGINE_STATUS_IN_PROGRESS  0x04

namespace codal
{
    class GameEngine : public CodalComponent
    {
        Image& displayBuffer;

        int maxPlayers;
        int playerCount;

        uint32_t gameIdentifier;

        ManagedString name;

        public:
        Sprite* sprites[GAME_ENGINE_MAX_SPRITES];

        GameEngine(Image& displayBuffer, ManagedString gameName, uint32_t identifier, int maxPlayers = 1, uint16_t id = DEVICE_ID_GAME_ENGINE);

        uint32_t getIdentifier();

        int start();
        int stop();
        int reset();

        bool isRunning();

        int setDisplayBuffer(Image& i);

        ManagedString getName();

        int getMaxPlayers();
        int getAvailableSlots();

        int add(Sprite& s);
        int remove(Sprite& s);

        void update(Event);

        friend class GameStateManager;
    };
}

#endif