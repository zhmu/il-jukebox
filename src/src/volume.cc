/*
 * volume.cc - Jukebox volume manager code
 *
 */
#include <sys/types.h>
#ifndef OS_SOLARIS
#include <sys/soundcard.h>
#endif /* OS_SOLARIS */
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "jukebox.h"
#include "volume.h"

#ifndef OS_SOLARIS

/*
 * VOLUME::VOLUME()
 *
 * This will create a plain new object.
 *
 */
VOLUME::VOLUME() {
	// no volume file descriptor yet, neither a mixer source
	fd = -1; source = -1;
}

/*
 * VOLUME::~VOLUME()
 *
 * This will deinitialize the volume manager.
 *
 */
VOLUME::~VOLUME() {
	// got a volume file descriptor?
	if (fd != -1)
		// yes. close it
		close (fd);
}

/*
 * VOLUME::init()
 *
 * This will initialize the volume manager. It will return zero on failure or
 * non-zero on success.
 *
 */
int
VOLUME::init() {
	char* tmp = "pcm";
	char* device;

	// fetch the mixer device from the config file
	if (config->get_string ("mixer", "device", &device) != CONFIGFILE_OK) {
		// this failed. complain
		logger->log (LOG_INFO, "VOLUME::init(): no mixer device specified in config file");
		return 0;
	}

	// valid recording source specified ?
	config->get_string ("mixer", "source", &tmp);
	if (!strcasecmp (tmp, "pcm")) {
		// pcm
		source = SOUND_MIXER_PCM;
	} else if (!strcasecmp (tmp, "master")) {
		// master
		source = SOUND_MIXER_VOLUME;
	} else {
		// this failed. complain
		logger->log (LOG_INFO, "VOLUME::init(): invalid mixer source specified in config file, only pcm and master supported");
		return 0;
	}

	// try to open this wonderful device
	if ((fd = open (device, O_RDWR)) < 0) {
		// this failed. complain
		logger->log (LOG_INFO, "VOLUME::init(): unable to open mixer device");
		return 0;
	}

	// fetch the current volume
	if (ioctl (fd, MIXER_READ (source), &curvol) < 0) {
		// this failed. complain
		logger->log (LOG_INFO, "VOLUME::init(): unable to query volume");
		close (fd); fd = -1;
		return 0;
	}

	// merge the high/low values (not quite, but this is good enough)
	curvol = curvol & 0xff;

	// victory
	return 1;
}

/*
 * VOLUME::setVolume (int vol)
 *
 * This will set the volume to [vol].
 *
 */
void
VOLUME::setVolume (int vol) {
	unsigned int i;

	// are we properly initialized
	if (fd == -1)
		// no. bail out
		return;

	// write the volume
	i = (vol << 8) | vol;
	if (ioctl (fd, MIXER_WRITE (source), &i) < 0)
		// this failed. bail out
		return;

	// activate the new volume value
	curvol = vol;
}

#else

/* dummy functions if volume management is unsupported */

VOLUME::VOLUME() { }
VOLUME::~VOLUME() { }

int VOLUME::init() {
	// this failed. complain
	logger->log (LOG_INFO, "VOLUME::init(): not yet supported on this architecture");
	return 0;
}

void VOLUME::setVolume (int vol) { }

#endif /* OS_SOLARIS */

/* vim:set ts=2 sw=2: */
