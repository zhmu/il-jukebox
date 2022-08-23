/*
 * client.cc - Jukebox daemon client code
 *
 */
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libplusplus/network.h>
#include "artist.h"
#include "album.h"
#include "jukebox.h"
#include "client.h"
#include "server.h"
#include "track.h"
#include "ident.h"
#include "user.h"

/*
 * JUKECLIENT::getState()
 *
 * This will fetch the user status.
 *
 */
int
JUKECLIENT::getState() {
	return state;
}

/*
 * JUKECLIENT::getUser()
 *
 * This will fetch the user information. If no user is authenticated, NULL will
 * be returned.
 *
 */
USER*
JUKECLIENT::getUser() {
	return (userid == -1) ? NULL : &user;
}

/*
 * JUKECLIENT::welcome()
 *
 * This will initialize and greet a new client.
 
 */
void
JUKECLIENT::welcome() {
	// initialize the state
	state = JUKECLIENT_STATE_CONN; userid = -1; sendUpdates = 0;

	// send the welcome
	sendf (JUKECLIENT_MSG_WELCOME);
}

/*
 * JUKECLIENT::checkPriv (char* cmd)
 *
 * This will check the privileges for command [cmd]. It will print a message and
 * return 0 on failure, or 1 on success.
 *
 */
int
JUKECLIENT::checkPriv (char* cmd) {
	// got enough rights?
	if (!config->checkRight (cmd, user.status)) {
		// no. too bad
		sendf (JUKECLIENT_MSG_NOPRIVS);
		return 0;
	}

	// it's okay
	return 1;
}

/*
 * JUKECLIENT::cmdUser (char* arg)
 *
 * This will set the username to [arg].
 *
 */
void
JUKECLIENT::cmdUser (char* arg) {
	USERS* userDB = users;

	// try all backends
	while (userDB) {
		// try to fetch the user
		if (userDB->fetchUserByName (arg, &user)) {
			// victory
			userid = user.id;
			sendf (JUKECLIENT_MSG_USEROK);
			return;
		}

		// next
		userDB = userDB->getNextDB();
	}

	// this failed. complain
	sendf (JUKECLIENT_MSG_NOUSER);
}

/*
 * JUKECLIENT::cmdPassword (char* arg)
 *
 * This will try to authenticate the user.
 *
 */
void
JUKECLIENT::cmdPassword (char* arg) {
	USERS* userDB = users;

	// got an userid?
	if (userid == -1) {
		// no. complain
		sendf (JUKECLIENT_MSG_USERFIRST);
		return;
	}

	// try all backends
	while (userDB) {
		// password match?
		if (userDB->verifyPassword ((const char*)arg, &user)) {
			// yes. victory!
			state = JUKECLIENT_STATE_AUTH;
			sendf (JUKECLIENT_MSG_PASSOK, user.username);
			return;
		}

		// next
		userDB = userDB->getNextDB();
	}

	// no suitable backends found. complain and log
	sendf (JUKECLIENT_MSG_BADPASS);
	logger->log (LOG_INFO, "User %s supplied a bad password", user.username);
}

/*
 * JUKECLIENT::cmdDisconnect()
 *
 * This will disconnect the client.
 *
 */
void
JUKECLIENT::cmdDisconnect() {
	// bye!
	sendf (JUKECLIENT_MSG_BYE);
	close();
	delete this;
}

/*
 * JUKECLIENT::cmdPause ()
 *
 * This will pause playback.
 *
 */
void
JUKECLIENT::cmdPause() {
	char tmp[256 /* XXX */];

	// privilege check
	JUKECLIENT_HANDLE_PRIV (JUKECLIENT_CMD_PAUSE)

	// just do it and tell the client
	player->pause();
	sendf (JUKECLIENT_MSG_PAUSED);

	// inform all clients who care
	snprintf (tmp, sizeof (tmp), JUKECLIENT_UPDATE_SONG, 'p', "", "");
	server->sendUpdate (tmp);

	// logging
	logger->log (LOG_INFO, "Playback paused by %s", user.username);
}

/*
 * JUKECLIENT::cmdResume ()
 *
 * This will resume playback.
 *
 */
void
JUKECLIENT::cmdResume() {
	char tmp[256 /* XXX */ ];

	// privilege check
	JUKECLIENT_HANDLE_PRIV (JUKECLIENT_CMD_CONTINUE)

	// just do it and tell the client
	player->resume();
	sendf (JUKECLIENT_MSG_RESUMED);

	// inform all clients who care
	snprintf (tmp, sizeof (tmp), JUKECLIENT_UPDATE_SONG, 'P', "", "");
	server->sendUpdate (tmp);

	// logging
	logger->log (LOG_INFO, "Playback resumed by %s", user.username);
}

