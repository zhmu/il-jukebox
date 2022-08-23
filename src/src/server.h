/*
 * server.h
 *
 * This is the jukebox server.
 *
 */
#include <stdlib.h>
#include <libplusplus/network.h>
#include "client.h"

#ifndef __JUKESERVER_H__
#define __JUKESERVER_H__

/*!
 * \class JUKESERVER
 * \brief This is the jukebox network server.
 */
class JUKESERVER : public NETSERVER {
	friend class JUKECLIENT;

public:
	//! \brief This will handle incoming connections.
	void	incoming();

	/*! \brief This will broadcast a message to all clients who desire them
	 *  \param msg The message to send
	 */
	void	sendUpdate (char* msg);
};

#endif // __JUKESERVER_H__
