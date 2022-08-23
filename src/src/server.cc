/*
 * server.cc - Jukebox server code
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libplusplus/network.h>
#include "client.h"
#include "server.h"

/*
 * JUKESERVER::incoming()
 *
 * This will be called on each incoming connection.
 *
 */
void
JUKESERVER::incoming() {
	JUKECLIENT* c = new JUKECLIENT();

	// auto-accept the connection
	if (!accept (c))
		// this failed. bail out
		return;

	// be polite and greet the client
	c->welcome();
}

/*
 * JUKESERVER::sendUpdate (char* msg)
 *
 * This will send update [msg] to every client who cares.
 *
 */
void
JUKESERVER::sendUpdate (char* msg) {
	// scan all clients, too
	for (int i = 0; i < getClients()->count(); i++) {
		// fetch the client
		JUKECLIENT* c = (JUKECLIENT*)getClients()->elementAt (i);

		// skip clients who don't care about updates
		if (!c->wantsUpdates()) continue;

		// go!
		c->sendf (msg);
	}
}

/* vim:set ts=2 sw=2: */
