#include "GameStateManager.h"
#include "CodalDmesg.h"
#include "Timer.h"

using namespace codal;

PktArcadePlayer::PktArcadePlayer(PktDevice d, uint8_t playerNumber, GameStateManager& manager) :
               PktArcadeDevice(d, playerNumber, manager, DEVICE_ID_PKT_ARCADE_PLAYER)
{
    DMESG("PLAYER: addr %d flags %d sn %d",device.address, device.flags, device.serial_number);
    status = 0;
}

int PktArcadePlayer::handleControlPacket(ControlPacket* cp)
{
    if (this->device.flags & PKT_DEVICE_FLAGS_REMOTE)
        return DEVICE_OK;

    GameAdvertisement* advert = (GameAdvertisement*)cp->data;

    if (cp->packet_type == GS_CONTROL_PKT_TYPE_JOIN)
    {
        if (cp->serial_number == device.serial_number)
        {
            // we've been accepted...
            if (cp->flags & GS_CONTROL_FLAG_JOIN_ACK)
            {
                this->playerNumber = advert->playerNumber;
                Player::playerNumber = advert->playerNumber;

                findControllingSprites();

                if (EventModel::defaultEventBus)
                    EventModel::defaultEventBus->listen(DEVICE_ID_SPRITE, SPRITE_EVT_BASE + this->playerNumber, (PktArcadeDevice*)this, &PktArcadeDevice::updateSprite, MESSAGE_BUS_LISTENER_DROP_IF_BUSY);

                status |= PKT_ARCADE_PLAYER_STATUS_JOIN_SUCCESS;
                Event(DEVICE_ID_NOTIFY_ONE, PKT_ARCADE_EVT_PLAYER_JOIN_RESULT);
                return DEVICE_OK;
            }

            // we've received a response without an ack, i.e. rejected
            Event(DEVICE_ID_NOTIFY_ONE, PKT_ARCADE_EVT_PLAYER_JOIN_RESULT);
            return DEVICE_OK;
        }
    }

    return DEVICE_OK;
}

int PktArcadePlayer::deviceConnected(PktDevice d)
{
    PktSerialDriver::deviceConnected(d);
    manager.playerConnectionChange(this, true);
    DMESG("P C %d %d", this->device.address, this->playerNumber);
    return DEVICE_OK;
}

int PktArcadePlayer::deviceRemoved()
{
    PktSerialDriver::deviceRemoved();
    manager.playerConnectionChange(this, false);
    DMESG("P D/C %d %d", this->device.address, this->playerNumber);
    return DEVICE_OK;
}

int PktArcadePlayer::sendJoinSpectateRequest(GameAdvertisement* ad, uint16_t flags)
{
    if (!(this->device.flags & PKT_DEVICE_FLAGS_INITIALISED))
    {
        DMESG("NC");
        fiber_wake_on_event(this->id, PKT_DRIVER_EVT_CONNECTED);
        schedule();
    }

    ControlPacket cp;
    cp.packet_type = GS_CONTROL_PKT_TYPE_JOIN;
    cp.address = device.address;
    cp.driver_class = this->driver_class;
    cp.serial_number = device.serial_number;
    cp.flags = flags;

    // copy advert into control packet
    GameAdvertisement* gscp = (GameAdvertisement*)cp.data;
    memcpy(gscp, ad, sizeof(GameAdvertisement));

    // reset our status.
    status = 0;

    PktSerialProtocol::send((uint8_t *)&cp, sizeof(ControlPacket), 0);

    // configure timeout event
    system_timer_event_after(500, DEVICE_ID_NOTIFY_ONE, PKT_ARCADE_EVT_PLAYER_JOIN_RESULT);
    fiber_wake_on_event(DEVICE_ID_NOTIFY_ONE, PKT_ARCADE_EVT_PLAYER_JOIN_RESULT);
    schedule();

    if ((status & PKT_ARCADE_PLAYER_STATUS_JOIN_SUCCESS))
        return DEVICE_OK;

    return DEVICE_CANCELLED;
}

int PktArcadePlayer::spectate(GameAdvertisement* ad)
{
    uint16_t flags = GS_CONTROL_FLAG_SPECTATE;
    return sendJoinSpectateRequest(ad, flags);
}

int PktArcadePlayer::join(GameAdvertisement* ad)
{
    uint16_t flags = GS_CONTROL_FLAG_PLAY;
    return sendJoinSpectateRequest(ad, flags);
}