/*
 * JUKECLIENT::cmdStop()
 *
 * This will stop playback.
 *
 */
void
JUKECLIENT::cmdStop() {
	char tmp[256 /* XXX */];

	// privilege check
	JUKECLIENT_HANDLE_PRIV (JUKECLIENT_CMD_STOP)

	// just do it and tell the client
	player->stop();
	sendf (JUKECLIENT_MSG_STOPPED);

	// inform all clients who care
	snprintf (tmp, sizeof (tmp), JUKECLIENT_UPDATE_SONG, 'I', "", "");
	server->sendUpdate (tmp);

	// logging
	logger->log (LOG_INFO, "Playback stopped by %s", user.username);
}

/*
 * JUKECLIENT::cmdPlay()
 *
 * This will start playback.
 *
 */
void
JUKECLIENT::cmdPlay() {
	// privilege check
	JUKECLIENT_HANDLE_PRIV (JUKECLIENT_CMD_PLAY)

	// are we currently paused?
	if (player->getStatus() == PLAYER_STATUS_PAUSED) {
		// yes. unpause and tell the client
		player->resume();
		sendf (JUKECLIENT_MSG_RESUMED);

		// logging
		logger->log (LOG_INFO, "Playback resumed by %s", user.username);
	} else if (player->getStatus() == PLAYER_STATUS_PLAYING) {
		// no, we are playing. tell the client he messed up :-(
		sendf (JUKECLIENT_MSG_ALREADYPLAYING);
	} else {
		// no, we are idling. start playing and tell the client
		player->play();
		sendf (JUKECLIENT_MSG_STARTED);

		// logging
		logger->log (LOG_INFO, "Playback started by %s", user.username);
	}
}

/*
 * JUKECLIENT::cmdSkip()
 *
 * This will skip the current track.
 *
 */
void
JUKECLIENT::cmdSkip() {
	// privilege check
	JUKECLIENT_HANDLE_PRIV (JUKECLIENT_CMD_NEXT)

	// are we playing?
	if (player->getStatus() != PLAYER_STATUS_PLAYING) {
		// no. complain
		sendf (JUKECLIENT_MSG_NOTPLAYING);
		return;
	}

	// do it and tell the client
	player->next();
	sendf (JUKECLIENT_MSG_SKIPPED, user.username);

	// logging
	logger->log (LOG_INFO, "Track skipped by %s", user.username);
}

/*
 * JUKECLIENT::cmdUsers()
 *
 * This will display the current users attached.
 *
 */
void
JUKECLIENT::cmdUsers() {
	VECTOR* clients = server->getClients();

	// handle all users
	for (int i = 0; i < clients->count(); i++) {
		// fetch the user
		JUKECLIENT* client = (JUKECLIENT*)clients->elementAt (i);

		// is the user authenticated?
		if (client->getState() == JUKECLIENT_STATE_AUTH) {
			// yes. display the information
			sendf (JUKECLIENT_MSG_USERLIST, client->getUser()->username);
		}
	}
	sendf (JUKECLIENT_MSG_USERSLISTED);
}

/*
 * JUKECLIENT::cmdStatus()
 *
 * This will display the current player status.
 *
 */
void
JUKECLIENT::cmdStatus() {
	int status = player->getStatus();
	int trackid = player->getTrackID();
	TRACK* t;
	char title[TRACK_MAX_TITLE_LEN];
	char artist[ARTIST_MAX_LEN];

	// what we return depends on the player status ...
	switch (status) {
		case PLAYER_STATUS_IDLE: // idle
		                         sendf (JUKECLIENT_MSG_STATUS, 'I', (queue->getRandom()) ? 'Y' : 'N', (player->isLocked()) ? 'Y' : 'N', "", "");
		                         break;
 case PLAYER_STATUS_PLAYING: // playing
  case PLAYER_STATUS_PAUSED: // paused
		                         // fetch the song title
														 try  {
														    t = new TRACK (trackid);
														    strncpy (title, t->getTitle(), TRACK_MAX_TITLE_LEN);
														  } catch (TrackException e) {
															  // no such track
															  strcpy (title, "?"); t = NULL;
														  }
		                          // fetch the song artist
															if (t != NULL) {
																try {
																	ARTIST* a = new ARTIST (t->getArtistID());
																	strncpy (artist, a->getName(), ARTIST_MAX_LEN);
																	delete a;
																} catch (ArtistException e) {
																	// no such artist
															  	strcpy (artist, "?");
																}
															}
		                          sendf (JUKECLIENT_MSG_STATUS, (status == PLAYER_STATUS_PLAYING) ? 'P' : 'p', (queue->getRandom()) ? 'Y' : 'N', (player->isLocked()) ? 'Y' : 'N', title, artist);
															if (t) delete t;
		                          break;
	}
}

