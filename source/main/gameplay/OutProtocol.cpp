/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2016 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#include "OutProtocol.h"

#include "Application.h"
#include "BeamEngine.h"
#include "BeamFactory.h"
#include "RoRVersion.h"
#include "Settings.h"

#include <Ogre.h>

using namespace Ogre;
using namespace RoR;

OutProtocol::OutProtocol(void) :
    delay(0.1f)
    , id(0)
    , mode(0)
    , sockfd(-1)
    , timer(0)
    , working(false)
{
    delay *= App::io_outgauge_delay.GetActive(); // TODO: Use the GVar directly, don't copy it.
    mode = App::io_outgauge_mode.GetActive();    // TODO: Use the GVar directly, don't copy it.
    id = App::io_outgauge_id.GetActive();        // TODO: Use the GVar directly, don't copy it.

    if (mode > 0)
    {
        startup();
    }
}

OutProtocol::~OutProtocol(void)
{
    if (sockfd != 0)
    {
#ifdef USE_SOCKETW
#   if _WIN32
        closesocket(sockfd);
#   else
        close( sockfd );
#   endif
#endif // USE_SOCKETW
        sockfd = 0;
    }
}

void OutProtocol::startup()
{
#if defined(_WIN32) && defined(USE_SOCKETW)
    SWBaseSocket::SWBaseError error;

    // startup winsock
    WSADATA wsd;
    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
    {
        LOG("[RoR|OutGauge] Error starting up winsock. OutGauge disabled.");
        return;
    }

    // open a new socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        LOG(String("[RoR|OutGauge] Error creating socket for OutGauge: ").append(strerror(errno)));
        return;
    }

    // get the IP of the remote side, this function is compatible with windows 2000
    hostent* remoteHost = gethostbyname(App::io_outgauge_ip.GetActive());
    char* ip = inet_ntoa(*(struct in_addr *)*remoteHost->h_addr_list);

    // init socket data
    struct sockaddr_in sendaddr;
    memset(&sendaddr, 0, sizeof(sendaddr));
    sendaddr.sin_family = AF_INET;
    sendaddr.sin_addr.s_addr = inet_addr(ip);
    sendaddr.sin_port = htons(App::io_outgauge_port.GetActive());

    // connect
    if (connect(sockfd, (struct sockaddr *) &sendaddr, sizeof(sendaddr)) == SOCKET_ERROR)
    {
        LOG(String("[RoR|OutGauge] Error connecting socket for OutGauge: ").append(strerror(errno)));
        return;
    }

    LOG("[RoR|OutGauge] Connected successfully");
    working = true;
#else
    // TODO: fix linux
#endif // _WIN32
}

bool OutProtocol::Update(float dt, Actor* truck)
{
#if defined(_WIN32) && defined(USE_SOCKETW)
    if (!working)
    {
        return false;
    }

    // below the set delay?
    timer += dt;
    if (timer < delay)
    {
        return true;
    }
    timer = 0;

    // send a package
    OutGaugePack gd;
    memset(&gd, 0, sizeof(gd));

    // set some common things
    gd.Time = Root::getSingleton().getTimer()->getMilliseconds();
    gd.ID = id;
    gd.Flags = 0 | OG_KM;
    sprintf(gd.Car, "RoR");

    if (!truck)
    {
        // not in a truck?
        sprintf(gd.Display2, "not in vehicle");
    }
    else if (truck && !truck->ar_engine)
    {
        // no engine?
        sprintf(gd.Display2, "no engine");
    }
    else if (truck && truck->ar_engine)
    {
        // truck and engine valid
        if (truck->ar_engine->HasTurbo())
        {
            gd.Flags |= OG_TURBO;
        }
        gd.Gear = std::max(0, truck->ar_engine->GetGear() + 1); // we only support one reverse gear
        gd.PLID = 0;
        gd.Speed = fabs(truck->ar_wheel_speed);
        gd.RPM = truck->ar_engine->GetEngineRpm();
        gd.Turbo = truck->ar_engine->GetTurboPsi() * 0.0689475729f;
        gd.EngTemp = 0; // TODO
        gd.Fuel = 0; // TODO
        gd.OilPressure = 0; // TODO
        gd.OilTemp = 0; // TODO

        gd.DashLights = 0;
        gd.DashLights |= DL_HANDBRAKE;
        gd.DashLights |= DL_BATTERY;
        gd.DashLights |= DL_SIGNAL_L;
        gd.DashLights |= DL_SIGNAL_R;
        gd.DashLights |= DL_SIGNAL_ANY;
        if (!truck->tc_nodash)
            gd.DashLights |= DL_TC;
        if (!truck->alb_nodash)
            gd.DashLights |= DL_ABS;

        gd.ShowLights = 0;
        if (truck->ar_parking_brake)
            gd.ShowLights |= DL_HANDBRAKE;
        if (truck->ar_lights)
            gd.ShowLights |= DL_FULLBEAM;
        if (truck->ar_engine->HasStarterContact() && !truck->ar_engine->IsRunning())
            gd.ShowLights |= DL_BATTERY;
        if (truck->ar_left_blink_on)
            gd.ShowLights |= DL_SIGNAL_L;
        if (truck->ar_right_blink_on)
            gd.ShowLights |= DL_SIGNAL_R;
        if (truck->ar_warn_blink_on)
            gd.ShowLights |= DL_SIGNAL_ANY;
        if (truck->tc_mode)
            gd.ShowLights |= DL_TC;
        if (truck->alb_mode)
            gd.ShowLights |= DL_ABS;

        gd.Throttle = truck->ar_engine->GetAcceleration();
        gd.Brake = truck->ar_brake / truck->ar_brake_force;
        gd.Clutch = 1 - truck->ar_engine->GetClutch(); // 0-1

        strncpy(gd.Display1, truck->ar_design_name.c_str(), 15);
        if (truck->ar_design_name.length() > 15)
        {
            strncpy(gd.Display2, truck->ar_design_name.c_str() + 15, 15);
        }
    }
    // send the package
    send(sockfd, (const char*)&gd, sizeof(gd), NULL);

    return true;
#else
    // TODO: fix linux
	return false;
#endif // _WIN32
}
