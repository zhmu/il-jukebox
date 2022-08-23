/*
 * main.cc - Jukebox Main Code
 *
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <unistd.h>
#include <time.h>
#include <libplusplus/database.h>
#include <libplusplus/network.h>
#include <libplusplus/log.h>
#include "config.h"
#include "jukebox.h"
#include "player.h"
#include "queue.h"
#include "server.h"
#include "user_sql.h"
#include "user_ldap.h"
#include "volume.h"

char* configfile = CONFIG_FILENAME;

JUKECONFIG* config;
NETWORK* net;
LOG* logger;
JUKESERVER* server;
QUEUE* queue;
DATABASE* db;
PLAYER* player;
USERS* users;
VOLUME* volume;

int quit = 0;

void sigchld_handler(int i) { player->launch(); } 
void sigint_handler(int i) { quit = 1; }

void sighup_handler(int i) {
	JUKECONFIG* newconfig;

	// load the configuration
	newconfig = new JUKECONFIG();
	if (newconfig->load (configfile) != CONFIGFILE_OK) {
		// this failed. complain
		delete newconfig;
		logger->log (LOG_CRIT, "Unable to reload configuration file '%s'", configfile);
		return;
	}

	// wonderful, this worked. ditch the old one and use this one
	delete config;
	config = newconfig;
	logger->log (LOG_INFO, "Configuration file successfully reloaded");
}

/*
 * loadBackends()
 *
 * This will attempt to laod the chain of backends. It will return the first
 * backend loaded on success or NULL on failure.
 *
 */
USERS*
loadBackends() {
	char* str = NULL;
	char* ptr;
	USERS* tmpUsers;
	USERS* browseUser;
	USERS* users = NULL;

	// fetch the type from the config file
	if (config->get_string ("userdb", "backends", &str) != CONFIGFILE_OK) {
		// no one is specified. bail out
		logger->log (LOG_CRIT, "getUserBackend(): no backends specified in configuration file");
		return NULL;
	}

	// wade through all backends
	while (1) {	
		// isolate the backend
		ptr = strchr (str, ',');
		if (ptr != NULL)
			// got it. isolate the next part
			*ptr++ = 0;

		// kill leading spaces
		while (*str == ' ') str++;

		// load the backend
		tmpUsers = NULL;
#ifdef USERDB_SQL
		if (!strcasecmp (str, "sql"))
			tmpUsers = new USERS_SQL();
#endif /* USERDB_SQL */
#ifdef USERDB_LDAP
		if (!strcasecmp (str, "ldap"))
			tmpUsers = new USERS_LDAP();
#endif /* USERDB_LDAP */
		if (tmpUsers != NULL) {
			// initalize the backend
			if (tmpUsers->init()) {
				// victory. do we have a list?
				if (users == NULL)
					// no. now, we do
					users = tmpUsers;
				else {
					// yes. wade to the end of the list
					browseUser = users;
					while (browseUser->getNextDB() != NULL)
						browseUser = browseUser->getNextDB();

					// append this one to the list
					browseUser->setNextDB (tmpUsers);
				}
			} else
				// this failed. complain
				logger->log (LOG_INFO, "cannot initialize backend '%s', skipping", str);
		} else
			// undefined type! complain
			logger->log (LOG_INFO, "unknown backend type '%s', skipping", str);

		// next pointer?
		if (ptr == NULL)
			// no. break out of here
			break;

		// skip spaces and go
		while (*ptr == ' ') ptr++;
		str = ptr;
	}

	// return the object
	return users;
}

/*
 * This will display a brief usuage.
 */
void
usuage() {
	fprintf (stderr, "usuage: jukebox [-d] [-c filename]\n\n");
	fprintf (stderr, "        -d            Daemonize after startup\n");
	fprintf (stderr, "        -c filename   Specify configuration filename\n");
	fprintf (stderr, "        -h, -?        This help\n");
}

/*
 * main (int argc, char** argv)
 *
 * This is the main code.
 *
 */