/*
 * JUKECLIENT::cmdRandom (char* arg)
 *
 * This will handle the setting of random playing mode.
 *
 */
void
JUKECLIENT::cmdRandom (char* arg) {
	// privilege check
	JUKECLIENT_HANDLE_PRIV (JUKECLIENT_CMD_RANDOM)

	// is the argument YES or NO?
	if ((strcasecmp (arg, "YES")) && (strcasecmp (arg, "NO"))) {
		// no. complain
		sendf (JUKECLIENT_MSG_YESORNO);
		return;
	}

	// yes. get the value
	int on = (!strcasecmp (arg, "YES")) ? 1 : 0;

	// set the random play
	queue->setRandom (on);

	// tell the user what we did and log it
	if (on) {
		sendf (JUKECLIENT_MSG_RANDOMON);
		logger->log (LOG_INFO, "Random play enabled by %s", user.username);
	} else {
		sendf (JUKECLIENT_MSG_RANDOMOFF);
		logger->log (LOG_INFO, "Random play disabled by %s", user.username);
	}
}

/*
 * JUKECLIENT::cmdQueue()
 *
 * This will handle the displaying of the queue.
 *
 */
void
JUKECLIENT::cmdQueue() {
	char title[TRACK_MAX_TITLE_LEN];;
	char artist[QUEUE_MAX_ARTIST_LEN];
	int playid, trackid;
	int n = 0;
	TRACK* t;

	// handle queue items until there are no more
	while (1) {
		// fetch the queue item
		if (!queue->getQueueTrackID (n, &playid, &trackid))
			// this failed. we're done
			break;

		// fetch the song artist
		try  {
			t = new TRACK (trackid);
			strncpy (title, t->getTitle(), TRACK_MAX_TITLE_LEN);
		} catch (TrackException e) {
			// no such track
			strcpy (title, "?"); t = NULL;
		}

		// fetch the song artist
		if (t) {
			try {
				ARTIST* a = new ARTIST (t->getArtistID());
				strncpy (artist, a->getName(), ARTIST_MAX_LEN);
				delete a;
			} catch (ArtistException e) {
				// no such artist
				strcpy (artist, "?");
			}
		}
		sendf (JUKECLIENT_MSG_QUEUEITEM, playid, title, artist);

		// next
		n++;
	}

	// all done
	sendf (JUKECLIENT_MSG_QUEUEDONE);
}

/*
 * JUKECLIENT::cmdRemove (char* arg)
 *
 * This will remove a track from the queue.
 *
 */
void
JUKECLIENT::cmdRemove (char* arg) {
	long l;
	char* ptr;

	// privilege check
	JUKECLIENT_HANDLE_PRIV (JUKECLIENT_CMD_REMOVE)

	// try to resolve the number
	l = strtol (arg, &ptr, 10);
	if ((!strlen (arg)) || (*ptr)) {
		// this failed. complain
		sendf (JUKECLIENT_MSG_REMOVESYN);
		return;
	}

	// need to log this ?
	if (config->getLogRemove()) {
		// yes. fetch the track id
		int trackid;
		if (queue->getQueueItem (l, &trackid)) {
			// got it. fetch the track
			try {
				TRACK* t = new TRACK (trackid);

				// grab the artist
				try {
					ARTIST* a = new ARTIST (t->getArtistID());

					// log the request
					logger->log (LOG_INFO, "Track %s - %s removed by %s", a->getName(), t->getTitle(), user.username);

					// ditch the artist
					delete a;
				} catch (ArtistException e) {
					// ...
				}

				// remove the track
				delete t;
			} catch (TrackException e) {
				// ...
			}
		}
	} else {
		// log it simple
		logger->log (LOG_INFO, "Queue item removed by %s", user.username);
	}

	// is this the current track?
	if (player->getQueueItemID() == l) {
		// yes. switch to the next track
		player->next();
	} else {
		// no. just zap the item
		queue->remove (l);
	}

	// victory!
	sendf (JUKECLIENT_MSG_REMOVEOK);
}

/*
 * JUKECLIENT::cmdLock()
 *
 * This will lock the player.
 *
 */
void
JUKECLIENT::cmdLock() {
	// privilege check
	JUKECLIENT_HANDLE_PRIV (JUKECLIENT_CMD_LOCK)

	// lock it
	player->lock();

	// all done
	sendf (JUKECLIENT_MSG_LOCKED);

	// logging
	logger->log (LOG_INFO, "Jukebox locked by %s", user.username);
}

