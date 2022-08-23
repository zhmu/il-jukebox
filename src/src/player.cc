/*
 * player.cc - Jukebox player management code
 *
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include "artist.h"
#include "jukebox.h"
#include "queue.h"
#include "player.h"
#include "track.h"

/*
 * PLAYER::PLAYER()
 * 
 * This will initialize the player.
 *
 */
PLAYER::PLAYER() {
	// no process yet, not playing, not locked
	pid = -1; status = PLAYER_STATUS_IDLE; trackid = playid = -1; locked = 0;
}

/*
 * PLAYER::~PLAYER()
 * 
 * This will deinitialize the player.
 *
 */
PLAYER::~PLAYER() {
	// ignore SIGCHLD signals now
	signal (SIGCHLD, SIG_IGN);

	// do we have a player?
	if (pid != -1) {
		// yes. get rid of it
		kill (pid, SIGKILL);
	}
}

/*
 * PLAYER::getStatus()
 *
 * This will return the player status.
 *
 */
int PLAYER::getStatus() { return status; }

/*
 * PLAYER::isLocked()
 *
 * This will return non-zero if the player is locked, or non-zero if it is not.
 *
 */
int PLAYER::isLocked() { return locked; }

/*
 * PLAYER::getTrackID()
 *
 * This will return the current track playing, or -1 if nothing is playing.
 *
 */
int PLAYER::getTrackID() { return trackid; }

/*
 * PLAYER::getQueueItemID()
 *
 * This will return the current queue item playing, or -1 if nothing is playing.
 *
 */
int PLAYER::getQueueItemID() { return playid; }

/*
 * PLAYER::lock()
 *
 * This will lock the player. 
 *
 */
void PLAYER::lock() { locked = 1; };

/*
 * PLAYER::unlock()
 *
 * This will unlock the player. 
 *
 */
void PLAYER::unlock() { locked = 0; };

/*
 * PLAYER::launch()
 *
 * This will launch the player process, so that it will play the file currently
 * on the playlist.
 *
 */
