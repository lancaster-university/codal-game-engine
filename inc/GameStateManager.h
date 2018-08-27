#ifndef GAME_STATE_MANAGER_H
#define GAME_STATE_MANAGER_H

#include "PktSerialProtocol.h"
#include "ManagedString.h"
#include "GameEngine.h"

#define GS_CONTROL_PKT_TYPE_ADVERTISEMENT           0x02
// flags mean something different in a advert packet
#define GS_CONTROL_FLAG_SPECTATORS_ALLOWED          0x01    // indicates whether spectators are allowed
#define GS_CONTROL_FLAG_GAME_IN_PROGRESS            0x02    // indicates whether the game has begun or not
#define GS_CONTROL_FLAG_JOIN_ACK                    0x04    // used to indicate a successful join.

#define GS_CONTROL_PKT_TYPE_JOIN                    0x03
// flags mean something different in a join packet
#define GS_CONTROL_FLAG_PLAY                        0x01
#define GS_CONTROL_FLAG_SPECTATE                    0x02

#define GS_STATUS_ADVERTISE                         0x02
#define GS_STATUS_SPECTATE                          0x04
#define GS_STATUS_JOINING                           0x08
#define GS_STATUS_JOINED                            0x10

#define GS_EVENT_JOIN_TIMEOUT                       2
#define GS_EVENT_JOIN_SUCCESSFUL                    3
#define GS_EVENT_JOIN_FAILED                        4
#define GS_EVENT_SEND_GAME_STATE                    5

#define GAME_STATE_NAME_SIZE                        13

#define GAME_STATE_STATUS_LISTING_GAMES             0x02
#define GAME_STATE_STATUS_ADVERTISING               0x04

#define PKT_ARCADE_EVT_PLAYER_JOIN_RESULT           2
#define PKT_ARCADE_PLAYER_STATUS_JOIN_SUCCESS       0x02

namespace codal
{
    struct GameStatePacket
    {
        uint8_t type;
        uint8_t owner;
        uint8_t data[30];
    };

    struct InitialSpriteData
    {
        uint16_t sprite_id;     // the id of a sprite
        int16_t x;
        int16_t y;
    };

    struct InProgressSpriteData
    {
        uint16_t sprite_id;     // the id of a sprite
        int16_t x;
        int16_t y;
        float dx;               // xvelocity
        float dy;               // yvelocity
    };

    struct CollisionData
    {
        uint16_t sprite_id;     // the id of a sprite
        float dx;
        float dy;

        uint16_t collision_sprite_id;     // the id of a sprite
        float cdx;
        float cdy;
    };

    struct GameAdvertisement
    {
        uint32_t game_id;           // used to identify different games
        uint8_t slots_available;    // the availability of the player slots in the game, (0 == no player slots available)
        uint8_t max_players;        // slots_available
        uint8_t game_name[GAME_STATE_NAME_SIZE + 1];      // a textual name, must include null terminator (therefore 13 chars for a name)
    };

    struct GameAdvertListItem
    {
        uint32_t serial_number;
        GameAdvertisement* item;
        GameAdvertListItem* next;
    };

    class GameStateManager;

    class PktArcadeDevice : public PktSerialDriver
    {
        public:
        uint8_t playerNumber;
        PktArcadeDevice* next;
        GameStateManager& manager;

        PktArcadeDevice(PktDevice d, uint8_t playerNumber, GameStateManager& manager, uint16_t id) :
                PktSerialDriver(d, PKT_DRIVER_CLASS_ARCADE, id),
                manager(manager)
        {
            playerNumber = playerNumber;
            next = NULL;
        }

        virtual void handleControlPacket(ControlPacket* cp) {}

        virtual void handlePacket(PktSerialPkt* p);
    };

    class PktArcadeHost : public PktArcadeDevice
    {
        GameAdvertListItem* games;

        public:
        PktArcadeHost(PktDevice d, uint8_t playerNumber, GameStateManager& manager);

        virtual int queueControlPacket();

        virtual void handleControlPacket(ControlPacket* cp);

        GameAdvertListItem* getGamesList();

        ~PktArcadeHost();
    };

    class PktArcadePlayer : public PktArcadeDevice
    {
        int sendJoinSpectateRequest(GameAdvertisement* ad, uint16_t flags);

        public:
        PktArcadePlayer(PktDevice d, uint8_t playerNumber, GameStateManager& manager);

        virtual void handleControlPacket(ControlPacket* cp);

        int join(GameAdvertisement* ad);

        int spectate(GameAdvertisement* ad);
    };

    class GameStateManager : public CodalComponent
    {
        GameEngine& engine;
        PktArcadeHost* host;
        PktArcadePlayer* players;

        uint32_t serial_number;

        int clearList(PktArcadeDevice** list);
        int addToList(PktArcadeDevice** list, PktArcadeDevice* item);

        public:
        GameStateManager(GameEngine& ge, uint32_t serial, uint16_t id = DEVICE_ID_PKT_GAME_STATE_DRIVER);

        int addPlayer(PktDevice d);

        int addSpectator(PktDevice d);

        int hostGame();

        int listGames();

        int joinGame(ManagedString name);

        void processPacket(PktSerialPkt* pkt);

        friend class PktArcadePlayer;
        friend class PktArcadeHost;
    };
}

#endif