/*
 * JUKECLIENT::cmdUnlock()
 *
 * This will unlock the player.
 *
 */
void
JUKECLIENT::cmdUnlock() {
	// privilege check
	JUKECLIENT_HANDLE_PRIV (JUKECLIENT_CMD_UNLOCK)

	// unlock it
	player->unlock();

	// all done
	sendf (JUKECLIENT_MSG_UNLOCKED);

	// logging
	logger->log (LOG_INFO, "Jukebox unlocked by %s", user.username);
}

/*
 * JUKECLIENT::cmdClear()
 *
 * This will clear the Jukebox queue.
 *
 */
void
JUKECLIENT::cmdClear() {
	// privilege check
	JUKECLIENT_HANDLE_PRIV (JUKECLIENT_CMD_CLEAR)

	// clear the queue
	queue->clear();

	// all done
	sendf (JUKECLIENT_MSG_CLEARED);

	// logging
	logger->log (LOG_INFO, "Jukebox cleared by %s", user.username);
}

/*
 * JUKECLIENT::cmdHelp()
 *
 * This displays available commands
 *
 */
void
JUKECLIENT::cmdHelp() {
	// just display some text, nothing more
	sendf ("[I] JukeServer 0.1 help\n" \
"Commands:\n" \
"help                    Display this text\n" \
"user <username>         authenticate with <username>\n" \
"pass(word) <password>   authenticate with <password>\n" \
"play                    start playback\n" \
"pause                   pause playback\n" \
"cont(inue)              continue playback\n" \
"skip                    skip playing track\n" \
"stop                    stop playback\n" \
"rand(om) <yes|no>       enable or disable random queue\n" \
"q(ueue)                 display queue\n" \
"rem(ove)                remove first track in queue\n" \
"lock                    lock queue for adding tracks\n" \
"unlock                  unlock queue for adding tracks\n" \
"users                   display currently connected users\n" 
"stat(us)                display status of jukebox\n" \
"clear                   clear the jukebox queue\n" \
"albums                  display all albums\n" \
"artistalbums <id>       display all albums by an artist\n" \
"artists                 display all artists\n" \
"getalbum <id>           fetch information on an album\n" \
"getartist <id>          fetch information on an artist n" \
"gettrack <id>           fetch information on a track\n" \
"listalbum <id>          displays all tracks in an album\n" \
"enqeueutrack <id>		   enqueue a track\n" \
"enqeueualbum <id>		   enqueue a complete album\n" \
"bye                     see you later..\n" \
"exit                    close client\n" \
"disc(onnect)            close connection with server\n" \
"updates <yes|no>        receive updates on player changes\n" \
"\n");
}

/*
 * JUKECLIENT::cmdAlbums()
 *
 * This will display all albums known to the jukebox.
 *
 */
void
JUKECLIENT::cmdAlbums() {
	ALBUM* album = new ALBUM();

	// handle all albums
	while (album->fetchNext ()) {
		// send the data
		sendf (JUKECLIENT_MSG_ALBUM, album->getID(), album->getArtistID(), album->getName());
	}

	// terminate the list
	sendf (JUKECLIENT_MSG_ALBUMEND);

	// bye bye
	delete album;
}

/*
 * JUKECLIENT::cmdArtists()
 *
 * This will display all artists known to the jukebox.
 *
 */
void
JUKECLIENT::cmdArtists() {
	ARTIST* artist = new ARTIST();

	// handle all artists
	while (artist->fetchNext()) {
		// send the data
		sendf (JUKECLIENT_MSG_ARTIST, artist->getID(), artist->getName());
	}

	// terminate the list
	sendf (JUKECLIENT_MSG_ARTISTEND);

	// bye bye
	delete artist;
}

/*
 * JUKECLIENT::cmdEnqueueTrack (char* arg)
 *
 * This will handle the ENQUEUETRACK command.
 *
 */
void
JUKECLIENT::cmdEnqueueTrack(char* arg) {
	long l;
	char* ptr;

	// privilege check
	JUKECLIENT_HANDLE_PRIV (JUKECLIENT_CMD_ENQUEUETR)

	// try to resolve the number
	l = strtol (arg, &ptr, 10);
	if ((!strlen (arg)) || (*ptr)) {
		// this failed. complain
		sendf (JUKECLIENT_MSG_ENQSYN);
		return;
	}

	// enqueue it
	if (queue->enqueueTrack (&user, l)) {
		// all done
		sendf (JUKECLIENT_MSG_ENQOK);

		// need to log?
		if (config->getLogEnqueue()) {
			// yes. log this
			try {
				// fetch the track
				TRACK* t = new TRACK (l);

				try {
					// fetch the artist
					ARTIST* a = new ARTIST (t->getArtistID());

					// finally, log
					logger->log (LOG_INFO, "Track %s - %s enqueued by %s", a->getName(), t->getTitle(), user.username);

					// ditch the artist
					delete a;
				} catch (ArtistException e) {
					// ... ??? ...
				}

				// farewell to the track
				delete t;
			} catch (TrackException e) {
				// ... ??? ...
			}
		}
	} else
		// bummer
		sendf (JUKECLIENT_MSG_NOTRACK);
}

