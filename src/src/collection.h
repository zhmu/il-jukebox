/*
 * collection.h
 *
 * This is the jukebox collection manager.
 *
 */
#include <stdlib.h>

#ifndef __COLLECTION_H__
#define __COLLECTION_H__

//! \brief COLLECTION_MAX_NAME_LEN is the maximum length of a collection name
#define COLLECTION_MAX_NAME_LEN	128

/*!
 * \class CollectionException
 * \brief This indicates a failure within the album
 */
class CollectionException {
};

/*!
 * \class COLLECTION
 * \brief This will manage a collection
 */
class COLLECTION {
public:
	/*! \brief This will load a collection from the database
	 *
	 * This function will throw a CollectionException on failure.
	 *
	 * \param id The collection to be loaded
	 */
	COLLECTION(int id);

	//! \brief This will create an empty collection
	COLLECTION();

	//! \brief This will destroy the collection object
	~COLLECTION();

	/*! \brief Updates the databases
	 *
	 * If the collection is new, a new record will be created and getID() will
	 * return a valid id.
	 *
	 * getID() will keep returning 0 if creating a new collection fails.
	 */
	void update();

	/*! \brief Sets the collection's name
	 *
	 * \param newname The new name of the collection
	 */
	void setName (char* newname);

	/*! \brief Sets the collection's owner's ID.
	 *
	 * \param newid The new ID of the owner
	 */
	void setOwnerID (int newid);

	//! \brief Returns the collection's ID
	inline int getID() { return id; }

	//! \brief Returns the collection's owner ID
	inline int getOwnerID() { return ownerID; }

	//! \brief Returns the collection's name
	inline char* getName() { return name; }

private:
	int id;
	int ownerID;
	char*	name;
};

#endif /* __COLLECTION_H__ */

/* vim:set ts=2 sw=2: */
