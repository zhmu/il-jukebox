/*
 * queue.h
 *
 * This is the jukebox queue manager.
 *
 */
#include <stdlib.h>
#include "album.h"
#include "user.h"

#ifndef __QUEUE_H__
#define __QUEUE_H__

//! \brief QUEUE_MAX_TITLE_LEN is the maximum length of a song title
#define QUEUE_MAX_TITLE_LEN 512

//! \brief QUEUE_MAX_ARTIST_LEN is the maximum length of a song artist
#define QUEUE_MAX_ARTIST_LEN 512

//! \brief QUEUE_MAX_FILENAME_LEN is the maximum length of a filename
#define QUEUE_MAX_FILENAME_LEN 1024

/*!
 * \class QUEUE
 * \brief This will manage the queue.
 */
class QUEUE {
public:
	//! \brief This will set the queue up
	QUEUE();

	/*!
	 * \brief This will retrieve the next track to play.
	 *
	 * This will return zero on failure or non-zero on success.
	 *
	 * \param playid Will be set to the queue item ID.
	 * \param trackid Will be set to the track ID.
	 *
	 */
	int getNextTrackID(int* playid, int* trackid);

	/*!
	 * \brief This will mark a queue item as currently playing.
	 *
	 * Nothing will happen if the supplied queue item ID doesn't exist.
	 *
	 * \param playid The queue item ID
	 *
	 */
	void markPlaying (int playid);

	/*!
	 * \brief This will mark a queue item as not playing.
	 *
	 * Nothing will happen if the supplied queue item ID doesn't exist.
	 *
	 * \param playid The queue item ID
	 *
	 */
	void markNotPlaying (int playid);

	/*!
	 * \brief This will remove a queue item.
	 *
	 * Nothing will happen if the supplied queue item ID doesn't exist.
	 *
	 * \param playid The queue item ID
	 *
	 */
	void remove (int playid);

	//! \brief This will return 1 if the player is randomizing, or 0 if not
	int getRandom();

	/*!
	 * \brief This will enable or disable random play.
	 *
	 * When random play is enabled, the entire queue will be emptied first!
	 *
	 * \param on Non-zero is enable, zero is disable
	 *
	 */
	void setRandom (int on);

	/*!
	 * \brief Retrieves the n-th queue item ID and track ID.
	 *
	 * This function will return non-zero on success. If the queue item does not
	 * exist, or if something fails, zero will be returned.
	 *
	 * \param n The queue item to return
	 * \param playid Will be replaced by the queue item ID
	 * \param trackid Will be replaced by the track ID
	 *
	 */
	int getQueueTrackID (int n,int* playid, int* trackid);

	/*!
	 * \brief Retrieves a queue item's track ID
	 *
	 * This function will return non-zero on success. If the queue item does not
	 * exist, or if something fails, zero will be returned.
	 *
	 * \param id The queue item to return the track ID of
	 * \param trackid Will be replaced by the track ID
	 *
	 */
	int getQueueItem (int id, int* trackid);

	//! \brief Removes all items in the queue
	void clear();

	/*! \brief Enqueues a track
	 *  \param user The user to queue for
	 *  \param id The ID of the track to enqueue
	 *
	 * This will return zero on failure or non-zero on success.
	 */
	 int enqueueTrack (USER* user, int id);

	/*! \brief Enqueues an album
	 *  \param user The user to queue for
	 *  \param id The ID of the album to enqueue
	 *
	 * This will return zero on failure or non-zero on success.
	 */
	int enqueueAlbum (USER* user, int id);

private:
	int random;
};

#endif // __QUEUE_H__

/* vim:set ts=2 sw=2: */
