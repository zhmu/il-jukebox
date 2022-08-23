/*
 * config.cc - Jukebox configuration file code
 *
 */
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libplusplus/database.h>
#include "config.h"
#include "jukebox.h"
#include "user_sql.h"
#include "user_ldap.h"

/*
 * JUKECONFIG::parse()
 *
 * This will parse the configuration file.
 *
 */
void
JUKECONFIG::parse() {
	struct passwd* pwent;
	struct group* grent;
	char* tmp;

	// defaults first
	port = CONFIG_PORT; dbDatabase = dbUsername = dbPassword = dbHostname = NULL;
	destUser = NULL; destGroup = NULL; chroot = NULL; uid = gid = 0;

	// try to read the port
	if (get_value ("general", "port", &port) == CONFIGFILE_ERROR_INVALIDVALUE) {
		// this failed. complain and exit
		fprintf (stderr, "JUKECONFIG::parse(): non-numeric ports are invalid\n");
		exit (EXIT_FAILURE);
	}

	// fetch the database configuration
	get_string ("database", "dbname", &dbDatabase);
	get_string ("database", "username", &dbUsername);
	get_string ("database", "password", &dbPassword);
	get_string ("database", "hostname", &dbHostname);

	// fetch possible setuid/setgid and chroot information
	get_string ("general", "user", &destUser);
	get_string ("general", "group", &destGroup);
	get_string ("general", "chroot", &chroot);

	// got an user?
	if (destUser != NULL) {
		// yes. look him up
	 	pwent = getpwnam (destUser);
		if (pwent == NULL) {
			// no such user. complain
			fprintf (stderr, "JUKECONFIG::parse(): user '%s' does not exist\n", destUser);
			exit (EXIT_FAILURE);
		}

		// store the uid
		uid = pwent->pw_uid;
	}

	// got a group?
	if (destGroup != NULL) {
		// yes. look him up
	 	grent = getgrnam (destGroup);
		if (grent == NULL) {
			// no such group complain
			fprintf (stderr, "JUKECONFIG::parse(): group '%s' does not exist\n", destGroup);
			exit (EXIT_FAILURE);
		}

		// store the gid
		gid = grent->gr_gid;
	}

	// got all we need to know?
	if ((dbDatabase == NULL) || (dbUsername == NULL) || (dbPassword == NULL) ||
			(dbHostname == NULL)) {
		// no. complain
		fprintf (stderr, "JUKECONFIG::parse(): database information missing\n");
		exit (EXIT_FAILURE);
	}

	// fetch the status of enqueue logging
	logenqueue = 0;
	if (get_string ("log", "enqueue", &tmp) == CONFIGFILE_OK) {
		// this worked. is it turned on?
		if ((!strcasecmp (tmp, "yes")) || (!strcasecmp (tmp, "true")) ||
		    (!strcasecmp (tmp, "on")))
			// yes. set the flag
			logenqueue = 1;
	}

	// fetch the status of remove logging
	logremove = 0;
	if (get_string ("log", "remove", &tmp) == CONFIGFILE_OK) {
		// this worked. is it turned on?
		if ((!strcasecmp (tmp, "yes")) || (!strcasecmp (tmp, "true")) ||
		    (!strcasecmp (tmp, "on")))
			// yes. set the flag
			logremove = 1;
	}

	// fetch the status of ident identification 
	identallowed = 0;
	if (get_string ("general", "allow_ident", &tmp) == CONFIGFILE_OK) {
		// this worked. is it turned on?
		if ((!strcasecmp (tmp, "yes")) || (!strcasecmp (tmp, "true")) ||
		    (!strcasecmp (tmp, "on")))
			// yes. set the flag
			identallowed = 1;
	}

	// fetch the status of anonymous status
	anonstatusallowed = 0;
	if (get_string ("general", "allow_anonymous_status", &tmp) == CONFIGFILE_OK) {
		// this worked. is it turned on?
		if ((!strcasecmp (tmp, "yes")) || (!strcasecmp (tmp, "true")) ||
		    (!strcasecmp (tmp, "on")))
			// yes. set the flag
			anonstatusallowed = 1;
	}
}

/*
 * JUKECONFIG::lookupPlayer (char* ext, char** dest)
 *
 * This will look up the executable to play a file with extension [ext] with.
 * It will place a pointer to that executable in [dest]. It will return zero
 * on failure and non-zero on success.
 *
 */
int
JUKECONFIG::lookupPlayer (char* ext, char** dest) {
	return (get_string ("player", ext, dest) == CONFIGFILE_OK) ? 1 : 0;
}

/*
 * JUKECONFIG::checkRight (char* cmd, int status)
 *
 * This will check whether the user with status [status] has enough
 * rights to do [cmd]. It will return zero if not, and non-zero on success.
 *
 */
int
JUKECONFIG::checkRight (char* cmd, int status) {
	char* str = "admin";
	int right = USER_STATUS_ADMIN;

	// try to fetch privileges->[cmd] first
	if (get_string ("privileges", cmd, &str) != CONFIGFILE_OK)
		// this failed. try privileges->JUKECONFIG_PRIV_DEFAULTKEY instead
		get_string ("privileges", JUKECONFIG_PRIV_DEFAULTKEY, &str);

	// resolve the rights
	if (!strcasecmp (str, "admin")) {
		// admin rights
		right = USER_STATUS_ADMIN;
	} else if (!strcasecmp (str, "user")) {
		// user rights
		right = USER_STATUS_USER;
	} else if (!strcasecmp (str, "anon")) {
		// anonymous rights
		right = USER_STATUS_ANON;
	} else if (!strcasecmp (str, "anonymous")) {
		// anonymous rights
		right = USER_STATUS_ANON;
	}

	// finally, check the rights
	return (right > status) ? 0 : 1;
}

/*
 * JUKECONFIG::getDatabase()
 *
 * This will return a database object as specified in the config file. If no
 * valid database is found, NULL will be returned.
 *
 */
DATABASE*
JUKECONFIG::getDatabase() {
	char* type;

	// fetch the database type
	if (get_string ("database", "type", &type) != CONFIGFILE_OK) {
		// this failed. complain
		fprintf (stderr, "JUKECONFIG::getDatabase(): no type specified in the configuration file\n");
		return NULL;
	}

	return DATABASE::getDatabase (type);
}

/*
 * JUKECONFIG::checkIdentHost (NETADDRESS* addr)
 *
 * This will check whether ident authentication is allowed from [addr]. It
 * will return non-zero if it is and zero if not.
 *
 */
int
JUKECONFIG::checkIdentHost (NETADDRESS* addr) {
	char* ident;
	char* ptr;

	// fetch the values from the config file
	if (get_string ("general", "allow_ident_from", &ident) != CONFIGFILE_OK)
		// this failed. it's allowed then
		return 1; 

	// walk through the values
	while (*ident) {
		// scan for a space
		ptr = strchr (ident, ' ');
		if (ptr == NULL)
			// not found. this must be the final entry, so scan for the end
			ptr = strchr (ident, 0);
		else {
			// isolate the string and move to the next
			*ptr = 0; ptr++;
			while (*ptr == ' ') ptr++;
		}

		// check this host for a match
		if (addr->compareAddr (ident))
			// got it
			return 1;

		// next
		ident = ptr;
	}

	// no matches
	return 0;
}

/* vim:set ts=2 sw=2: */