/*
 * JUKECLIENT::cmdEnqueueAlbum (char* arg)
 *
 * This will handle the ENQUEUEALBUM command.
 *
 */
void
JUKECLIENT::cmdEnqueueAlbum(char* arg) {
	long l;
	char* ptr;

	// privilege check
	JUKECLIENT_HANDLE_PRIV (JUKECLIENT_CMD_ENQUEUEAL)

	// try to resolve the number
	l = strtol (arg, &ptr, 10);
	if ((!strlen (arg)) || (*ptr)) {
		// this failed. complain
		sendf (JUKECLIENT_MSG_ENQSYN);
		return;
	}

	// enqueue it
	if (queue->enqueueAlbum (&user, l)) {
		// all done
		sendf (JUKECLIENT_MSG_ENQOK);

		// need to log?
		if (config->getLogEnqueue()) {
			// yes. log this
			try {
				// fetch the album
				ALBUM* a = new ALBUM (l);

				try {
					// fetch the artist
					ARTIST* ar = new ARTIST (a->getArtistID());

					// finally, log
					logger->log (LOG_INFO, "Album %s - %s enqueued by %s", ar->getName(), a->getName(), user.username);

					// ditch the artist
					delete ar;
				} catch (ArtistException e) {
					// ... ??? ...
				}

				// farewell to the album
				delete a;
			} catch (AlbumException e) {
				// ... ??? ...
			}
		}
	} else
		// nummer
		sendf (JUKECLIENT_MSG_NOALBUM);
}

/*
 * JUKECLIENT::cmdListAlbum (char* arg)
 *
 * This will handle the LISTALBUM command.
 *
 */
void
JUKECLIENT::cmdListAlbum(char* arg) {
	long l;
	char* ptr;
	int pos = 0;
	int trackid;

	// try to resolve the number
	l = strtol (arg, &ptr, 10);
	if ((!strlen (arg)) || (*ptr)) {
		// this failed. complain
		sendf (JUKECLIENT_MSG_LISTSYN);
		return;
	}

	// wade through the album
	try {
		// try to fetch the album
		ALBUM* album = new ALBUM (l);

		// keep fetching tracks
		while (album->fetchTrack (pos, &trackid)) {
			// this worked. fetch the track itself
			try {
				TRACK* track = new TRACK (trackid);

				// send it over
				sendf (JUKECLIENT_MSG_TRACK, trackid, track->getTitle());

				// remove the track
				delete track;
			} catch (TrackException e) {
				// ... silently ignore it ...
			}

			// next
			pos++;
		}

		// enough is enough
		delete album;
	} catch (AlbumException e) {
		// bummer
		sendf (JUKECLIENT_MSG_NOALBUM);
		return;
	}

	// all done
	sendf (JUKECLIENT_MSG_LISTOK);
}

/*
 * JUKECLIENT::cmdGetArtist (char* cmd)
 *
 * This will handle the GETARTIST command.
 *
 */
void
JUKECLIENT::cmdGetArtist (char* arg) {
	long l;
	char* ptr;

	// try to resolve the number
	l = strtol (arg, &ptr, 10);
	if ((!strlen (arg)) || (*ptr)) {
		// this failed. complain
		sendf (JUKECLIENT_MSG_GETASYN);
		return;
	}

	try {
		// fetch the artist
		ARTIST* artist = new ARTIST (l);

		// send the data over
		sendf (JUKECLIENT_MSG_ARTIST, artist->getID(), artist->getName());

		// delete the artist
		delete artist;
	} catch (ArtistException e) {
		// too bad
		sendf (JUKECLIENT_MSG_NOARTIST);
	}
}

/*
 * JUKECLIENT::cmdGetAlbum (char* cmd)
 *
 * This will handle the GETALBUM command.
 *
 */
