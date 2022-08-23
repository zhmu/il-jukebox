/*
 * album.h
 *
 * This is the jukebox album manager.
 *
 */
#include <stdlib.h>

#ifndef __ALBUM_H__
#define __ALBUM_H__

//! \brief ALBUM_MAX_LEN is the maximum length of a song album
#define ALBUM_MAX_LEN 512

/*!
 * \class AlbumException
 * \brief This indicates a failure within the album
 */
class AlbumException {
};

/*!
 * \class ALBUM
 * \brief This will manage an album
 */
class ALBUM {
public:
	/*! \brief This will load an album from the database
	 *
	 * This function will throw an ArtistException on failure.
	 *
	 * \param id The album to be loaded
	 */
	ALBUM(int id);

	/*! \brief This will load an album from the database
	 *
	 * This function will throw an ArtistException on failure.
	 *
	 * \param name The album to be loaded
	 * \param artistid The ID of the artist of which album to load
	 */
	ALBUM(char* name, int artistid);

	//! \brief This will create an empty album
	ALBUM();

	//! \brief This will destroy the album object
	~ALBUM();

	/*! \brief Updates the databases
	 *
	 * If the album is new, a new record will be created and getID() will
	 * return a valid id.
	 *
	 * getID() will keep returning 0 if creating a new album fails.
	 */
	void update();

	/*! \brief Sets the album's name
	 *
	 * \param newname The new name of the album
	 */
	void setName (char* newname);

	/*! \brief Sets the album's artist ID.
	 *
	 * \param newid The new ID of the artist
	 */
	void setArtistID (int newid);

	//! \brief Returns the album's ID
	inline int getID() { return id; }

	//! \brief Returns the album's artist ID
	inline int getArtistID() { return artistID; }

	//! \brief Returns the album's name
	inline char* getName() { return name; }

	/*! \brief Fetches the next available album
	 *
	 *  This will return zero on failure or non-zero on success.
	 */
	int fetchNext();

	/*! \brief Fetches the next available album for an artist
	 *  \param artistid The artist ID to fetch for
	 *
	 *  This will return zero on failure or non-zero on success.
	 */
	int fetchArtistNext (int artistid);

	/*! \brief Fetches a track id for this album
	 *	\param pos Which track to fetch
	 *  \param trackid Will be set to the track id
	 *
	 *  This will return zero on failure or non-zero on success.
	 */
	int fetchTrack (int pos, int* trackid);

private:
	int id;
	int artistID;
	char*	name;
};

#endif /* __ALBUM_H__ */

/* vim:set ts=2 sw=2: */
