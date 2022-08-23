/*
 * track.h
 *
 * This is the jukebox track manager.
 *
 */
#include <stdlib.h>

#ifndef __TRACK_H__
#define __TRACK_H__

//! \brief TRACK_MAX_TITLE_LEN is the maximum length of a song title
#define TRACK_MAX_TITLE_LEN 512

//! \brief TRACK_MAX_ARTIST_LEN is the maximum length of a song artist
#define QUEUE_MAX_ARTIST_LEN 512

//! \brief TRACK_MAX_FILENAME_LEN is the maximum length of a filename
#define QUEUE_MAX_FILENAME_LEN 1024

/*!
 * \class TrackException
 * \brief This indicates a failure within the tracks.
 */
class TrackException {
};

/*!
 * \class TRACK
 * \brief This will manage a track
 */
class TRACK {
public:
	/*! \brief This will load a track from the database
	 *
	 * This function will throw a TrackException on failure.
	 *
	 * \param id The track to be loaded
	 */
	TRACK(int id);

	/*! \brief This will load a track from the database
	 *
	 * This function will throw a TrackException on failure.
	 *
	 * \param fname The filename of the track to be loaded
	 */
	TRACK(char* fname);

	//! \brief This will create a blank track.
	TRACK();

	//! \brief This will destroy the track object
	~TRACK();

	/*! \brief Updates the databases
	 *
	 * If the album is new, a new record will be created and getID() will
	 * return a valid id.
	 *
	 * getID() will keep returning 0 if creating a new album fails.
	 */
	void update();

	/*! \brief Sets a new title
	 *
	 * \param newtitle The new title of the track
	 */
	void setTitle (char* newtitle);

	/*! \brief Sets a new filename
	 *
	 * \param fname The filename of the track
	 */
	void setFilename (char* fname);

	/*! \brief Sets a new artist ID
	 *
	 * \param newid The new artist ID of the track
	 */
	void setArtistID (int newid);

	/*! \brief Sets a new album ID
	 *
	 * \param newid The new album ID of the track
	 */
	void setAlbumID (int newid);

	/*! \brief Sets a new year
	 *
	 * \param year The new year of the track
	 */
	void setYear (int year);

	/*! \brief Sets the track number
	 *
	 * \param newno The new track number
	 */
	void setTrackNo (int newtrackno);

	//! \brief Increments the track's play count
	void incrementPlaycount();

	//! \brief Returns the track's title
	inline char* getTitle() { return title; }

	//! \brief Returns the track's filename
	inline char* getFilename() { return filename; }

	//! \brief Returns the artist's ID
	inline int getArtistID() { return artistID; }

	//! \brief Returns the album's ID
	inline int getAlbumID() { return albumID; }

	//! \brief Returns the track number
	inline int getTrackNo() { return trackno; }

	//! \brief Returns the track's ID
	inline int getID() { return id; }

	//! \brief Returns the track's play count
	inline int getPlaycount() { return playcount; }

private:
	int   id;
	int   artistID;
	int   albumID;
	char*	title;
	char*	filename;
	int   year;
	int   trackno;
	int   playcount;
};

#endif /* __TRACK_H__ */

/* vim:set ts=2 sw=2: */
