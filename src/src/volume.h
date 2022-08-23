/*
 * volume.h
 *
 * This is the jukebox file player.
 *
 */
#include <stdlib.h>

#ifndef __VOLUME_H__
#define __VOLUME_H__

//! \brief The number of steps a VOLUP or VOLDN increases the volume
#define VOLUME_STEPSIZE	10

/*! \class VOLUME
 *  \brief Handles volume modifications
 */
class VOLUME {
public:
	//! \brief Creates a new object
	VOLUME();

	//! \brief Cleanly removes the oject
	~VOLUME();

	/*! \brief Initializes the volume manager
	 *
	 *  This will return zero on failure or non-zero on success.
	 */
	int init();

	//! \brief Returns whether the volume manager is available
	inline int isAvailable() { return (fd == -1) ? 0 : 1; }

	//! \brief Retrieves the current volume
	inline int getVolume() { return curvol; }

	/*! \brief Changes the current volume
	 *  \param vol The new volume
	 *
	 *  This function will do nothing if the volume isn't
	 *  properly initialized.
	 */
	void setVolume (int vol);

private:
	//! \brief File descriptor of the volume device, or -1 if none
	int fd;

	//! \brief The source to change the volume for
	int source;

	//! \brief The current volume
	int curvol;
};

#endif /* __VOLUME_H__ */

/* vim:set ts=2 sw=2: */
