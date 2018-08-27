#include "GameStateManager.h"
#include "CodalDmesg.h"
#include "Timer.h"

using namespace codal;

PktArcadePlayer::PktArcadePlayer(PktDevice d, uint8_t playerNumber, GameStateManager& manager) :
               PktArcadeDevice(d, playerNumber, manager, DEVICE_ID_PKT_ARCADE_PLAYER)
{
}

void PktArcadePlayer::handleControlPacket(ControlPacket* cp)
{
    if (cp->packet_type == GS_CONTROL_PKT_TYPE_JOIN)
    {
        if (cp->serial_number == device.serial_number)
        {
            // we've been accepted...
            if (cp->flags & GS_CONTROL_FLAG_JOIN_ACK)
            {
                status |= PKT_ARCADE_PLAYER_STATUS_JOIN_SUCCESS;
                Event(DEVICE_ID_NOTIFY_ONE, PKT_ARCADE_EVT_PLAYER_JOIN_RESULT);
                return;
            }

            // we've received a response without an ack, i.e. rejected
            Event(DEVICE_ID_NOTIFY_ONE, PKT_ARCADE_EVT_PLAYER_JOIN_RESULT);
            return;
        }
    }
}

int PktArcadePlayer::sendJoinSpectateRequest(GameAdvertisement* ad, uint16_t flags)
{
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

    if (!(status & PKT_ARCADE_PLAYER_STATUS_JOIN_SUCCESS))
        return DEVICE_CANCELLED;

    return DEVICE_OK;
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