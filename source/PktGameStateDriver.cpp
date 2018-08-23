#include "PktGameStateDriver.h"
#include "CodalDmesg.h"
#include "Timer.h"

using namespace codal;

int PktGameStateDriver::queueControlPacket()
{
    PKT_DMESG("QUEUED CUSTOM CP");
    ControlPacket cp;
    memset(&cp, 0, sizeof(ControlPacket));

    cp.packet_type = CONTROL_PKT_TYPE_HELLO;
    cp.address = device.address;
    cp.flags = 0;
    cp.driver_class = this->driver_class;
    cp.serial_number = device.serial_number;

    if ((this->status & GAME_STATE_STATUS_ADVERTISE))
    {
        cp.packet_type = GS_CONTROL_PKT_TYPE_ADVERTISEMENT;

        GameStateControl* gscp = (GameStateControl*)cp.data;
        cp.flags |= GAME_SPECTATORS_ALLOWED;

        if (engine.isRunning())
            cp.flags |= GAME_STATE_IN_PROGRESS;

        gscp->max_players = engine.getMaxPlayers();
        gscp->slots_available = engine.getAvailableSlots();

        ManagedString name = engine.getName();
        memcpy(gscp->name, name.getCharArray(), min(name.length(), GAME_STATE_NAME_SIZE));
        // set the null terminator
        gcsp->name[GAME_STATE_NAME_SIZE] = 0;
    }

    PktSerialProtocol::send((uint8_t *)&cp, sizeof(ControlPacket), 0);

    return DEVICE_OK;
}

PktGameStateDriver::PktGameStateDriver(GameEngine& ge) :
    PktSerialDriver(PktDevice(0, 0, PKT_DEVICE_FLAGS_LOCAL, 0),
                    PKT_DRIVER_CLASS_GAME_STATE,
                    DEVICE_ID_PKT_GAME_STATE_DRIVER),
    engine(ge),
    host(NULL),
    players(NULL)
{
    // if (EventModel::defaultEventBus)
        // EventModel::defaultEventBus->listen(n.id, RADIO_EVT_DATA_READY, this, &PktGameStateDriver::forwardPacket);
}

int PktGameStateDriver::sendAdvertisements(bool advertise)
{
    this->host = new PktArcadeHost()
    if (advertise)
        this->status |= GAME_STATE_STATUS_ADVERTISE;
    else
        this->status &= ~GAME_STATE_STATUS_ADVERTISE;

    return DEVICE_OK;
}

void PktGameStateDriver::handleControlPacket(ControlPacket* cp)
{
    GameStateControl* gscp = (GameStateControl*)cp->data;

    // if advert packet received from other game
    if (cp->packet_type == GS_CONTROL_PKT_TYPE_ADVERTISEMENT)
    {
        // check id
        // for now lets just join, we can list games in the future...
        // game identifiers must match
        if (gscp->game_id == engine.getIdentifier() && this->device.flags & PKT_DEVICE_FLAGS_REMOTE)
        {
            cp->packet_type = GS_CONTROL_PKT_TYPE_JOIN;
            cp->address = device.address;
            cp->driver_class = this->driver_class;
            cp->serial_number = device.serial_number;
            cp->flags = 0;

            if (gcsp->slots_available > 0)
                cp->flags |= GS_CONTROL_FLAG_PLAY;
            else if(cp->flags & GS_CONTROL_FLAG_SPECTATORS_ALLOWED)
                cp->flags |= GS_CONTROL_FLAG_SPECTATE;
            else
                return;

            system_timer_event_after(500, DEVICE_ID_PKT_GAME_STATE_DRIVER_INTERNAL, GS_EVENT_JOIN_TIMEOUT)
        }
    }

    if (cp->packet_type == GS_CONTROL_PKT_TYPE_JOIN)
    {
        // if we're advertising and receive a join request.
        if (status & GS_STATUS_ADVERTISE && gscp->game_id == engine.getIdentifier() && ManagedString(gscp->game_name) == engine.getName())
        {
            // if we have a player slot available and the arcade has requested to play...
            // if arcade has requested to spectate and we allow spectate mode
            if ((engine.getAvailableSlots() > 0 && cp->flags & GS_CONTROL_FLAG_PLAY) || (this->status & GS))
            {
                // communicate game state
                // TODO: add identifier to some list...?
                Event(DEVICE_ID_PKT_GAME_STATE_DRIVER_INTERNAL, GS_EVENT_COMMUNICATE_GAME_STATE);
                cp->flags |= GS_CONTROL_FLAG_JOIN_ACK;
            }
        }
        else if (status & GS_STATUS_JOINING && cp->serial_number == device.serial_number)
        {
            status &= ~(GS_STATUS_JOINING);
            // we've been accepted...
            if (cp->flags & GS_CONTROL_FLAG_JOIN_ACK)
            {
                status |= GS_STATUS_JOINED;
                Event(this->id, GS_EVENT_JOIN_SUCCESSFUL);
                return;
            }

            // we've received a response without an ack, i.e. rejected
            Event(this->id, GS_EVENT_JOIN_FAILED);
            return;
        }
    }

    PktSerialProtocol::send((uint8_t *)&cp, sizeof(ControlPacket), 0);
}

void PktGameStateDriver::onGameStateEvent(Event e)
{
    if (e.value == GS_EVENT_SEND_GAME_STATE)
        sendGameState();

    if (e.value == GS_EVENT_JOIN_TIMEOUT)
    {
        status &= ~(GS_STATUS_JOINING);
        Event(this->id, GS_EVENT_JOIN_FAILED);
    }
}

void PktGameStateDriver::handlePacket(PktSerialPkt* p)
{
    uint32_t id = p->address << 16 | p->crc;

    DMESG("ID: %d",id);
    if (checkHistory(id))
        return;

    addToHistory(id);

    ManagedBuffer b((uint8_t*)p, PKT_SERIAL_PACKET_SIZE);
    int ret = networkInstance->sendBuffer(b);

    DMESG("ret %d",ret);
}