void
PLAYER::launch() {
	int i, st;
	char* argv[PLAYER_MAX_ARGS];
	char* ptr;
	char* ptr2;
	char  fname[QUEUE_MAX_FILENAME_LEN];
	char  title[QUEUE_MAX_TITLE_LEN];
	char  artist[QUEUE_MAX_ARTIST_LEN];
  char  tmp[256 /* XXX */ ];
	char* playcmd;
	TRACK* t;

	// do we even need to play?
	if (status == PLAYER_STATUS_IDLE)
		// no. just return
		return;

	// do we have a valid pid?
	if (pid != -1) {
		// yes. is that process truely dead?
		i = waitpid (pid, &st, WNOHANG | WUNTRACED);
		if ((i == pid) && (WIFSTOPPED (st)))
			// no, it just stopped. return
			return;
	}

	// did we used to have a play item?
	if (playid != -1) {
		// get rid of it
		queue->remove (playid);
		playid = -1;
	}

	// fetch the next track
	if (!queue->getNextTrackID (&playid, &trackid)) {
		// this failed. don't play anything
		pid = -1; status = PLAYER_STATUS_IDLE;
		return;
	}

	// got a track id?
	if (trackid == -1) {
		// no. nothing to play
		pid = -1;
		return;
	}

	/*
	 * Do all database interaction *BEFORE* fork()-ing. When the databases are
	 * linked without thread support, they tend to re-use the same memory, which
	 * seems to cause weird problems later on.
	 */

	// grab the title, artist and filename
	try {
		t = new TRACK (trackid);
		strncpy (title, t->getTitle(), sizeof (title));
		strncpy (fname, t->getFilename(), sizeof (fname));
	} catch (TrackException e) {
		logger->log (LOG_ERR, "Track %u from queue doesn't exist???", trackid);
		return;
	}

	// fetch the artist
	try {
		ARTIST* a = new ARTIST (t->getArtistID());
		strncpy (artist, a->getName(), sizeof (artist));
		delete a;
	} catch (ArtistException e) {
		strcpy (artist, "?");
	}

	// increment the track's play count and store it
	t->incrementPlaycount();
	t->update();

	// track is no longer needed now
	delete t;
	
	// figure out the file type
	ptr = strrchr (fname, '.');
	if (ptr == NULL) {
		// this failed. complain
		logger->log (LOG_ERR, "Track %u ['%s'] has a filename without extension, skipped\n", trackid, fname);
		return;
	}

	// look the player up
	ptr++;
	if (!config->lookupPlayer (ptr, &playcmd)) {
		// this failed. complain
		logger->log (LOG_ERR, "No registered player for '%s' files\n", ptr);
		return;
	}

	// inform all clients
	snprintf (tmp, sizeof (tmp) - 1, JUKECLIENT_UPDATE_SONG, 'N', artist, title);
	server->sendUpdate (tmp);

	// final logging
	logger->log (LOG_INFO, "Now playing %s - %s", artist, title);
	
	// mark the queue item as being played
	queue->markPlaying (playid);

	// fork!
	i = fork();

	// master process?
	if (i) {
		// yes. store the pid and return
		pid = i;
		return;
	}

	// distance ourselves as much from mpg123 as possible ...
	if (setsid() < 0)
		// this failed. log it, but it's not fatal...
		logger->log (LOG_NOTICE, "setsid(): failed");

	// redirect all descriptors to /dev/null, we want the player to be totally
	// quiet
	int fd = open ("/dev/null", O_RDWR);
	if (fd < 0) {
		// yikes, this failed! log this, this is bad ...
		logger->log (LOG_CRIT, "cannot open /dev/null for writing ???");
		exit (EXIT_FAILURE);
	}

	// do the redirection
	dup2 (fd, STDIN_FILENO);
	dup2 (fd, STDOUT_FILENO);
	dup2 (fd, STDERR_FILENO);

	/*
	 * The next line makes the main jukebox process fail with ECONNABORTED. Oddly,
   * it works fine if using the non-shared library. It doesn't matter anyway,
   * because lib++ opens sockets and uses FD_CLOEXEC to ensure our child process
   * can't mess with sockets... but if anyone can explain this to me, I'd be
   * thankful.
	 *
	 * Both FreeBSD and Linux had this behaviour.
	 *
	 * delete server;
	 *
	 */

	// copy the player name and parameters
	ptr = playcmd; i = 0; st = 1;
	while (st) {
		// look for a space, or use the null char if we have none
		ptr2 = strchr (ptr, ' ');
		if (ptr2 == NULL) { ptr2 = strchr (ptr, 0); st = 0; }

		// skip any spaces now
		while (*ptr2 == ' ') { *ptr2 = 0; ptr2++; }

		// set the pointer
		argv[i++] = ptr;
		if (i >= (PLAYER_MAX_ARGS - 2)) {
			// too much arguments. bail out
			logger->log (LOG_ALERT, "Too much arguments for player '%s'\n", argv[0]);
			exit (EXIT_FAILURE);
		}

		// next
		ptr = ptr2;
	}

	// add the filename and NULL
	argv[i++] = fname;
	argv[i  ] = NULL;
	if (execv (argv[0], argv) < 0) {
		// then again, maybe we did not ... log the error and exit still
		logger->log (LOG_ALERT, "Player '%s' didn't start?", argv[0]);
		exit (EXIT_FAILURE);
	}

	/* NOTREACHED */
}

/*
 * PLAYER::pause()
 *
 * This will pause the player. 
 *
 */
void
PLAYER::pause() {
	// are we currently playing?
	if (status != PLAYER_STATUS_PLAYING)
		// no. leave
		return;

	// we're pausing now
	status = PLAYER_STATUS_PAUSED;

	// suspend the thread
	kill (pid, SIGSTOP);
}

/*
 * PLAYER::resume()
 *
 * This will resume the player. 
 *
 */
void
PLAYER::resume() {
	// are we currently paused?
	if (status != PLAYER_STATUS_PAUSED)
		// no. leave
		return;

	// we're playing now
	status = PLAYER_STATUS_PLAYING;

	// resume the thread
	kill (pid, SIGCONT);
}

/*
 * PLAYER::play()
 *
 * This will start playing.
 *
 */
void
PLAYER::play() {
	// are we idle?
	if (status == PLAYER_STATUS_IDLE) {
		// yes. change the status
		status = PLAYER_STATUS_PLAYING;

		// launch!
		launch();
	}
}

/*
 * PLAYER::stop()
 *
 * This will stop playing.
 *
 */
void
PLAYER::stop() {
	// are we already stopped?
	if (status == PLAYER_STATUS_IDLE)
		// yes. just leave
		return;

	// set the status
	status = PLAYER_STATUS_IDLE;
	
	// a farewell to processes
	kill (pid, SIGKILL);

	// wait until it is gone, this avoids those annoying zombie processes
	waitpid (pid, (int*)0, 0);

	// if we have a play item id, mark it as not playing
	if (playid != -1)
		queue->markNotPlaying (playid);

	// no process anymore
	pid = -1;
}

/*
 * PLAYER::next()
 *
 * This will switch to the next track.
 *
 */
void
PLAYER::next() {	
	// stop playing
	stop();

	// remove the current queue item
	queue->remove (playid);

	// next
	play();
}

/* vim:set ts=2 sw=2: */
