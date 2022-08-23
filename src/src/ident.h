/*
 * ident.h
 *
 * This will handle the RFC 1413 identification protocol.
 *
 */
#include <stdlib.h>
#include <libplusplus/network.h>

#ifndef __IDENTCLIENT_H__
#define __IDENTCLIENT_H__

class IDENT : public NETCLIENT {
public:
	int identify (NETADDRESS* dest, char* user);
	void incoming();
};

#endif // __IDENTCLIENT_H__

/* vim:set ts=2 sw=2: */
