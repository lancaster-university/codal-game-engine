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

    /**
     * This class manages new players, detecting and joining hosts, and the synchronisation of game state.
     * It has a reference to the currently running game.
     **/
    class GameStateManager : public CodalComponent
    {
        GameEngine& engine; // the game that this class is managing.
        PktArcadeHost* host; // the list of connected hosts (should only ever be one.)
        PktArcadePlayer* players; // the list of connected players.

        uint32_t serial_number; // the serial number to use when advertising or joining a game.

        /**
         * Walks the given list, deleting entries, finally nullifying the list at the end.
         *
         * @param list the list to walk
         *
         * @returns DEVICE_OK on success.
         **/
        int clearList(PktArcadeDevice** list);

        /**
         * Walks to the end of the given list and adds the item.
         *
         * @param list the list to walk
         *
         * @param the item to add
         *
         * @returns DEVICE_OK on success.
         **/
        int addToList(PktArcadeDevice** list, PktArcadeDevice* item);

        public:

        /**
         * Constructor
         *
         * @param ge the game engine to manage
         *
         * @param serial the serial number (identifier) to use when advertising or joining a game.
         **/
        GameStateManager(GameEngine& ge, uint32_t serial, uint16_t id = DEVICE_ID_PKT_GAME_STATE_DRIVER);

        /**
         * Adds a player to the player list.
         *
         * @param d the device information for the new player.
         *
         * @returns DEVICE_OK on success
         **/
        int addPlayer(PktDevice d);

        /**
         * Adds a spectator to the player list.
         *
         * @param d the device information for the new player.
         *
         * @returns DEVICE_OK on success
         **/
        int addSpectator(PktDevice d);

        /**
         * Games are shared by calling hostGame(), this creates a game host, and begins advertising using data from engine.
         *
         * @returns DEVICE_OK on success.
         **/
        int hostGame();

        /**
         * listGames() creates a host, but in sniffing mode. All advertisements are captured, and added to a list of GameAdvertListItems, after 500 ms the thread will be woken up.
         *
         * @returns a list of GameAdvertListItems. NULL if no games are found.
         *
         * @note beginning to host a game or join a game will invalidate this list (it will be free'd)
         **/
        GameAdvertListItem* listGames();

        /**
         * Joins a game based on the name of the game
         *
         * @param advert the advert of the game to join. A list of adverts can be obtained from the listGames member function.
         *
         * @returns DEVICE_OK on success, DEVICE_CANCELLED if not listing games, DEVICE_NO_RESOURCES if a game is not found
         *
         * @note beginning to host a game or join a game will invalidate this list (it will be free'd)
         **/
        int joinGame(GameAdvertisement* advert);

        /**
         * Any player or host that receives data will pass the received packet to this class where it can be disseminated into game state.
         *
         * @param name the name of the game to join.
         **/
        void processPacket(PktSerialPkt* pkt);

        friend class PktArcadePlayer;
        friend class PktArcadeHost;
    };
}

#endif