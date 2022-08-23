/*
 * player.h
 *
 * This is the jukebox file player.
 *
 */
#include <stdlib.h>

#ifndef __PLAYER_H__
#define __PLAYER_H__

#define PLAYER_MAX_ARGS				16

#define PLAYER_STATUS_IDLE			0
#define PLAYER_STATUS_PLAYING			1
#define PLAYER_STATUS_PAUSED			2

/*!
 * \class PLAYER
 * \brief This will supervise the playing of MP3's.
 *
 */
class PLAYER {
public:
	//! \brief This will initialize the player.
	PLAYER();

	//! \brief This will deinitialize the player.
	~PLAYER();

	/*!
	 * \brief This will fork over and spawn the MP3 player.
	 *
	 * If the player status is not playing, this function will do nothing.
	 *
	 */
	void launch();

	//! \brief This will retrieve the current status.
	int		getStatus();

	/*!
	 * \brief This will retrieve the currently playing track ID.
	 *
	 * -1 will be returned if nothing is playing.
	 *
	 */
	int		getTrackID();

	/*!
	 * \brief This will retrieve the currently playing queue item ID.
	 *
	 * -1 will be returned if nothing is playing.
	 *
	 */
	int		getQueueItemID();

	/*!
	 * \brief This will return non-zero if the player is locked, or non-zero if
	 *        it is not.
	 */
	int		isLocked();

	/*!
	 * \brief This will pause playback.
	 *
	 * If the player is not playing, nothing will happen.
	 *
	 */
	void	pause();

	/*!
	 * \brief This will resume playback.
	 *
	 * If the player is not paused, nothing will happen.
	 *
	 */
	void	resume();

	/*!
	 * \brief This will start playback.
	 *
	 * If the player is not stopped, nothing will happen.
	 *
	 */
	void	play();

	/*!
	 * \brief This will stop playback.
	 *
	 * If the player is not playing, nothing will happen.
	 *
	 */
	void	stop();

	/*!
	 * \brief This will skip the current song.
	 *
	 * If the player is not playing, nothing will happen.
	 *
	 */
	void	next();

	//! \brief This will lock the player
	void	lock();

	//! \brief This will unlock the player
	void	unlock();

private:
	/*!
	 * \brief This is the process ID of the MP3 player.
	 *
	 * If there is no player process, this will be -1.
	 *
	 */
	int pid;

	//! \brief This is the status of the player
	int status;
	
	/*!
	 * \brief This is the track number that is currently playing.
	 *
	 * If there is no such track, this will be -1.
	 *
	 */
	int trackid;

	/*!
	 * \brief This is the number of the queue item that is currently playing.
	 *
	 * If there is no such item, this will be -1.
	 *
	 */
	int playid;

	/*!
	 * \brief Non-zero if the player is locked, zero if it is not.
	 *
	 * If the player is locked, the current track cannot be touched by anyone but
	 * an administrator. Only admins can lock/unlock the player.
	 *
	 */
	int locked;
};

#endif // __PLAYER_H__

/* vim:set ts=2 sw=2: */
