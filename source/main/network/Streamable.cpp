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
// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 7th of August 2009

#include "Streamable.h"

#include "Language.h"
#include "Network.h"
#include "NetworkStreamManager.h"

using namespace Ogre;

Streamable::Streamable() : isOrigin(false)
{
	//NetworkStreamManager::getSingleton().addStream(this);
}

Streamable::~Streamable()
{
}

void Streamable::addPacket(int type, unsigned int len, char* content)
{
#ifdef USE_SOCKETW
	if (len > maxPacketLen)
		// packet too big, discarded
		return;

	if (packets.size() > packetBufferSizeDiscardData && type == MSG2_STREAM_DATA)
		// discard unimportant data packets for some while
		return;

	if (packets.size() > packetBufferSize)
		// buffer full, packet discarded
		return;

	int uid = gEnv->network->getUID();
	unsigned int streamid = this->streamid; //we stored the streamid upon stream registration in this class

	bufferedPacket_t packet;
	memset(&packet, 0, sizeof(bufferedPacket_t));

	// allocate buffer
	//packet.packetBuffer = (char*)calloc(len, sizeof(char));

	char *buffer = (char*)(packet.packetBuffer);

	// write header in buffer
	header_t *head = (header_t *)buffer;
	head->command  = type;
	head->source   = uid;
	head->size     = len;
	head->streamid = streamid;

	// then copy the contents
	char *bufferContent = (char *)(buffer + sizeof(header_t));
	memcpy(bufferContent, content, len);

	// record the packet size
	packet.size = len + sizeof(header_t);

	/*
	String header_hex = hexdump(buffer, sizeof(header_t));
	String content_hex = hexdump((buffer + sizeof(header_t)), len);

	LOG("header:  " + header_hex);
	LOG("content: " + content_hex);
	*/

	/*
	char hash[256] = "";
	RoR::CSHA1 sha1;
	sha1.UpdateHash((uint8_t *)bufferContent, len);
	sha1.Final();
	sha1.ReportHash(hash, RoR::CSHA1::REPORT_HEX_SHORT);
	LOG("S|HASH: " + String(hash));
	*/

	{
		std::lock_guard<std::mutex> lock(m_send_work_mutex);

		packets.push_back(packet);
	}

	// trigger buffer clearing
	NetworkStreamManager::getSingleton().triggerSend();
#endif //SOCKETW
}

void Streamable::addReceivedPacket(header_t header, char *buffer)
{
	if (receivedPackets.size() > packetBufferSize)
		// buffer full, packet discarded
		return;

	// construct the data holding struct
	recvPacket_t packet;
	memset(&packet, 0, sizeof(packet)); // we need to do this, since we use String(buffer) at some point which will crash otherwise
	packet.header = header;
	memcpy(packet.buffer, buffer, header.size);

	std::lock_guard<std::mutex> lock(m_recv_work_mutex);

	receivedPackets.push_back(packet);
}

void Streamable::sendStream(Network *net)
{
	std::vector<bufferedPacket_t> work_packets;
	{
		std::lock_guard<std::mutex> lock(m_send_work_mutex);
		work_packets = packets;
		packets.clear();
	}

	for (bufferedPacket_t packet : work_packets)
	{
		int etype = net->sendMessageRaw(packet.packetBuffer, packet.size);
		if (etype)
		{
			wchar_t emsg[256];
			UTFString tmp = _L("Error %i while sending data packet");
			swprintf(emsg, 256, tmp.asWStr_c_str(), etype);
			net->netFatalError(UTFString(emsg));
			return;
		}
	}
}

void Streamable::receiveStream()
{
	std::vector<recvPacket_t> work_packets;
	{
		// Deadlock ...
		//std::lock_guard<std::mutex> lock(m_recv_work_mutex);
		work_packets = receivedPackets;
		receivedPackets.clear();
	}

	for (recvPacket_t packet : work_packets)
	{
		//Network::debugPacket("receive-2", &packet.header, (char *)packet.buffer);
		receiveStreamData(packet.header.command, packet.header.source, packet.header.streamid, (char*)packet.buffer, packet.header.size);
	}
}

void Streamable::addStreamRegistrationResult(int sourceid, stream_register_t reg)
{
	std::lock_guard<std::mutex> lock(m_results_mutex);

	mStreamableResults[sourceid] = reg;
}

int Streamable::getStreamRegisterResultForSource(int sourceid, stream_register_t *reg)
{
	std::lock_guard<std::mutex> lock(m_results_mutex);

	// Fixes program freeze <- Don't ask why
	if (mStreamableResults.size() > 1000)
		mStreamableResults.clear();

	if (mStreamableResults.find(sourceid) == mStreamableResults.end())
		return 1;

	*reg = mStreamableResults[sourceid];

	return 0;
}
