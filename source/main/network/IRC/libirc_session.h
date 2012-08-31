/*
 * Copyright (C) 2004-2009 Georgy Yunaev gyunaev@ulduzsoft.com
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 */


#ifndef INCLUDE_IRC_SESSION_H
#define INCLUDE_IRC_SESSION_H


#ifndef IN_INCLUDE_LIBIRC_H
	#error This file should not be included directly, include just libircclient.h
#endif


#include "libirc_params.h"
#include "libirc_dcc.h"
#include "libirc_events.h"


struct irc_session_s
{
	void		*	ctx;
	int				dcc_timeout;

	int				options;
	int				lasterror;

	char 			incoming_buf[LIBIRC_BUFFER_SIZE];
	unsigned int	incoming_offset;

	char 			outgoing_buf[LIBIRC_BUFFER_SIZE];
	unsigned int	outgoing_offset;
	port_mutex_t	mutex_session;

	socket_t		sock;
	int				state;
	int				motd_received;

	char 		  *	server;
	char		  * server_password;
	char 		  *	realname;
	char		  * username;
	char		  *	nick;

#if defined( ENABLE_IPV6 )
	struct in6_addr	local_addr;
#else
	struct in_addr	local_addr;
#endif
	irc_dcc_t		dcc_last_id;
	irc_dcc_session_t * dcc_sessions;
	port_mutex_t	mutex_dcc;

	irc_callbacks_t	callbacks;
};


#endif /* INCLUDE_IRC_SESSION_H */
