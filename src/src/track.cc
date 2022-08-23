/*
 * track.cc - Jukebox track management code
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "jukebox.h"
#include "track.h"

/*
 * TRACK::TRACK()
 *
 * Creates an empty track object.
 *
 */
TRACK::TRACK() {
	id = artistID = albumID = year = 0; playcount = 0;
	title    = NULL; filename = NULL;
}

/*
 * TRACK::TRACK (int id)
 *
 * This will initialize the object with track [id].
 * 
 */
TRACK::TRACK(int id) {
	// reset the object first
	artistID = albumID = year = trackno = this->id = 0;
	title = filename = NULL;

	// fetch the information from the database
	DBRESULT* res = db->query ("SELECT artistid,albumid,year,title,filename,trackno,playcount FROM tracks WHERE id=#", id);
	if (res == NULL)
		// this failed. oh my...
		throw TrackException();

	// got a result?	
	if (res->numRows() == 0) {
		// no. free the result and throw an exception
		delete res;
		throw TrackException();
	}

	// copy the data
	this->id  = id;
	artistID  = res->fetchColumnAsInteger (0);
	albumID   = res->fetchColumnAsInteger (1);
	year      = res->fetchColumnAsInteger (2);
	title     = strdup (res->fetchColumnAsString (3));
	filename  = strdup (res->fetchColumnAsString (4));
	trackno   = res->fetchColumnAsInteger (5);
	playcount = res->fetchColumnAsInteger (6);

	// all done! ditch the result handle
	delete res;
}

/*
 * TRACK::TRACK (char* fname)
 *
 * This will initialize the object with filename [fname].
 * 
 */
TRACK::TRACK(char* fname) {
	// reset the object first
	artistID = albumID = year = id = playcount = 0;
	title = filename = NULL;

	// fetch the information from the database
	DBRESULT* res = db->query ("SELECT id,artistid,albumid,year,title,filename,trackno,playcount FROM tracks WHERE filename=?", fname);
	if (res == NULL)
		// this failed. oh my...
		throw TrackException();

	// got a result?	
	if (res->numRows() == 0) {
		// no. free the result and throw an exception
		delete res;
		throw TrackException();
	}

	// copy the data
	id          = res->fetchColumnAsInteger (0);
	artistID    = res->fetchColumnAsInteger (1);
	albumID     = res->fetchColumnAsInteger (2);
	year        = res->fetchColumnAsInteger (3);
	title       = strdup (res->fetchColumnAsString (4));
	filename    = strdup (res->fetchColumnAsString (5));
	trackno     = res->fetchColumnAsInteger (6);
	playcount = res->fetchColumnAsInteger (7);

	// all done! ditch the result handle
	delete res;
}

/*
 * TRACK::~TRACK()
 *
 * This will remove the object.
 *
 */
TRACK::~TRACK() {
	if (title)
		free (title);
	if (filename)
		free (filename);
}

/*
 * TRACK::setTitle(char* title)
 *
 * Sets the title of the track.
 *
 */
void
TRACK::setTitle(char* title) {
	// if we have an old title, free it
	if (this->title)
		free (this->title);

	// duplicate the new title
	this->title = strdup (title);
}

/*
 * TRACK::setFilename(char* fname)
 *
 * Sets the filename of the track.
 *
 */
void
TRACK::setFilename(char* fname) {
	// if we have an old filename, free it
	if (filename)
		free (filename);

	// duplicate the new filename
	filename = strdup (fname);
}

/*
 * TRACK::setArtistID(int newid)
 *
 * Sets the ID of the artist of this track.
 *
 */
void TRACK::setArtistID(int newid) { artistID = newid; }

/*
 * TRACK::setAlbumID(int newid)
 *
 * Sets the ID of the album of this track.
 *
 */
void TRACK::setAlbumID(int newid) { albumID = newid; }

/*
 * TRACK::setYear(int year)
 *
 * Sets the ID of the year of this track.
 *
 */
void TRACK::setYear(int year) { this->year = year; }

/*
 * TRACK::setTrackNo (int newno)
 *
 * Sets the track number of this track.
 *
 */
void TRACK::setTrackNo(int newno) { trackno = newno; }

/*
 * TRACK::update()
 *
 * This will update the current track. If the track is new, a new track
 * will be written and getID() will return the correct ID.
 *
 */
void
TRACK::update() {
	// got an ID?
	if (id != 0) {
		// yes. just update the track
		db->execute ("UPDATE tracks SET albumid=#,artistid=#,title=?,filename=?,year=#,trackno=#,playcount=# WHERE id=#", albumID, artistID, title, filename, year, trackno, playcount, id);
		return;
	}

	// no. create a new album
	db->execute ("INSERT INTO tracks (artistid,albumid,title,filename,year,trackno,playcount) VALUES (#,#,?,?,#,#,#)", artistID, albumID, title, filename, year, trackno, playcount);

	// fetch the id
	DBRESULT* res = db->query ("SELECT id FROM tracks WHERE filename=?", filename);
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
 * TRACK::incrementPlaycount().
 *
 * This will increment the track's play count
 *
 */
void
TRACK::incrementPlaycount() {
	playcount++;
}

/* vim:set ts=2 sw=2: */
