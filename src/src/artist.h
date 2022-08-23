/*
 * artist.h
 *
 * This is the jukebox artist manager.
 *
 */
#include <stdlib.h>

#ifndef __ARTIST_H__
#define __ARTIST_H__

//! \brief ARTIST_MAX_LEN is the maximum length of a song artist
#define ARTIST_MAX_LEN 512

/*!
 * \class ArtistException
 * \brief This indicates a failure within the artist.
 */
class ArtistException {
};

/*!
 * \class ARTIST
 * \brief This will manage an artist
 */
class ARTIST {
public:
	//! \brief This will create a blank artist
	ARTIST();

	/*! \brief This will load an artist from the database
	 *
	 * This function will throw an ArtistException on failure.
	 *
	 * \param id The artist to be loaded
	 */
	ARTIST(int id);

	/*! \brief This will load an artist from the database
	 *
	 * This function will throw an ArtistException on failure.
	 *
	 * \param name The name of the artist to be loaded
	 */
	ARTIST(const char* name);

	//! \brief This will destroy the artist object
	~ARTIST();

	/*! \brief Updates the databases
	 *
	 * If the artist is new, a new record will be created and getID() will
	 * return a valid id.
	 *
	 * getID() will keep returning 0 if creating a new artist fails.
	 */
	void update();

	/*! \brief Sets the artist's name
	 *
	 * \param newname The new name of the artist
	 */
	void setName (char* newname);

	//! \brief Returns the artist's ID
	inline int getID() { return id; }

	//! \brief Returns the artist's name
	inline char* getName() { return name; }

	/*! \brief Fetches the next available artist
	 *
	 *  This will return zero on failure or non-zero on success.
	 */
	int fetchNext();

private:
	int id;
	char*	name;
};

#endif /* __ARTIST_H__ */

/* vim:set ts=2 sw=2: */