void
JUKECLIENT::cmdGetAlbum (char* arg) {
	long l;
	char* ptr;

	// try to resolve the number
	l = strtol (arg, &ptr, 10);
	if ((!strlen (arg)) || (*ptr)) {
		// this failed. complain
		sendf (JUKECLIENT_MSG_GETASYN);
		return;
	}

	try {
		// fetch the album
		ALBUM* album = new ALBUM (l);

		// send the data over
		sendf (JUKECLIENT_MSG_ALBUM, album->getID(), album->getArtistID(), album->getName());

		// delete the album
		delete album;
	} catch (AlbumException e) {
		// too bad
		sendf (JUKECLIENT_MSG_NOALBUM);
	}
}

/*
 * JUKECLIENT::cmdGetTrack (char* cmd)
 *
 * This will handle the GETTRACK command.
 *
 */
void
JUKECLIENT::cmdGetTrack (char* arg) {
	long l;
	char* ptr;

	// try to resolve the number
	l = strtol (arg, &ptr, 10);
	if ((!strlen (arg)) || (*ptr)) {
		// this failed. complain
		sendf (JUKECLIENT_MSG_GETASYN);
		return;
	}

	try {
		// fetch the track
		TRACK* track = new TRACK (l);

		// send the data over
		sendf (JUKECLIENT_MSG_TRACK, l, track->getTitle());

		// delete the track
		delete track;
	} catch (TrackException e) {
		// too bad
		sendf (JUKECLIENT_MSG_NOTRACK);
	}
}

/*
 * JUKECLIENT::cmdArtisAlbums(char* arg)
 *
 * This will display all albums known to the jukebox.
 *
 */
void
JUKECLIENT::cmdArtistAlbums(char* arg) {
	long l;
	char* ptr;

	// try to resolve the number
	l = strtol (arg, &ptr, 10);
	if ((!strlen (arg)) || (*ptr)) {
		// this failed. complain
		sendf (JUKECLIENT_MSG_GETASYN);
		return;
	}

	// create a blank album
	ALBUM* album = new ALBUM();

	// handle all albums
	while (album->fetchArtistNext (l)) {
		// send the data
		sendf (JUKECLIENT_MSG_ALBUM, album->getID(), album->getArtistID(), album->getName());
	}

	// terminate the list
	sendf (JUKECLIENT_MSG_ALBUMEND);

	// bye bye
	delete album;
}

/*
 * JUKECLIENT::cmdVolume (char* arg)
 *
 * This will handle the VOLUME command.
 *
 */
void
JUKECLIENT::cmdVolume (char* arg) {
	long l;
	char* ptr;

	// privilege check
	JUKECLIENT_HANDLE_PRIV (JUKECLIENT_CMD_VOLUME)

	// do we even have a volume mananger?
	if (!volume->isAvailable())	{
		// no. complain
		sendf (JUKECLIENT_MSG_NOVOL);
		return;
	}

	// try to resolve the number
	l = strtol (arg, &ptr, 10);
	if ((!strlen (arg)) || (*ptr)) {
		// this failed. send the current volume over
		sendf (JUKECLIENT_MSG_VOLUME, volume->getVolume());
		return;
	}

	// update the volume
	if (l > 100) l = 100;
	volume->setVolume (l);

	// all done
	sendf (JUKECLIENT_MSG_VOLOK);
}

/*
 * JUKECLIENT::cmdVolumeUp ()
 *
 * This will handle the VOLUP command.
 *
 */
void
JUKECLIENT::cmdVolumeUp() {
	int i;

	// privilege check
	JUKECLIENT_HANDLE_PRIV (JUKECLIENT_CMD_VOLUME)

	// do we even have a volume mananger?
	if (!volume->isAvailable())	{
		// no. complain
		sendf (JUKECLIENT_MSG_NOVOL);
		return;
	}

	// update the volume
	i = volume->getVolume();
	if ((i + VOLUME_STEPSIZE) > 100)
		i = 100;
	else
		i += VOLUME_STEPSIZE;
	volume->setVolume (i);

	// all done
	sendf (JUKECLIENT_MSG_VOLOK);
}

/*
 * JUKECLIENT::cmdVolumeDown ()
 *
 * This will handle the VOLDN command.
 *
 */
void
JUKECLIENT::cmdVolumeDown() {
	int i;

	// privilege check
	JUKECLIENT_HANDLE_PRIV (JUKECLIENT_CMD_VOLUME)

	// do we even have a volume mananger?
	if (!volume->isAvailable())	{
		// no. complain
		sendf (JUKECLIENT_MSG_NOVOL);
		return;
	}

	// update the volume
	i = volume->getVolume();
	if ((i - VOLUME_STEPSIZE) < 0)
		i = 0;
	else
		i -= VOLUME_STEPSIZE;
	volume->setVolume (i);

	// all done
	sendf (JUKECLIENT_MSG_VOLOK);
}