int
main (int argc, char** argv) {
	int ch;
	int dflag = 0;
	USERS* tmpUsers;
	char* logtype = NULL;

	#ifdef OS_FREEBSD
		// seed the random generator (FreeBSD way)
		srandomdev();
	#else
		// seed the random generator
		srand ((int)time((time_t*)NULL));
	#endif

	// parse all parameters
	while ((ch = getopt (argc, argv, "c:dh?")) != -1) {
		switch (ch) {
			case 'd': // daemonize
								dflag++;
								break;
			case 'c': // config file
								configfile = optarg;
								break;
			case '?':
			case 'h': // help
								usuage();
								exit (EXIT_FAILURE);
		}
	}

	// load the configuration
	config = new JUKECONFIG();
	if (config->load (configfile) != CONFIGFILE_OK) {
		// this failed. complain
		fprintf (stderr, "JUKECONFIG::load(): unable to load configuration file '%s'\n", CONFIG_FILENAME);
		return EXIT_FAILURE;
	}

	// initialize the logger
	if (config->get_string ("log", "type", &logtype) != CONFIGFILE_OK)
		logtype = "stderr";
	logger = LOG::getLog (logtype, "jukebox");
	if (logger == NULL) {
		fprintf (stderr, "LOG::getLog(): can't setup log type '%s'\n", logtype);
		return EXIT_FAILURE;
	}

	// initialize the database connection
	db = config->getDatabase();
	if (db == NULL) {
		// this failed. complain
		fprintf (stderr, "database type unsupported by lib++\n");
		return EXIT_FAILURE;
	}

	// connect to the database server
	if (!db->connect (config->dbHostname,
										config->dbUsername,
										config->dbPassword,
	                  config->dbDatabase)) {
		// this failed. too bad, so sad
		logger->log (LOG_CRIT, "Unable to open database connection: %s", db->getErrorMsg());
		return EXIT_FAILURE;
	}

	// do we have to chroot?
	if (config->chroot != NULL) {
		// yes. do it
		if (chroot (config->chroot) < 0) {
			// this failed. complain
			logger->log (LOG_CRIT, "Unable to change root dir");
			return EXIT_FAILURE;
		}

		// change the directory to /
		chdir ("/");
	}

	// got a group to run as?
	if (config->destGroup != NULL) {
		// yes. make the switch
		if (setregid (config->gid, config->gid) < 0) {
			// this failed. bail out
			logger->log (LOG_CRIT, "Unable to switch group");
			return EXIT_FAILURE;
		}
	}

	// got an user to run as?
	if (config->destUser != NULL) {
		// yes. make the switch
		if (setreuid (config->uid, config->uid) < 0) {
			// this failed. bail out
			logger->log (LOG_CRIT, "Unable to switch user");
			return EXIT_FAILURE;
		}
	}

	// create the queue
	queue = new QUEUE();

	// create the network
	net = new NETWORK();
	
	// create the player
	player = new PLAYER();

	// create the backends
	users = loadBackends();
	if (users == NULL) {
		// this failed. complain
		logger->log (LOG_CRIT, "loadBackends(): no backends loaded, exiting");
		return 1;
	}

	// create the server
	server = new JUKESERVER();
	if (!server->create (config->port)) {
		// this failed. complain
		logger->log (LOG_CRIT, "Unable to bind server to %u/TCP, exiting", config->port);
		return 1;
	}

	// initialize the volume manager
	volume = new VOLUME();
	if (!volume->init())
		// this failed. notify the user (but don't quit)
		logger->log (LOG_INFO, "volume manager failed to initialize, disabling volume management");

	// attach the server to the network
	net->addService (server);

#ifndef OS_SOLARIS
	// need to daemonize ourselves?
	if (dflag)
		// yes. demons, beware!
		if (daemon (0, 0) < 0)
			// this failed. complain
			logger->log (LOG_INFO, "Unable to daemonize, running on foreground\n");
#endif /* OS_SOLARIS */

	// hook hangup, child, interrupt and terminate signals to us
	signal (SIGHUP, sighup_handler);
	signal (SIGCHLD, sigchld_handler);
	signal (SIGINT, sigint_handler);
	signal (SIGTERM, sigint_handler);

	// go!
	player->play();

	// handle the network
	logger->log (LOG_INFO, "Jukebox doing main loop");
	while (!quit)
		net->run();

	// bye!
	logger->log (LOG_INFO, "Jukebox exiting");

	// remove all objects
	delete volume;
	delete player;
	delete server;
	delete net;	
	delete queue;
	while (users) {
		tmpUsers = users->getNextDB();
		delete users;
		users = tmpUsers;
	}
	delete users;
	delete db;
	delete logger;

	// all done
	return 0;
}

/* vim:set ts=2 sw=2: */
