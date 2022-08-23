/*
 * jukectl.cc - Jukebox Control Utility
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libplusplus/network.h>
#include "config.h"
#include "jukectl.h"

NETWORK* net;
JUKECTLCLIENT* client;
IPV4ADDRESS* netaddr;
CONFIGFILE* config;

int verbose = 0;
int status = 0;
int error = 0;

// JUKECTL_DEFAULT_CONFIGFILE is the default configuration file
#define JUKECTL_DEFAULT_CONFIGFILE "/.jukectl.conf"

/*
 * JUKECTLCLIENT::incoming()
 *
 * This will be called whenever there is incoming data.
 *
 */
void
JUKECTLCLIENT::incoming() {
	char temp[512];
	char* ptr;
	char* nextPtr;

	// grab the data
	memset (temp, 0, sizeof (temp));
	int len = recv (temp, sizeof (temp) - 1);
	if (!len)
		return;

	// terminate the string
	temp[len] = 0;

	// handle the data as separate strings
	ptr = temp;
	while (*ptr) {
		// scan for the next string
		nextPtr = strchr (ptr, '\n');
		if (nextPtr != NULL) {
			// remove the newline
			*nextPtr = 0; nextPtr++;
		}

		if ((ptr[1] != 'I') || (verbose))
			printf ("%s\n", ptr);

		// error ?
		if (ptr[1] == 'E')
			// yes. too bad
			error++;

		// if we have no nextPtr, leave
		if (nextPtr == NULL)
			return;

		// if we have a 0 just after nextPtr, leave
		if (!*nextPtr)
			return;

		// next
		ptr = nextPtr;
	}
}

/*
 * usuage()
 *
 * This will print information about the usuage of this utility.
 *
 */
void
usuage() {
	fprintf (stderr, "usuage: jukectl [-v] [-c filename] command\n");
	fprintf (stderr, "        -c filename   Specify configuration file\n");
	fprintf (stderr, "                       (Defaults to $HOME/.jukectl.conf)\n");
	fprintf (stderr, "        -v            Increase verbosity (repeat for more)\n");
	fprintf (stderr, "        -q            Quiet mode: don't print progress\n");
	fprintf (stderr, "        -h            Print this help message\n");
	fprintf (stderr, "        command is one of the jukeclient commands\n");
}

/*
 * main (int argc, char** argv)
 *
 * This is the main code.
 *
 */
int
main (int argc, char** argv, char** env) {
	char* hostname = NULL;
	char* username = NULL;
	char* password = NULL;
	char* tmp;
	int	  portno = -1;
	int	  use_ident;
	int		login_ok;
	char ch;
	char* home = getenv("HOME");
	char* defcfile = (char*)malloc(strlen(home)+strlen(JUKECTL_DEFAULT_CONFIGFILE)+1);
	char* cfile = defcfile;

	strcpy(cfile, home);
	strcat(cfile, JUKECTL_DEFAULT_CONFIGFILE);
	int		cmd_begin;

	// handle command line parameters
	while ((ch = getopt (argc, argv, "c:vh?")) != -1) {
		switch (ch) {
			case 'c': // configuration file
			          cfile = optarg;
				        break;
			case 'v': // increate verbosity
								verbose++;
								break;
			case 'h':
			case '?': // help
			 default: // unknown option
								usuage();
								exit (EXIT_FAILURE);
		}
	}

	// set the command parameter
	cmd_begin = optind;
	if ((argc - 1) != cmd_begin) {
		// no command given. complain
		usuage();
		exit (EXIT_FAILURE);
	}

	// try to open the configuration file
	config = new CONFIGFILE();
	if (config->load (cfile) != CONFIGFILE_OK) {
		// this failed. complain
		fprintf (stderr, "jukectl: unable to load configuration file '%s'\n", cfile);
		return EXIT_FAILURE;
	}

	// don't need this any more
	free(defcfile);

	// fetch the hostname, username, password and port number
	config->get_string ("general", "hostname", &hostname);
	config->get_string ("general", "username", &username);
	config->get_string ("general", "password", &password);
	config->get_value  ("general", "port",     &portno);

	// fetch the status of ident authentication
	use_ident = 0;
	if (config->get_string ("general", "use_ident", &tmp) == CONFIGFILE_OK) {
		// check whether ident authenication is to be used
		if ((!strcasecmp (tmp, "yes")) || (!strcasecmp (tmp, "true")) ||
		    (!strcasecmp (tmp, "on")))
			// yes. set the flag
			use_ident = 1;
	}

	// ensure we have a configuration
	if (hostname == NULL) {
		fprintf (stderr, "jukectl: no hostname specified in configuration file\n");
		return EXIT_FAILURE;
	}
	if (username == NULL) {
		fprintf (stderr, "jukectl: no username specified in configuration file\n");
		return EXIT_FAILURE;
	}
	if (portno < 0) {
		fprintf (stderr, "jukectl: no port specified in configuration file\n");
		return EXIT_FAILURE;
	}

	// create the network
	net = new NETWORK();

	// create the host
	netaddr = new IPV4ADDRESS();

	// resolve the host
	if (!netaddr->setAddr (hostname)) {
		// this failed. complain
		printf ("jukectl: cannot resolve '%s'\n", hostname);
		return EXIT_FAILURE;
	}

	// fill the port number out
	netaddr->setPort (portno);

	// build a new client
	client = new JUKECTLCLIENT();

	// try to connect
	if (!client->connect (netaddr)) {
		// this failed. complain
		printf ("jukectl: unable to connect to %s:%u\n", hostname, portno);
		return EXIT_FAILURE;
	}

	// attach the client to the network
	net->addService (client);

	// fetch the welcome message
	net->run();

	// send the username
	client->sendf ("USER %s\r\n", username);
	if (verbose)
		printf (">> USER %s\n", username);
	net->run();
	if (error) {
		delete netaddr; delete net;
		printf ("jukectl: username refused\n");
		return EXIT_FAILURE;
	}

	// need to use ident?
	login_ok = 0;
	if (use_ident) {
		// yes. try that
		client->sendf ("IDENT\r\n");
		net->run();
		if (!error)
			// this worked!
			login_ok = 1;
		error = 0;
	}

	// did IDENT work ?
	if (!login_ok) {
		// no. do we even have a password?
		if (password == NULL)
			// no. query for it
			password = getpass("jukebox password: ");

		// send the password
		client->sendf ("PASSWORD %s\r\n", password);
		if (verbose)
			printf (">> PASSWORD (xxx)\n");
		net->run();
		if (error) {
			delete netaddr; delete net;
			printf ("jukectl: password refused\n");
			return EXIT_FAILURE;
		}
	}

	// send the actual command
	client->sendf ("%s\r\n", argv[cmd_begin]);
	if (verbose)
		printf (">> %s\n", argv[cmd_begin]);

	// ensure we exit after this
	client->sendf ("BYE\r\n");
	if (verbose)
		printf (">> BYE\n");

	// handle the network
	while (client->isActive())
		net->run();

	// remove all objects
	delete netaddr;
	delete net;

	return 0;
}

/* vim:set ts=2 sw=2: */
