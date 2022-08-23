/*
 * ident.cc - Identification Protocol client, as per RFC1413
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "ident.h"
#include "jukebox.h"

/*
 * IDENT::incoming()
 *
 * This will be called whenever there is incoming data.
 *
 */
void
IDENT::incoming() {
	char temp[128];

	// grab the data
	int len = recv (temp, sizeof (temp) - 1);
	if (!len)
		return;

	// terminate the string
	temp[len] = 0;

	// display it
	printf (temp);
}

/*
 * IDENT::identify (NETADDRESS* dest, char* user)
 *
 * This will try to identify user [user] using ident server [dest]. It will
 * return zero on failure or non-zero on success.
 *
 */
int
IDENT::identify (NETADDRESS* dest, char* user) {
	char buf[128];
	char *ptr, *ptr2;
	int len;
	IPV4ADDRESS* icon = new IPV4ADDRESS();

	// try to connect
	memcpy (icon->getInternalAddress(), dest->getInternalAddress(), dest->getInternalLength());
	icon->setPort (113);
	if (!connect (icon)) {
		// this failed. complain
		delete icon;
		return 0;
	}

	// address is no longer needed
	delete icon;

	// send the query
	sendf ("%u, %u\n", dest->getPort(), config->getPort());

	// fetch the reply
	len = recv (buf, sizeof (buf) - 1);
	buf[len] = 0;

	// close the socket (we don't need it anymore)
	close();

	// destroy any trailing CR/LF's
	while ((ptr = strrchr (buf, '\r')) != NULL) *ptr = 0;
	while ((ptr = strrchr (buf, '\n')) != NULL) *ptr = 0;

	// the line should be in the format:
	// clientport, serverport : USERID : UNIX : username
	//
	// chop off the port numbers
	ptr = strchr (buf, ':');
	if (ptr == NULL)
		// humm, no colon. this is a corrupt packet, fail
		return 0;
	ptr++;

	// isolate the next colon
	ptr2 = strchr (ptr, ':');
	if (ptr2 == NULL)
		// humm, no colon. this is a corrupt packet, fail
		return 0;
	*ptr2 = 0; ptr2++;

	// skip preceding spaces
	while (*ptr == ' ') ptr++;

	// ptr2 should now be USERID, correct ?
	if (strncasecmp (ptr, "USERID", 6))
		// no. invalid reply, ditch it
		return 0;

	// isolate another chunk (it usually reads UNIX, but we skip it)
	ptr = strchr (ptr2, ':');
	if (ptr == NULL)
		// humm, no colon. this is a corrupt packet, fail
		return 0;
	ptr++;

	// the final text is all we care about now. skip trailing spaces
	while (*ptr == ' ') ptr++;

	// it's all up to the match now
	return (!strcasecmp (ptr, user)) ? 1 : 0;
}

/* vim:set ts=2 sw=2: */