/*
 * JUKECLIENT::cmdIdent()
 *
 * This will handle the IDENT command.
 *
 */
void
JUKECLIENT::cmdIdent() {
	IDENT* ident;

	// got an userid?
	if (userid == -1) {
		// no. complain
		sendf (JUKECLIENT_MSG_USERFIRST);
		return;
	}

	// is IDENT allowed?
	if (!config->isIdentAllowed()) {
		// no. complain
		sendf (JUKECLIENT_MSG_NOIDENT);
		return;
	}

	// is IDENT allowed from this host?
	if (!config->checkIdentHost (getClientAddress())) {
		// no. complain
		sendf (JUKECLIENT_MSG_NOIDENTHOST);
		return;
	}

	// try to identify ourselves
	ident = new IDENT();
	if (!ident->identify (getClientAddress(), user.username)) {
		// this failed. complain	
		delete ident;
		sendf (JUKECLIENT_MSG_IDENTFAIL);
		return;
	}

	// victory
	delete ident;
	state = JUKECLIENT_STATE_AUTH;
	sendf (JUKECLIENT_MSG_PASSOK, user.username);
}

/*
 * JUKECLIENT::cmdUpdates (char* arg)
 *
 * This will handle the setting of updates.
 *
 */
void
JUKECLIENT::cmdUpdates (char* arg) {
	// privilege check
	JUKECLIENT_HANDLE_PRIV (JUKECLIENT_CMD_UPDATES)

	// is the argument YES or NO?
	if ((strcasecmp (arg, "YES")) && (strcasecmp (arg, "NO"))) {
		// no. complain
		sendf (JUKECLIENT_MSG_YESORNO);
		return;
	}

	// yes. update the values
	sendUpdates = (!strcasecmp (arg, "YES")) ? 1 : 0;

	// tell the user what we did and log it
	if (sendUpdates)
		sendf (JUKECLIENT_MSG_UPDATESON);
	else
		sendf (JUKECLIENT_MSG_UPDATESOFF);
}

/*
 * JUKECLIENT::incoming()
 *
 * This will be called on incoming data.
 *
 */
