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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 12th of August 2009

#ifndef __StreamableFactory_H_
#define __StreamableFactory_H_

#include "RoRPrerequisites.h"

#include "NetworkStreamManager.h"
#include "StreamableFactoryInterface.h"

#include <pthread.h>

#ifdef USE_SOCKETW
#include "SocketW.h"
#endif //SOCKETW

template<class T, class X> class StreamableFactory : public StreamableFactoryInterface
{
	friend class Network;
public:
	// constructor, destructor and singleton
	StreamableFactory( void ) : locked(false)
	{
		MYASSERT( !_instance );
		_instance = static_cast< T* >( this );
		pthread_mutex_init(&stream_reg_mutex, NULL);

		// add self to factory list
		NetworkStreamManager::getSingleton().addFactory(this);
	}

	~StreamableFactory( void )
	{
		MYASSERT( _instance );
		_instance = 0;
	}

	static T& getSingleton( void )
	{
		MYASSERT( _instance );
		return ( *_instance );
	}

	static T* getSingletonPtr( void )
	{
		return _instance;
	}

	// useful functions
	virtual X *createLocal(int colour) = 0;
	virtual void netUserAttributesChanged(int source, int streamid) = 0;
	virtual X *createRemoteInstance(stream_reg_t *reg) = 0;

	// common functions
	void createRemote(int sourceid, int streamid, stream_register_t *reg, int colour)
	{
		lockStreams();

		stream_reg_t registration;
		registration.sourceid = sourceid;
		registration.streamid = streamid;
		registration.reg      = *reg; // really store the data
		registration.colour   = colour;
		stream_registrations.push_back(registration);

		unlockStreams();
	}

	void deleteRemote(int sourceid, int streamid)
	{
		lockStreams();

		stream_del_t deletion;
		deletion.sourceid = sourceid;
		deletion.streamid = streamid;
		stream_deletions.push_back(deletion);

		unlockStreams();
	}

	virtual bool syncRemoteStreams()
	{
		lockStreams();
		// first registrations
		int changes = 0;
		while (!stream_registrations.empty())
		{
			stream_reg_t reg = stream_registrations.front();
			Streamable *s = createRemoteInstance(&reg);
			if (s)
			{
				// add it to the streams list
				NetworkStreamManager::getSingleton().addRemoteStream(s, reg.sourceid, reg.streamid);
				reg.reg.status = 1;
			} else
			{
				// error creating stream, tell the sourceid that it failed
				reg.reg.status = -1;
			}
			// fixup the registration information
			reg.reg.origin_sourceid = reg.sourceid;
			reg.reg.origin_streamid = reg.streamid;

			// only save registration results for beam streams
			// TODO: maybe enforce general design to allow all stream types to
			// have a feedback channel
			if (reg.reg.type == 0)
			{
				stream_creation_results.push_back(reg);
			}
			// remove registration from list
			stream_registrations.pop_front();
			changes++;
		}

		// count the stream creation results into the changes
		changes += (int)stream_creation_results.size();

		// then deletions:
		// first registrations
		while (!stream_deletions.empty())
		{
			stream_del_t del = stream_deletions.front();
			removeInstance(&del);
			stream_deletions.pop_front();
			changes++;
		}
		unlockStreams();
		return (changes > 0);
	}

	void removeInstance(stream_del_t *del)
	{
		// already locked
		//lockStreams();
		typename std::map < int, std::map < unsigned int, X *> > &streamables = getStreams();
		typename std::map < int, std::map < unsigned int, X *> >::iterator it1;
		typename std::map < unsigned int, X *>::iterator it2;

		for (it1=streamables.begin(); it1!=streamables.end();++it1)
		{
			if (it1->first != del->sourceid) continue;

			for (it2=it1->second.begin(); it2!=it1->second.end();++it2)
			{
				if (del->streamid == -1 || del->streamid == (int)it2->first)
				{
					// deletes the stream
					delete it2->second;
					it2->second = 0;
				}
			}
			break;
		}
		//unlockStreams();
	}

	int checkStreamsOK(int sourceid)
	{
		// walk client and the streams and checks for errors
		lockStreams();
		typename std::map < int, std::map < unsigned int, X *> > &streamables = getStreams();
		typename std::map < int, std::map < unsigned int, X *> >::iterator it1;
		typename std::map < unsigned int, X *>::iterator it2;

		int ok = 0;
		int num = 0;
		for (it1=streamables.begin(); it1!=streamables.end();++it1)
		{
			if (it1->first != sourceid) continue;
			for (it2=it1->second.begin(); it2!=it1->second.end();++it2)
			{
				num++;
				if (it2->second != 0)
				{
					ok = 1;
					break;
				}
			}
			break;
		}
		if (!num)
			ok = 2;
		unlockStreams();
		return ok;
	}

	int checkStreamsResultsChanged()
	{
		lockStreams();
		typename std::map < int, std::map < unsigned int, X *> > &streamables = getStreams();
		typename std::map < int, std::map < unsigned int, X *> >::iterator it1;
		typename std::map < unsigned int, X *>::iterator it2;

		for (it1=streamables.begin(); it1!=streamables.end();++it1)
		{
			for (it2=it1->second.begin(); it2!=it1->second.end();++it2)
			{
				if (!it2->second) continue;
				if (it2->second->getStreamResultsChanged())
				{
					unlockStreams();
					return 1;
				}
			}
		}
		unlockStreams();
		return 0;
	}

	int checkStreamsRemoteOK(int sourceid)
	{
		// walk client and the streams and checks for errors
		lockStreams();
		typename std::map < int, std::map < unsigned int, X *> > &streamables = getStreams();
		typename std::map < int, std::map < unsigned int, X *> >::iterator it1;
		typename std::map < unsigned int, X *>::iterator it2;

		int ok = 0;
		int originstreams = 0;
		for (it1=streamables.begin(); it1!=streamables.end();++it1)
		{
			for (it2=it1->second.begin(); it2!=it1->second.end();++it2)
			{
				if (!it2->second)
					continue;
				if (!it2->second->getIsOrigin())
					continue;
				originstreams++;
				stream_register_t reg;
				reg.status = -2;
				int res = it2->second->getStreamRegisterResultForSource(sourceid, &reg);
				if (!res)
				{
					if (reg.status == 1)
						ok = 1;
				}
				break;
			}
			break;
		}
		if (!originstreams)
			ok = 2;
		unlockStreams();
		return ok;
	}

	int clearStreamRegistrationResults()
	{
		lockStreams();
		stream_creation_results.clear();
		unlockStreams();
		return 0;
	}

	int getStreamRegistrationResults(std::deque < stream_reg_t > *net_results)
	{
		lockStreams();
		// move list entries over to the list in the networking thread
		int res = 0;
		while (!stream_creation_results.empty())
		{
			stream_reg_t reg = stream_creation_results.front();
			net_results->push_back(reg);
			stream_creation_results.pop_front();
			res++;
		}
		unlockStreams();
		return res;
	}

	int addStreamRegistrationResults(int sourceid, stream_register_t *reg)
	{
		lockStreams();
		typename std::map < int, std::map < unsigned int, X *> > &streamables = getStreams();
		typename std::map < int, std::map < unsigned int, X *> >::iterator it1;
		typename std::map < unsigned int, X *>::iterator it2;

		int res = 0;
		for (it1=streamables.begin(); it1!=streamables.end();++it1)
		{
			for (it2=it1->second.begin(); it2!=it1->second.end();++it2)
			{
				if (!it2->second)
					continue;
				if (!it2->second->getIsOrigin())
					continue;
				//int sid = it2->second->getSourceID(); // unused
				int stid = it2->second->getStreamID();
				// only use our locally created streams
				if (stid == reg->origin_streamid)
				{
					it2->second->addStreamRegistrationResult(sourceid, *reg);
					if (reg->status == 1)
						LOG("Client " + TOSTRING(sourceid) + " successfully loaded stream " + TOSTRING(reg->origin_streamid) + " with name '" + reg->name + "', result code: " + TOSTRING(reg->status));
					else
						LOG("Client " + TOSTRING(sourceid) + " could not load stream " + TOSTRING(reg->origin_streamid) + " with name '" + reg->name + "', result code: " + TOSTRING(reg->status));
					res++;
					break;
				}
			}
			break;
		}
		unlockStreams();
		return res;
	}

protected:
	static T* _instance;
	pthread_mutex_t stream_reg_mutex;
	bool locked;

	std::deque < stream_reg_t > stream_registrations;
	std::deque < stream_del_t > stream_deletions;
	std::deque < stream_reg_t > stream_creation_results;

	std::map < int, std::map < unsigned int, X *> > &getStreams()
	{
		// ensure we only access the map when we locked it before
		MYASSERT(locked);
		return mStreamables;
	}

	void lockStreams()
	{
		// double locking is not healthy!
		//MYASSERT(!this->locked);
		MUTEX_LOCK(&stream_reg_mutex);
		this->locked=true;
	}

	void unlockStreams()
	{
		MUTEX_UNLOCK(&stream_reg_mutex);
		this->locked=false;
	}

private:
	// no direct access to it, helps with locking it before using it
	std::map < int, std::map < unsigned int, X *> > mStreamables;

};

#endif // __StreamableFactory_H_
