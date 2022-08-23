/*
 * jukectl.h
 *
 * This is the jukebox control client.
 *
 */
#include <stdlib.h>
#include <libplusplus/network.h>

#ifndef __JUKECTL_H__
#define __JUKECTL_H__

class JUKECTLCLIENT : public NETCLIENT {
public:
	void incoming();
};

#endif // __JUKECTL_H__

/* vim:set ts=2 sw=2: */
