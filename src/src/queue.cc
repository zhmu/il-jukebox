/*
 * queue.cc - Jukebox queue management code
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "jukebox.h"
#include "queue.h"
#include "track.h"

/*
 * QUEUE::QUEUE()
 *
 * This will initialize the queue manager.
 * 
 */
QUEUE::QUEUE() {
	// zap any old crud
	db->execute ("DELETE FROM queue WHERE playtime IS NOT NULL");

	// not random-playing
	random = 0;
}

/*
 * QUEUE::getNextTrackID()
 *
 * This will fetch the track ID of the song that should be playing in [trackid],
 * with the line id in [playid]. it will return 1 on success or 0 on failure.
 *
 */
int
QUEUE::getNextTrackID(int* trackid, int* playid) {
	DBRESULT* res;
	char now[64 /* XXX */];
	time_t curtime;

	time (&curtime);

	// are we randomizing?
	if (random) {
		// yes. grab the total number of tracks
		res = db->query ("SELECT COUNT(id) FROM tracks");
		if (res == NULL)
			// this failed. too bad
			return 0;
		int num = res->fetchColumnAsInteger (0);
		delete res;

		// fetch a random track id
		res = db->limitQuery ("SELECT id FROM tracks", 1, ::random() % num);
		if (res == NULL) {
			// this failed. too bad
			return 0;
		}

		// got a result?	
		if (res->numRows() == 0) {
			// no. free the result and return failure
			delete res;
			return 0;
		}

		// fetch the track id and remove the results
		int tid = res->fetchColumnAsInteger (0);
		delete res;

		// insert the new item
		time (&curtime);
		strftime (now, sizeof (now), "%Y-%m-%d %H:%M:%S", localtime (&curtime));
		db->execute ("INSERT INTO queue (trackid,timestamp) VALUES (#,?)", tid, now);
	}

	// fetch the information
	res = db->limitQuery ("SELECT id,trackid FROM queue WHERE playtime IS NULL ORDER BY id,timestamp ASC", 1, 0);
	if (res == NULL) {
		// this failed. too bad
		return 0;
	}

	// got any results?
	if (res->numRows() == 0) {
		// no. no tracks then
		delete res;
		return 0;
	}

	// fetch the track number
	*trackid = res->fetchColumnAsInteger (0);
	*playid = res->fetchColumnAsInteger (1);

	// drop the result and return the id
	delete res;
	return 1;
}

/*
 * QUEUE::markPlaying (int id)
 *
 * This will mark queue item [id] as being played.
 *
 */
void
QUEUE::markPlaying (int id) {
	char now[64 /* XXX */];
	time_t curtime;

	// mark the song as being played
	time (&curtime);
	strftime (now, sizeof (now), "%Y-%m-%d %H:%M:%S", localtime (&curtime));
	db->execute ("UPDATE queue SET playtime=? WHERE id=#", now, id);
}

/*
 * QUEUE::markNotPlaying (int id)
 *
 * This will mark queue item [id] as not being played.
 *
 */
void
QUEUE::markNotPlaying (int id) {
	// mark the song as being played
	db->execute ("UPDATE queue SET playtime=NULL WHERE id=#",id);
}

/*
 * QUEUE::remove (int id)
 *
 * This will remove queue item [id].
 *
 */
void
QUEUE::remove (int id) {
	// bye!
	db->execute ("DELETE FROM queue WHERE id=#",id);
}

/*
 * QUEUE::getRandom()
 *
 * This will return 0 if randomized play is turned off, or 1 if it is on.
 *
 */
int QUEUE::getRandom() { return random; }

/*
 * QUEUE::setRandom (int on)
 *
 * This will set random play to [on].
 *
 */
void
QUEUE::setRandom (int on) {
	// just do it
	random = on;

	// random play enabled?
	if (random) {
		// yes. kill the playlist
		db->execute ("DELETE FROM queue");
	}
}

/*
 * QUEUE::getQueueTrackID (int n, int* playid)
 *
 * This will return the n-th track ID in the queue into [trackid]. [playid] will
 * be set to the queue item ID. This function will return zero on failure or
 * non-zero on success.
 *
 */
int
QUEUE::getQueueTrackID (int n, int* playid, int* trackid) {
	// fetch the track id
	DBRESULT* res = db->limitQuery ("SELECT id,trackid FROM queue ORDER BY timestamp,id ASC", 1, n);
	if (res == NULL) {
		// this failed. oh my...
		return 0;
	}

	// got any results?
	if (res->numRows() == 0) {
		// no. free the result and return a no-no
		delete res;
		return 0;
	}

	// fetch the queue item id and track id
	*playid = res->fetchColumnAsInteger (0);
	*trackid = res->fetchColumnAsInteger (1);

	// free the result and return victory
	delete res;
	return 1;
}

/*
 * QUEUE::getQueueItem (int id, int* trackid)
 *
 * This will fetch the track ID of queue item [id]. This function will
 * return zero on failure or non-zero on success.
 *
 */
int
QUEUE::getQueueItem (int id, int* trackid) {
	// fetch the track id
	DBRESULT* res = db->query ("SELECT trackid FROM queue WHERE id=#", id);
	if (res == NULL) {
		// this failed. oh my...
		return 0;
	}

	// got any results?
	if (res->numRows() == 0) {
		// no. free the result and return a no-no
		delete res;
		return 0;
	}

	// fetch the queue item id and track id
	*trackid = res->fetchColumnAsInteger (0);

	// free the result and return victory
	delete res;
	return 1;
}

/*
 * QUEUE::clear()
 *
 * This will remove all items in the queue.
 *
 */
void
QUEUE::clear() {
	// kill the playlist
	db->execute ("DELETE FROM queue");
}

/*
 * QUEUE::enqueueTrack (USER* user, int id)
 *
 * This will enqueue track [id] for user [userid]. It will return zero on
 * failure and non-zero on success.
 *
 */
int
QUEUE::enqueueTrack (USER* user, int id) {
	char now[64 /* XXX */];
	time_t curtime;

	// ensure the track exists
	try {
		// fetch the track
		TRACK* track = new TRACK (id);

		// just insert it
		time (&curtime);
		strftime (now, sizeof (now), "%Y-%m-%d %H:%M:%S", localtime (&curtime));
		db->execute ("INSERT INTO queue (trackid,timestamp) VALUES (#,?)", track->getID(), now);

		// bye bye
		delete track;
	} catch (TrackException e) {
		// no such track
		return 0;
	}

	// victory
	return 1;
}

/*
 * QUEUE::enqueueAlbum (USER* user, int id)
 *
 * This will enqueue album [id] for user [userid]. It will return zero on
 * failure and non-zero on success.
 *
 */
int
QUEUE::enqueueAlbum (USER* user, int id) {
	int pos = 0;
	int trackid;

	try {
		// try to fetch the album
		ALBUM* album = new ALBUM (id);

		// keep fetching tracks
		while (album->fetchTrack (pos, &trackid)) {
			// this worked. enqueue it
			enqueueTrack (user, trackid);

			// next
			pos++;
		}

		// enough is enough
		delete album;
	} catch (AlbumException e) {
		// bummer
		return 0;
	}

	// victory
	return 1;
}


/* vim:set ts=2 sw=2: */
