/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "OutProtocol.h"

#include "BeamEngine.h"
#include "BeamFactory.h"
#include "RoRVersion.h"
#include "Settings.h"

#include <Ogre.h>

using namespace Ogre;

OutProtocol::OutProtocol(void) : 
	  delay(0.1f)
	, id(0)
	, mode(0)
	, sockfd(-1)
	, timer(0)
	, working(false)
{
	delay *= FSETTING("OutGauge Delay", 10);
	mode   = ISETTING("OutGauge Mode", 0);
	id     = ISETTING("OutGauge ID", 0);

	if ( mode > 0 )
	{
		startup();
	}
}

OutProtocol::~OutProtocol(void)
{
	if ( sockfd != 0 )
	{
#if _WIN32
		closesocket( sockfd );
#else
		close( sockfd );
#endif
		sockfd = 0;
	}
}

void OutProtocol::startup()
{
#ifdef _WIN32
	SWBaseSocket::SWBaseError error;

	// get some settings
	String ipstr = SSETTING("OutGauge IP", "192.168.1.100");
	int port     = ISETTING("OutGauge Port", 1337);
	
	// startup winsock
	WSADATA wsd;
	if ( WSAStartup(MAKEWORD(2, 2), &wsd) != 0 )
	{
		LOG("error starting up winsock");
		return;
	}

	// open a new socket
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
	{
		LOG(String("error creating socket for OutGauge: ").append(strerror(errno)));
		return;
	}

	// get the IP of the remote side, this function is compatible with windows 2000
	hostent *remoteHost = gethostbyname(ipstr.c_str());
	char *ip = inet_ntoa(*(struct in_addr *)*remoteHost->h_addr_list);

	// init socket data
	struct sockaddr_in sendaddr;
	memset(&sendaddr, 0, sizeof(sendaddr));
	sendaddr.sin_family      = AF_INET;
	sendaddr.sin_addr.s_addr = inet_addr(ip);
	sendaddr.sin_port        = htons(port);

	// connect
	if ( connect(sockfd, (struct sockaddr *) &sendaddr, sizeof(sendaddr)) == SOCKET_ERROR )
	{
		LOG(String("error connecting socket for OutGauge: ").append(strerror(errno)));
		return;
	}

	LOG("OutGauge connected successfully");
	working = true;
#else
	// TODO: fix linux
#endif // _WIN32
}

bool OutProtocol::update(float dt)
{
#ifdef _WIN32
	if ( !working )
	{
		return false;
	}

	// below the set delay?
	timer += dt;
	if ( timer < delay )
	{
		return true;
	}
	timer = 0;

	// send a package
	OutGaugePack gd;
	memset(&gd, 0, sizeof(gd));

	// set some common things
	gd.Time  = Root::getSingleton().getTimer()->getMilliseconds();
	gd.ID    = id;
	gd.Flags = 0 | OG_KM;
	sprintf(gd.Car, "RoR");

	Beam *truck = BeamFactory::getSingleton().getCurrentTruck();
	if ( !truck )
	{
		// not in a truck?
		sprintf(gd.Display2, "not in vehicle");
	} else if ( truck && !truck->engine )
	{
		// no engine?
		sprintf(gd.Display2, "no engine");
	} else if ( truck && truck->engine )
	{
		// truck and engine valid
		if ( truck->engine->hasTurbo() )
		{
			gd.Flags |= OG_TURBO;
		}
		gd.Gear        = std::max(0, truck->engine->getGear() + 1); // we only support one reverse gear
		gd.PLID        = 0;
		gd.Speed       = fabs(truck->WheelSpeed);
		gd.RPM         = truck->engine->getRPM();
		gd.Turbo       = truck->engine->getTurboPSI() * 0.0689475729f;
		gd.EngTemp     = 0; // TODO
		gd.Fuel        = 0; // TODO
		gd.OilPressure = 0; // TODO
		gd.OilTemp     = 0; // TODO

		gd.DashLights  = 0;
		gd.DashLights |= DL_HANDBRAKE;
		gd.DashLights |= DL_BATTERY;
		gd.DashLights |= DL_SIGNAL_L;
		gd.DashLights |= DL_SIGNAL_R;
		gd.DashLights |= DL_SIGNAL_ANY;
		if (truck->tc_present)   gd.DashLights |= DL_TC;
		if (truck->alb_present)  gd.DashLights |= DL_ABS;

		gd.ShowLights = 0;
		if (truck->parkingbrake)   gd.ShowLights |= DL_HANDBRAKE;
		if (truck->lights)         gd.ShowLights |= DL_FULLBEAM;
		if (truck->engine->hasContact() && !truck->engine->isRunning()) gd.ShowLights |=  DL_BATTERY;
		if (truck->left_blink_on)  gd.ShowLights |= DL_SIGNAL_L;
		if (truck->right_blink_on) gd.ShowLights |= DL_SIGNAL_R;
		if (truck->warn_blink_on)  gd.ShowLights |= DL_SIGNAL_ANY;
		if (truck->tc_mode)        gd.ShowLights |= DL_TC;
		if (truck->alb_mode)       gd.ShowLights |= DL_ABS;

		gd.Throttle = truck->engine->getAcc();
		gd.Brake    = truck->brake / truck->brakeforce;
		gd.Clutch   = 1 - truck->engine->getClutch(); // 0-1

		strncpy(gd.Display1, truck->realtruckname.c_str(), 15);
		if ( truck->realtruckname.length() > 15 )
		{
			strncpy(gd.Display2, truck->realtruckname.c_str() + 15, 15);
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
