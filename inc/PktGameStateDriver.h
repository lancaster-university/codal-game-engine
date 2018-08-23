#ifndef PKT_BRIDGE_DRIVER_H
#define PKT_BRIDGE_DRIVER_H

#include "PktSerialProtocol.h"
#include "MessageBus.h"
#include "Radio.h"
#include "ManagedBuffer.h"

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

    struct GameStateControl
    {
        uint32_t game_id;           // used to identify different games
        uint8_t slots_available;    // the availability of the player slots in the game, (0 == no player slots available)
        uint8_t max_players;        // slots_available
        uint8_t game_name[GAME_STATE_NAME_SIZE + 1];      // a textual name, must include null terminator (therefore 13 chars for a name)
    };

    class PktArcadeDevice : public PktSerialDriver
    {
        public:
        uint8_t playerNumber;
        PktArcadeDevice* next;

        PktArcadeDevice(PktDevice d, uint8_t playerNumber, GameEngine& ge);

        virtual void handleControlPacket(ControlPacket* cp);

        virtual void handlePacket(PktSerialPkt* p);
    };

    class PktArcadeHost : public PktArcadeDevice
    {
        PktArcadeHost(PktDevice d, uint8_t playerNumber, GameEngine& ge);

        virtual void handleControlPacket(ControlPacket* cp);

        virtual void handlePacket(PktSerialPkt* p);
    };

    class PktArcadePlayer : public PktArcadeDevice
    {
        PktArcadeHost(PktDevice d, uint8_t playerNumber, GameEngine& ge);

        virtual void handleControlPacket(ControlPacket* cp);

        virtual void handlePacket(PktSerialPkt* p);
    };


    class PktGameStateDriver : public PktSerialDriver
    {
        GameEngine& engine;
        EventModel& messageBus;
        PktArcadeDevice* host;
        PktArcadeDevice* players;

        public:
        PktGameStateDriver(PktSerialProtocol& proto, GameEngine& ge, EventModel& messageBus, uint16_t id = DEVICE_ID_PKT_GAME_STATE_DRIVER);

        int sendAdvertisements(bool advertise);

        int sendGameState();

        virtual int queueControlPacket();

        virtual void handleControlPacket(ControlPacket* cp);

        virtual void handlePacket(PktSerialPkt* p);
    };
}

#endif