void
JUKECLIENT::incoming() {
	char data[JUKECLIENT_MAX_DATA_LENGTH];
	int	 len = recv (data, sizeof (data) - 1);
	char* cmd = data;
	char* arg = "";
	char* ptr;

	// if we got no data, bail out
	if (!len)
		return;

	// terminate the data buffer
	data[len] = 0;

	// get rid of any trailing newlines
	len--;
	while ((data[len] == '\n') || (data[len] == '\r'))
		data[len--] = 0;

	// scan for a space
	ptr = strchr (cmd, ' ');
	if (ptr != NULL) {
		// we found one. isolate the arguments
		*ptr++ = 0; arg = ptr;

		// skip any extra spaces
		while (*arg == ' ') {
			*arg = 0; arg++;
		}
	}

	#ifdef DEBUG
	logger->log (LOG_INFO, "JUKECLIENT::incoming(): got command [%s] arg [%s]", cmd, arg);
	#endif

	// want help?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_HELP)) {
		// ok now tell them something
		cmdHelp();
		return;
	}

	// disconnect?
	if ((!strcasecmp (cmd, JUKECLIENT_CMD_EXIT)) ||
			(!strcasecmp (cmd, JUKECLIENT_CMD_DISCONNECT1)) ||
			(!strcasecmp (cmd, JUKECLIENT_CMD_DISCONNECT2)) ||
			(!strcasecmp (cmd, JUKECLIENT_CMD_BYE))) {
		// yes. bye bye!
		cmdDisconnect();
		return;
	}

	// set user?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_USER)) {
		// yes. handle it
		cmdUser (arg);
		return;
	}

	// set password?
	if ((!strcasecmp (cmd, JUKECLIENT_CMD_PASSWORD)) ||
			(!strcasecmp (cmd, JUKECLIENT_CMD_PASSWORD2))) {
		// yes. handle it
		cmdPassword (arg);
		return;
	}

	// need to identify ourselves?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_IDENT)) {
		// yes. handle it
		cmdIdent();
		return;
	}

	// need to display status?
	if ((!strcasecmp (cmd, JUKECLIENT_CMD_STATUS)) ||
			(!strcasecmp (cmd, JUKECLIENT_CMD_STATUS2))) {
		// yes. do we need to be authorized for this?
		if ((state == JUKECLIENT_STATE_CONN) && (!config->isAnonStatusAllowed())) {
			// yes, and we are not. complain
			sendf (JUKECLIENT_MSG_MUSTAUTH);
			return;
		}

		// do it
		cmdStatus();
		return;
	}


	// from now on, the user must be authenticated ... is he ?
	if (state == JUKECLIENT_STATE_CONN) {
		// no! bad boy, complain!
		sendf (JUKECLIENT_MSG_MUSTAUTH);
		return;
	}

	// need to display the users?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_USERS)) {
		// yes. handle it
		cmdUsers();
		return;
	}

	// need to display the queue?
	if ((!strcasecmp (cmd, JUKECLIENT_CMD_QUEUE)) ||
			(!strcasecmp (cmd, JUKECLIENT_CMD_QUEUE2))) {
		// yes. handle it
		cmdQueue();
		return;
	}

	// is the player locked and this user NOT an admin?
	if ((player->isLocked()) && (user.status < USER_STATUS_ADMIN)) {
		// yes. complain
		sendf (JUKECLIENT_MSG_LOCKERR);
		return;
	}

	// need to pause?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_PAUSE)) {
		// yes. do it
		cmdPause();
		return;
	}

	// need to continue?
	if ((!strcasecmp (cmd, JUKECLIENT_CMD_CONTINUE)) ||
			(!strcasecmp (cmd, JUKECLIENT_CMD_CONTINUE2))) {
		// yes. handle it
		cmdResume();
		return;
	}

	// need to stop?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_STOP)) {
		// yes. do it
		cmdStop();
		return;
	}

	// need to play?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_PLAY)) {
		// yes. do it
		cmdPlay();
		return;
	}

	// need to skip a song?
	if ((!strcasecmp (cmd, JUKECLIENT_CMD_NEXT)) ||
			(!strcasecmp (cmd, JUKECLIENT_CMD_NEXT2))) {
		// yes. handle it
		cmdSkip();
		return;
	}

	// need to set random play?
	if ((!strcasecmp (cmd, JUKECLIENT_CMD_RANDOM)) ||
			(!strcasecmp (cmd, JUKECLIENT_CMD_RANDOM2))) {
		// yes. handle it
		cmdRandom (arg);
		return;
	}

	// need to remove a queue item?
	if ((!strcasecmp (cmd, JUKECLIENT_CMD_REMOVE)) ||
			(!strcasecmp (cmd, JUKECLIENT_CMD_REMOVE2))) {
		// yes. handle it
		cmdRemove (arg);
		return;
	}

	// need to lock the player?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_LOCK)) {
		// yes. handle it
		cmdLock();
		return;
	}

	// need to unlock the player?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_UNLOCK)) {
		// yes. handle it
		cmdUnlock();
		return;
	}

	// need to clear the player?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_CLEAR)) {
		// yes. handle it
		cmdClear();
		return;
	}

	// need to fetch all albums ?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_ALBUMS)) {
		// yes. handle it
		cmdAlbums();
		return;
	}

	// need to fetch all artists ?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_ARTISTS)) {
		// yes. handle it
		cmdArtists();
		return;
	}

	// need to enqueue a track ?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_ENQUEUETR)) {
		// yes. handle it
		cmdEnqueueTrack (arg);
		return;
	}

	// need to enqueue a track ?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_ENQUEUEAL)) {
		// yes. handle it
		cmdEnqueueAlbum (arg);
		return;
	}

	// need to list an album ?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_LISTALBUM)) {
		// yes. handle it
		cmdListAlbum (arg);
		return;
	}

	// need to fetch an album ?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_GETALBUM)) {
		// yes. handle it
		cmdGetAlbum (arg);
		return;
	}

	// need to fetch an artist?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_GETARTIST)) {
		// yes. handle it
		cmdGetArtist (arg);
		return;
	}

	// need to fetch an track?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_GETTRACK)) {
		// yes. handle it
		cmdGetTrack (arg);
		return;
	}

	// need to fetch all albums by an artist?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_ARTISTALBUMS)) {
		// yes. handle it
		cmdArtistAlbums (arg);
		return;
	}

	// need to fetch or modify the volume?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_VOLUME)) {
		// yes. handle it
		cmdVolume (arg);
		return;
	}

	// need to increase the volume?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_VOLUP)) {
		// yes. handle it
		cmdVolumeUp();
		return;
	}

	// need to decrease the volume?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_VOLDN)) {
		// yes. handle it
		cmdVolumeDown();
		return;
	}

	// need to handle update status?
	if (!strcasecmp (cmd, JUKECLIENT_CMD_UPDATES)) {
		// yes. handle it
		cmdUpdates (arg);
		return;
	}

	// what's this ?
	sendf (JUKECLIENT_MSG_UNKNOWN);
}

/* vim:set ts=2 sw=2: */
