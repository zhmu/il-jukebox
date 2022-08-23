/*
 * album.cc - Jukebox album management code
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "album.h"
#include "jukebox.h"

/*
 * ALBUM::ALBUM()
 *
 * This will initialize an empty object.
 * 
 */
ALBUM::ALBUM() {
	id = artistID = 0; this->name = NULL;
}

/*
 * ALBUM::ALBUM (int id)
 *
 * This will initialize the object with album [id].
 * 
 */
ALBUM::ALBUM(int id) {
	// reset the object first
	this->id = artistID = 0; name = NULL;

	// fetch the information from the database
	DBRESULT* res = db->query ("SELECT artistid,name FROM albums WHERE id=#", id);
	if (res == NULL)
		// this failed. oh my...
		throw AlbumException();

	// got a result?	
	if (res->numRows() == 0) {
		// no. free the result and throw an exception
		delete res;
		throw AlbumException();
	}

	// copy the data
	this->id = id;
	artistID = res->fetchColumnAsInteger (0);
	name     = strdup (res->fetchColumnAsString (1));

	// all done! ditch the result handle
	delete res;
}

/*
 * ALBUM::ALBUM (char* name, int artistid)
 *
 * This will initialize the object with album [name] and artist id [artistid].
 * 
 */
ALBUM::ALBUM(char* name, int artistid) {
	// reset the object first
	id = artistID = 0; this->name = NULL;

	// fetch the information from the database
	DBRESULT* res = db->query ("SELECT id,artistid,name FROM albums WHERE name=? AND artistid=#", name, artistid);
	if (res == NULL)
		// this failed. oh my...
		throw AlbumException();

	// got a result?	
	if (res->numRows() == 0) {
		// no. free the result and throw an exception
		delete res;
		throw AlbumException();
	}

	// copy the data
	this->id = res->fetchColumnAsInteger (0);
	artistID = res->fetchColumnAsInteger (1);
	name     = strdup (res->fetchColumnAsString (2));

	// all done! ditch the result handle
	delete res;
}

/*
 * ALBUM::~ALBUM()
 *
 * This will remove the object.
 *
 */
ALBUM::~ALBUM() {
	if (name)
		free (name);
}

/*
 * ALBUM::update()
 *
 * This will update the current album. If the album is new, a new album
 * will be written and getID() will return the correct ID.
 *
 */
void
ALBUM::update() {
	// got an ID?
	if (id != 0) {
		// yes. just update the album
		db->execute ("UPDATE albums SET name=?,artistid=# WHERE id=#", name, artistID, id);
		return;
	}

	// no. create a new artist
	db->execute ("INSERT INTO albums (artistid,name) VALUES (#,?)", artistID, name);

	// fetch the id
	DBRESULT* res = db->query ("SELECT id FROM albums WHERE name=? AND artistID=#", name, artistID);
	if (res == NULL) {
		// this shouldn't happen...
	 id = 0;
	 return;
	}

	id = res->fetchColumnAsInteger (0);

	// all done
	delete res;
}

/*
 * ALBUM::setName (char* newname)
 *
 * Sets the name of the album to [newname].
 *
 */
void
ALBUM::setName (char* newname) {
	// if we have an old name, free it first
	if (name)
		free (name);

	// copy the name
	name = strdup (newname);
}

/*
 * ALBUM::setArtistID (int newid)
 *
 * Sets the artist ID to [newid].
 *
 */
void ALBUM::setArtistID (int newid) { artistID = newid; }

/*
 * ALBUM::fetchNext()
 *
 * This will try to fetch the next album in place. It will
 * return 0 on failure or non-zero on success.
 *
 */
int
ALBUM::fetchNext() {
	// fetch the information from the database
	DBRESULT* res = db->limitQuery ("SELECT id,artistid,name FROM albums WHERE id># ORDER BY id ASC", 1, 0, id);
	if (res == NULL)
		// this failed. oh my...
		return 0;

	// got a result?	
	if (res->numRows() == 0) {
		// no. free the result and return failure
		delete res;
		return 0;
	}

	// free the name first, if needed
	if (name)
		free (name);

	// copy the data
	this->id = res->fetchColumnAsInteger (0);
	artistID = res->fetchColumnAsInteger (1);
	name     = strdup (res->fetchColumnAsString (2));

	delete res;

	// all done
	return 1;
}

/*
 * ALBUM::fetchTrack (int pos, int* trackid)
 *
 * This will the ID of album's track into [trackid]. [pos] is the
 * number of the track to fetch. This will return zero on failure or
 * non-zero on success.
 *
 */
int
ALBUM::fetchTrack (int pos, int* trackid) {
	// fetch the information from the database
	DBRESULT* res = db->limitQuery ("SELECT id FROM tracks WHERE albumid=# ORDER BY trackno ASC", 1, pos, id);
	if (res == NULL)
		// this failed. oh my...
		return 0;

	// got a result?	
	if (res->numRows() == 0) {
		// no. free the result and return failure
		delete res;
		return 0;
	}

	// copy the data
	*trackid = res->fetchColumnAsInteger (0);
	delete res;

	// all done
	return 1;
}

/*
 * ALBUM::fetchArtistNext(int artistid)
 *
 * This will try to fetch the next album for artist [artistid]. It will
 * return 0 on failure or non-zero on success.
 */
int
ALBUM::fetchArtistNext(int artistid) {
	// fetch the information from the database
	DBRESULT* res = db->limitQuery ("SELECT id,artistid,name FROM albums WHERE artistid=# AND id># ORDER BY id ASC", 1, 0, artistid, id);
	if (res == NULL)
		// this failed. oh my...
		return 0;

	// got a result?	
	if (res->numRows() == 0) {
		// no. free the result and return failure
		delete res;
		return 0;
	}

	// free the name first, if needed
	if (name)
		free (name);

	// copy the data
	this->id = res->fetchColumnAsInteger (0);
	artistID = res->fetchColumnAsInteger (1);
	name     = strdup (res->fetchColumnAsString (2));

	delete res;

	// all done
	return 1;
}


/* vim:set ts=2 sw=2: */
