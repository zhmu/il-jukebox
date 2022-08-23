/*
 * config.h
 *
 * This is the jukebox configuration handler.
 *
 */
#include <stdlib.h>
#include <libplusplus/configfile.h>
#include <libplusplus/database.h>
#include <libplusplus/network.h>
#include "user.h"

#ifndef __JUKECONFIG_H__
#define __JUKECONFIG_H__

//! \brief The value to use if a specific privilege is not known
#define JUKECONFIG_PRIV_DEFAULTKEY	"*default*"

/*!
 * \class JUKECONFIG
 * \brief Manages the jukebox configuration
 */
class JUKECONFIG : public CONFIGFILE {
public:
	int   port;
	char* dbHostname;
	char* dbDatabase;
	char* dbUsername;
	char* dbPassword;

	char* destUser;
	char* destGroup;
	char* chroot;

	int	uid, gid, logenqueue, logremove, identallowed, anonstatusallowed;

	/*! \brief Looks up a player for the supplied extension
	 *
	 * This function will return zero on failure or non-zero on success.
	 *
	 * \param ext The extension to look up, like 'ogg'
	 * \param dest Pointer to a buffer where to put the result
	 *
	 */
	int	 lookupPlayer (char* ext, char** dest);

	/*! \brief Checks whether the user possesses a right
	 *
	 * This function will return zero on failure or non-zero on success.
	 *
   * \param cmd The command to check for
	 * \param status The user's status
	 *
	 */
	int checkRight (char* cmd, int status);

	/*! \brief Returns a database object as specified in the config file
	 *
	 * If the database type is unsupported, NULL will be returned.
	 */
	DATABASE* getDatabase();

	//! \brief Returns whether enqueues will be logged
	inline int getLogEnqueue() { return logenqueue; }

	//! \brief Returns whether removes will be logged
	inline int getLogRemove() { return logremove; }

	//! \brief Returns the port to which the daemon is bound
	inline int getPort() { return port; }

	//! \brief Returns whether Identification Protocol authentication is allowed
	inline int isIdentAllowed() { return identallowed; }

	//! \brief Returns whether Status Requests for anonymous users are allowed
	inline int isAnonStatusAllowed() { return anonstatusallowed; }

	/*! \brief Checks whether IDENT authentication is allowed from a host
	 *	\return Non-zero if it is allowed, zero if not
	 *  \param addr The address to check
	 */
	int checkIdentHost (NETADDRESS* addr);

private:
	//! \brief Parses the configuration file
	void	parse();
};

#endif /* __JUKECONFIG_H__ */

/* vim:set ts=2 sw=2: */
