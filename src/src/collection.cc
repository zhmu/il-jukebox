/*
 * collection.cc - Jukebox collection management code
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "collection.h"
#include "jukebox.h"

/*
 * COLLECTION::COLLECTION ()
 *
 * This will initialize the object with no collection.
 * 
 */
COLLECTION::COLLECTION() {
	// no id, owner or artist
	id = 0; ownerID = 0; name = NULL;
}

/*
 * COLLECTION::COLLECTION (int id)
 *
 * This will initialize the object with collection [id].
 * 
 */
COLLECTION::COLLECTION(int id) {
	// reset the object first
	name = NULL; this->id = id; this->ownerID = 0;

	// fetch the information from the database
	DBRESULT* res = db->query ("SELECT name,ownerid FROM collections WHERE id=#", id);
	if (res == NULL)
		// this failed. oh my...
		throw CollectionException();

	// got a result?	
	if (res->numRows() == 0) {
		// no. free the result and throw an exception
		delete res;
		throw CollectionException();
	}

	// copy the data
	name    = strdup (res->fetchColumnAsString (0));
	ownerID = res->fetchColumnAsInteger (1);

	// all done! ditch the result handle
	delete res;
}

/*
 * COLLECTION::~COLLECTION()
 *
 * This will remove the object.
 *
 */
COLLECTION::~COLLECTION() {
	if (name)
		free (name);
}

/*
 * COLLECTION::setName (char* newname)
 *
 * Sets the name of the collection to [newname].
 *
 */
void
COLLECTION::setName (char* newname) {
	// if we have an old name, free it first
	if (name)
		free (name);

	// copy the name
	name = strdup (newname);
}

/*
 * COLLECTION::update()
 *
 * This will update the current collection. If the collection is new, a new
 * collection will be created and getID() will return the correct ID.
 *
 */
void
COLLECTION::update() {
	// got an ID?
	if (id != 0) {
		// yes. just update the artist
		db->execute ("UPDATE collections SET name=?,ownerid=? WHERE id=#", name, ownerID, id);
		return;
	}

	// no. create a new artist
	db->execute ("INSERT INTO collections (name,ownerid) VALUES (?,?)", name, ownerID);

	// fetch the id
	DBRESULT* res = db->query ("SELECT id FROM collections WHERE name=?", name);
	if (res == NULL) {
		// this shouldn't happen...
	 id = 0;
	 return;
	}

	id = res->fetchColumnAsInteger (0);

	// all done
	delete res;
}

// vim:set ts=2 sw=2:
