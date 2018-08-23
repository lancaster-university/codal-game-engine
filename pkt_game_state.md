# PKT Game State Driver

This class is responsible for the dissemination of game state between arcade compatible devices.

## Control Packets

Pkt serial has the concept of control packets --- packets that are broadcast every 500 ms (or more if required). The Game State driver uses these packets to look for an available game.

A control packet for the game state driver maintains the base payload structure:

```cpp
struct ControlPacket
{
    uint8_t packet_type;    // indicates the type of the packet, normally just HELLO
    uint8_t address;        // the address assigned by the logic driver
    uint16_t flags;         // various flags
    uint32_t driver_class;  // the class of the driver
    uint32_t serial_number; // the "unique" serial number of the device.
    uint8_t data[CONTROL_PACKET_PAYLOAD_SIZE];
};
```

In addition it specifies the following data payload:

```cpp

// bit fields used in the above flags field
#define GAME_SPECTATORS_ALLOWED         0x01    // indicates whether spectators are allowed
#define GAME_STATE_IN_PROGRESS          0x02    // indicates whether the game has begun or not
#define GAME_JOIN_NACK                  0x04    // used to indicate a failed join.

struct GameStateControl
{
    uint16_t game_id;           // used to identify different games
    uint8_t slots_available;    // the availability of the player slots in the game, (0 == no player slots available)
    uint8_t max_players;        // slots_available
    uint8_t game_name[16];      // a textual name, must include null terminator (therefore 15 chars for a name)
};
```

## Joining a game

The process is as follows:

1) Listen for control packets
2) If a control packet arrives:
    - check the game_hash
        - if the same, check the number of players
            - join if possible (whether as a spectator or player)
                - a join is performed by setting the packet type to join
                - the host receives the join request
                - if join is permitted, sends the same packet with the ack flag set
                - if join is not permitted, no ack flag is set.
                - on a successful join both should then store the control packet device address for future use. slots_available is decremented.

## State Synchronisation

After completing the join process, game state should be synchronised. This will involve communicating all sprite information (initial positions) in the game world and may take many packets. This might not be necessary as the initial state of all sprites is known on game start. The driver will communicate all of its controlled assets first, followed by assets controlled by other players. Packet format to be specified.


# Open questions

## To simulate or not to simulate?

There is a question around which model to choose to enable multiplayer gameplay:

* Full game state: All game state is communicated every display update. This would work really well for small games, but not for large games. In this scenario the main simulation is handled by a host device, other devices act as displays and inputs.
* Event-based updates: Game state is only communicated when something "interesting" happens. In this scenario initial state is communicated, and update events are broadcast.

I think the best approach is some combination of the two:

* every second a full game state is communicated by the host
* collisions of sprites, sprite creation, and player movements are handled using events. Each device will broadcast its events which could be use for consensus. This has the benefit of rather than communicating game state periodically, it's communicated when required.

## How is the host determined?

This could be set manually in code, or arcades could wake and check to see if control packets with a matching game hash are already being broadcast, and join that host. I think the best option is to make it selectable via some sort of menu, then multiple arcades could be running the same "game" but running different instances.

## How is sprite ownership handled? Is the concept of ownership necessary?

Individual arcades could own all sprites in the world, but this raises the question over who owns a player sprite. It would be rather simple to include an owner_id field in the game state synchronisation phase as a potential solution.



