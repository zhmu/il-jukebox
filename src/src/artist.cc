/*
 * artist.cc - Jukebox artist management code
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "artist.h"
#include "jukebox.h"

/*
 * ARTIST::ARTIST ()
 *
 * This will initialize the object with no artist.
 * 
 */
ARTIST::ARTIST() {
	// no id or artist
	id = 0; name = NULL;
}

/*
 * ARTIST::ARTIST (int id)
 *
 * This will initialize the object with artist [id].
 * 
 */
ARTIST::ARTIST(int id) {
	// reset the object first
	name = NULL; this->id = id;

	// fetch the information from the database
	DBRESULT* res = db->query ("SELECT name FROM artists WHERE id=#", id);
	if (res == NULL)
		// this failed. oh my...
		throw ArtistException();

	// got a result?	
	if (res->numRows() == 0) {
		// no. free the result and throw an exception
		delete res;
		throw ArtistException();
	}

	// copy the data
	name     = strdup (res->fetchColumnAsString (0));

	// all done! ditch the result handle
	delete res;
}

/*
 * ARTIST::ARTIST (const char* name)
 *
 * This will initialize the object with artist [name].
 * 
 */
ARTIST::ARTIST(const char* name) {
	// reset the object first
	id = 0; this->name = NULL;

	// fetch the information from the database
	DBRESULT* res = db->query ("SELECT id,name FROM artists WHERE name=?", name);
	if (res == NULL)
		// this failed. oh my...
		throw ArtistException();

	// got a result?	
	if (res->numRows() == 0) {
		// no. free the result and throw an exception
		delete res;
		throw ArtistException();
	}

	// copy the data
	id       = res->fetchColumnAsInteger (0);
	name     = strdup (res->fetchColumnAsString (1));

	// all done! ditch the result handle
	delete res;
}

/*
 * ARTIST::~ARTIST()
 *
 * This will remove the object.
 *
 */
ARTIST::~ARTIST() {
	if (name)
		free (name);
}

/*
 * ARTIST::setName (char* newname)
 *
 * Sets the name of the artist to [newname].
 *
 */
void
ARTIST::setName (char* newname) {
	// if we have an old name, free it first
	if (name)
		free (name);

	// copy the name
	name = strdup (newname);
}

/*
 * ARTIST::update()
 *
 * This will update the current artist. If the artist is new, a new artist
 * will be written and getID() will return the correct ID.
 *
 */
void
ARTIST::update() {
	// got an ID?
	if (id != 0) {
		// yes. just update the artist
		db->execute ("UPDATE artists SET name=? WHERE id=#", name, id);
		return;
	}

	// no. create a new artist
	db->execute ("INSERT INTO artists (name) VALUES (?)", name);

	// fetch the id
	DBRESULT* res = db->query ("SELECT id FROM artists WHERE name=?", name);
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
 * ARTIST::fetchNext()
 *
 * This will try to fetch the next artist in place. It will
 * return 0 on failure or non-zero on success.
 *
 */
int
ARTIST::fetchNext() {
	// fetch the information from the database
	DBRESULT* res = db->limitQuery ("SELECT id,name FROM artists WHERE id># ORDER BY id ASC", 1, 0, id);
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
	id       = res->fetchColumnAsInteger (0);
	name     = strdup (res->fetchColumnAsString (1));

	delete res;

	// all done
	return 1;
}

// vim:set ts=2 sw=2:
