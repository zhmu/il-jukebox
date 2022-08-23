/*
 * scan.cc - Jukebox Queue Scanner
 *
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#ifdef MP3_SUPPORT
#include <id3.h>
#include <id3/tag.h>
#include <id3/misc_support.h>
#endif /* MP3_SUPPORT */
#include <libplusplus/database.h>
#include <libplusplus/log.h>
#include "artist.h"
#include "album.h"
#include "config.h"
#include "jukebox.h"
#include "player.h"
#include "track.h"
#include "vcedit.h"

JUKECONFIG* config;
LOG* logger;
JUKESERVER* server;
DATABASE* db;

char twirl[] = "/-\\|/-\\|";
int tpos = 0;
int verbose = 0, quiet = 0;
int numUpdated = 0, numNew = 0, skip = 0;

int tq_title = 1;
int tq_artist = 1;
int tq_album = 1;
int tq_year = 1;
int tq_trackno = 1;
int tq_required = 0;

char* module_artist = "<MODULES>";
char* module_album = "<MODULES>";
char* adlib_artist = "<MODULES>";
char* adlib_album = "<ADLIB>";
char* sid_artist = "<SID>";
char* sid_album = "<SID>";

/*
 * getArtist (char* name)
 *
 * This will return the ID of artist [name]. The artist will be created if
 * needed.
 *
 */
int
getArtist (char* name) {
	ARTIST* a;
	int id;

	try {
		// try to fetch the artist
		a = new ARTIST (name);
		id = a->getID();
		delete a;
		return id;
	} catch (ArtistException e) {
		// no such artist. create one
		a = new ARTIST();
		a->setName (name);
		a->update();
		id = a->getID();
		delete a;

		// be verbose if needed
		if (verbose > 1)
			printf ("New artist '%s'[%u]\n", name, id);
		return id;
	}
}

/*
 * getAlbum (char* name, int artistid)
 *
 * This will return the ID of album [name] and artist [artistid]. The album
 * will be created if needed.
 *
 */
int
getAlbum (char* name, int artistid) {
	ALBUM* a;
	int id;

	try {
		// try to fetch the artist
		a = new ALBUM (name, artistid);
		id = a->getID();
		delete a;
		return id;
	} catch (AlbumException e) {
		// no such artist. create one
		a = new ALBUM();
		a->setName (name);
		a->setArtistID (artistid);
		a->update();
		id = a->getID();
		delete a;

		// be verbose if needed
		if (verbose > 1)
			printf ("New album '%s'[%u]\n", name, id);
		return id;
	}
}

/*
 * addTrack (char* fname, char* title, char* artist, char* album, int year,
 *           int demo)
 *
 * This will add the track to the database. If [demo] is non-zero, changes will
 * not actually be committed.
 *
 */
void
addTrack (char* fname, char* title, char* artist, char* album, int year, int trackno, int demo) {
	TRACK* t;

	// demo mode?
	if (demo) {
		// yes. try to fetch the track
		try {
			t = new TRACK (fname);
			delete t;

			// be verbose if needed
			if (verbose > 2)
				printf ("Would update track [%s]: title '%s', artist '%s', album '%s', year %u trackno %u\n", fname, title, artist, album, year, trackno);
		} catch (TrackException e) {
			// be verbose if needed
			if (verbose > 2)
				printf ("Would add track [%s]: title '%s', artist '%s', album '%s', year %u trackno %u\n", fname, title, artist, album, year, trackno);
		}

		// later
		return;
	}

	int artistid = getArtist (artist);
	int albumid = getAlbum (album, artistid);
	try {
		t = new TRACK (fname);
		t->setTitle (title);
		t->setArtistID (artistid);
		t->setAlbumID (albumid);
		t->setYear (year);
		t->setTrackNo (trackno);
		t->update();
		delete t;
		numUpdated++;

		// be verbose if needed
		if (verbose > 2)
			printf ("Updated track [%s]: title '%s', artist '%s'[%u], album '%s'[%u], year %u, trackno %u\n", fname, title, artist, artistid, album, albumid, year, trackno);
	} catch (TrackException e) {
		t = new TRACK();
		t->setFilename (fname);
		t->setTitle (title);
		t->setArtistID (artistid);
		t->setAlbumID (albumid);
		t->setYear (year);
		t->setTrackNo (trackno);
		t->update();
		delete t;
		numNew++;

		// be verbose if needed
		if (verbose > 1)
			printf ("New track [%s]: title '%s', artist '%s'[%u], album '%s'[%u], year %u, trackno %u\n", fname, title, artist, artistid, album, albumid, year, trackno);
	} 
}

#ifdef MP3_SUPPORT
/*
 * scanFile_MP3 (char* file, int demo)
 *
 * This will scan file [file] for ID3 tags and add it. If [demo] is non-zero,
 * changes will not actually be committed.
 *
 */
void
scanFile_MP3 (char* file, int demo) {
	char title[TRACK_MAX_TITLE_LEN];
	char artist[ARTIST_MAX_LEN];
	char album[ALBUM_MAX_LEN];
	char tmp[64];
	int year = 0;
	int tracknum = 0;
	ID3_Frame* f;
	int tq = 0;

	ID3_Tag tag;
	tag.Link (file);

	// defaults
	strcpy (title,  "?");
	strcpy (album,  "?");
	strcpy (artist, "?");

	// fetch the information
	f = tag.Find (ID3FID_TITLE);
	if (f != NULL) {
		f->Field (ID3FN_TEXT).Get (title, sizeof (title));
		tq += tq_title;
	}
	f = tag.Find (ID3FID_ALBUM);
	if (f != NULL) {
		f->Field (ID3FN_TEXT).Get (album, sizeof (album));
		tq += tq_album;
	}
	f = tag.Find (ID3FID_LEADARTIST);
	if (f != NULL) {
		f->Field (ID3FN_TEXT).Get (artist, sizeof (artist));
		tq += tq_artist;
	}
	f = tag.Find (ID3FID_YEAR);
	if (f != NULL) {
		f->Field (ID3FN_TEXT).Get (tmp, sizeof (tmp));
		year = atoi (tmp); tq++;
		tq += tq_year;
	}
	f = tag.Find (ID3FID_TRACKNUM);
	if (f != NULL) {
		f->Field (ID3FN_TEXT).Get (tmp, sizeof (tmp));
		tracknum = atoi (tmp); tq++;
		tq += tq_trackno;
	}

	// handle empty fields, they classify as having no tag at all
	if (!strcmp (title, ""))  { strcpy (title,  "?"); tq -= tq_title;  }
	if (!strcmp (album, ""))  { strcpy (album,  "?"); tq -= tq_album;  }
	if (!strcmp (artist, "")) { strcpy (artist, "?"); tq -= tq_artist; }

	// got a good enough tag?
	if (tq < tq_required) {
		// need to be verbose?
		if (verbose) {
			// yes. display it
			printf ("File '%s' skipped due to tag quality, needed %u got %u\n", file, tq_required, tq);
			skip++;
		}
	} else {
		// add it
		addTrack (file, title, artist, album, year, tracknum, demo);
	}
}
#endif /* MP3_SUPPORT */

#ifdef OGG_SUPPORT
/*
 * scanFile_ogg (char* file, int demo)
 *
 * This will scan file [file] for ogg tags and add it. If [demo] is non-zero,
 * changes will not actually be commited.
 *
 */
void
scanFile_ogg (char* file, int demo) {
	char title[TRACK_MAX_TITLE_LEN];
	char artist[ARTIST_MAX_LEN];
	char album[ALBUM_MAX_LEN];
	int year = 0;
	int tracknum = 0;
	int tq = 0;
	FILE* f;
	vcedit_state* state;
	vorbis_comment* vc;

	// defaults
	strcpy (title,  "?");
	strcpy (album,  "?");
	strcpy (artist, "?");

	// open the file
	if ((f = fopen (file, "rb")) == NULL)
		// this failed. leave
		return;

	// initialize the ogg vorbis file
	state = vcedit_new_state();
	if (vcedit_open (state, f) < 0) {
		// this failed. leave
		fclose (f);
		return;
	}

	// grab the comments
	vc = vcedit_comments (state);

	// process them
	for (int i = 0; i < vc->comments; i++) {
		// scan for a '='
		char* ptr = strchr (vc->user_comments[i], '=');

		// got it?
		if (ptr != NULL) {
			// yes. isolate the value
			*ptr = 0; ptr++;

			// got an ARTIST tag?
			if (!strcasecmp (vc->user_comments[i], "artist")) {
				// yes. copy it over
				strncpy (artist, ptr, sizeof (artist));
				tq += tq_artist;
			}
			// got a TITLE tag?
			if (!strcasecmp (vc->user_comments[i], "title")) {
				// yes. copy it over
				strncpy (title, ptr, sizeof (title));
				tq += tq_title;
			}
			// got an ALBUM tag?
			if (!strcasecmp (vc->user_comments[i], "album")) {
				// yes. copy it over
				strncpy (album, ptr, sizeof (album));
				tq += tq_album;
			}
			// got an TRACKNUMBER tag?
			if (!strcasecmp (vc->user_comments[i], "tracknumber")) {
				// yes. copy it over
				tracknum = atoi (ptr);
				tq += tq_trackno;
			}
			// got a DATE tag?
			if (!strcasecmp (vc->user_comments[i], "date")) {
				// yes. copy it over and hope it's the year :)
				year = atoi (ptr);
				tq += tq_year;
			}
		}
	}

	// handle empty fields, they classify as having no tag at all
	if (!strcmp (title, ""))  { strcpy (title,  "?"); tq -= tq_title;  }
	if (!strcmp (album, ""))  { strcpy (album,  "?"); tq -= tq_album;  }
	if (!strcmp (artist, "")) { strcpy (artist, "?"); tq -= tq_artist; }

	// got a good enough tag?
	if (tq < tq_required) {
		// need to be verbose?
		if (verbose) {
			// yes. display it
			printf ("File '%s' skipped due to tag quality, needed %u got %u\n", file, tq_required, tq);
			skip++;
		}
	} else {
		// add it
		addTrack (file, title, artist, album, year, tracknum, demo);
	}
	
	vcedit_clear (state);
	fclose (f);
}
#endif /* OGG_SUPPORT */

/*
 * scanFile_module (char* file, int demo)
 *
 * This will scan file [file] for module tags and add it. If [demo] is non-zero,
 * changes will not actually be commited. [ext] should be the extension and is
 * used to determine the file type.
 *
 */
void
scanFile_module (char* file, char* ext, int demo) {
	char title[TRACK_MAX_TITLE_LEN];
	FILE* f;
	int offset = 0;
	int titleLength = 0;
	char* ptr = strrchr (file, '/');

	// defaults
	strcpy (title,  "?");

	// depending on the extension, set options
	if ((!strcasecmp (ext, "mod"))) {
		// .MOD files have 20 bytes title length at offset 0 */
		titleLength = 20;
	} else if (!strcasecmp (ext, "s3m")) {
		// .S3M files have 20 bytes title length at offset 0 */
		titleLength = 28;
	} else if (!strcasecmp (ext, "stm")) {
		// .STM files have 20 bytes title length at offset 0 */
		titleLength = 20;
	} else if (!strcasecmp (ext,  "it")) {
		// .IT files have 26 bytes title length at offset 4 */
		titleLength = 26; offset = 4;
	} else if (!strcasecmp (ext,  "xm")) {
		// .XM files have 20 bytes title length at offset 17 */
		titleLength = 20; offset = 17;
	} else
		// ?
		return;

	// open the file
	if ((f = fopen (file, "rb")) == NULL)
		// this failed. leave
		return;

	// seek to the correct offset */
	fseek (f, offset, SEEK_SET);

	// read the title
	memset (title, 0, TRACK_MAX_TITLE_LEN);
	if (!fread (title, titleLength, 1, f)) {
		// this failed. close the file and bail out
		fclose (f);
		return;
	}

	// got a title?
	if (!*title)
		// no. use the filename instead
		strncpy (title, ptr + 1, TRACK_MAX_TITLE_LEN - 1);

	// add it
	addTrack (file, title, module_artist, module_album, 0, 0, demo);
}

/*
 * scanFile_rad (char* file, int demo)
 *
 * This will scan file [file] for a RAD module title and add it. If [demo] is
 * non-zero, changes will not actually be commited.
 *
 */
void
scanFile_rad (char* file, int demo) {
	char title[TRACK_MAX_TITLE_LEN];
	char tmp[TRACK_MAX_TITLE_LEN];
	char* ptr = strrchr (file, '/');
	FILE* f;
	int i = 0, j;

	// defaults
	strcpy (title,  "?");

	// open the file
	if ((f = fopen (file, "rb")) == NULL)
		// this failed. leave
		return;

	// fetch the first 3 bytes
	if (!fread (tmp, 3, 1, f)) {
		// this failed. close the file and bail out
		fclose (f);
		return;
	}

	// got a RAD file?
	if ((tmp[0] != 'R') || (tmp[1] != 'A') || (tmp[2] != 'D')) {
		// no. close the file and bail out
		fclose (f);
		return;
	}

	// reset the title
	memset (title, 0, TRACK_MAX_TITLE_LEN);

	// check for a description
	fseek (f, 0x11, SEEK_SET);
	if (!fread (tmp, 1, 1, f)) {
		// this failed. close the file and bail out
		fclose (f);
		return;
	}
	if (tmp[0] & 0x80) {
		// we have a description! fetch it
		do {
			// fetch the char
			if (!fread (tmp, 1, 1, f)) {
				// this failed. close the file and bail out
				fclose (f);
				return;
			}

			// newline?
			if (tmp[0] == 1)
				// yes. stop adding the song name then
				tmp[0] = 0;
			else if ((tmp[0] >= 0x2) && (tmp[0] <= 0x1f))
				// output this many spaces
				for (j = 0; j < tmp[0]; j++)
					title[i++] = ' ';
			else if (tmp[0])
				// just append it
				title[i++] = tmp[0];
		} while (tmp[0]);
	}

	// got a title?
	if (!*title)
		// no. use the filename instead
		strncpy (title, ptr + 1, TRACK_MAX_TITLE_LEN - 1);

	// add it
	addTrack (file, title, adlib_artist, adlib_album, 0, 0, demo);
}

/*
 * scanFile_raw (char* file, int demo)
 *
 * This will just take file [file] add it. If [demo] is non-zero, changes will
 * not actually be commited.
 *
 */
void
scanFile_raw (char* file, int demo) {
	char title[TRACK_MAX_TITLE_LEN];
	char* ptr = strrchr (file, '/');

	// these file are all headerless (as far as I know at least), so just use
	// the filename for them.
	strncpy (title, ptr + 1, TRACK_MAX_TITLE_LEN - 1);

	// add it
	addTrack (file, title, adlib_artist, adlib_album, 0, 0, demo);
}

/*
 * scanFile_sid (char* file, int demo)
 *
 * This will just take SID file [file] add it. If [demo] is non-zero, changes
 * will not actually be commited.
 *
 */
void
scanFile_sid (char* file, int demo) {
	char title[TRACK_MAX_TITLE_LEN];
	char* ptr = strrchr (file, '/');

	// these file are all headerless (as far as I know at least), so just use
	// the filename for them.
	strncpy (title, ptr + 1, TRACK_MAX_TITLE_LEN - 1);

	// add it
	addTrack (file, title, sid_artist, sid_album, 0, 0, demo);
}

/*
 * scanFile_adlib (char* file, int demo)
 *
 * This will scan file [file] for module tags and add it. If [demo] is non-zero,
 * changes will not actually be commited. [ext] should be the extension and is
 * used to determine the file type.
 *
 */
void
scanFile_adlib (char* file, char* ext, int demo) {
	char title[TRACK_MAX_TITLE_LEN];
	FILE* f;
	int offset = 0;
	int titleLength = 0;
	char* ptr = strrchr (file, '/');

	// defaults
	strcpy (title,  "?");

	// depending on the extension, set options
	if ((!strcasecmp (ext, "d00"))) {
		// .D00 files have 32 bytes title length at offset 11 */
		titleLength = 32; offset = 11;
	} else if (!strcasecmp (ext, "amd")) {
		// .AMD files have 24 bytes title length at offset 0 */
		titleLength = 24;
	} else
		// ?
		return;

	// open the file
	if ((f = fopen (file, "rb")) == NULL)
		// this failed. leave
		return;

	// seek to the correct offset */
	fseek (f, offset, SEEK_SET);

	// read the title
	memset (title, 0, TRACK_MAX_TITLE_LEN);
	if (!fread (title, titleLength, 1, f)) {
		// this failed. close the file and bail out
		fclose (f);
		return;
	}

	// got a title?
	if (!*title)
		// no. use the filename instead
		strncpy (title, ptr + 1, TRACK_MAX_TITLE_LEN - 1);

	// add it
	addTrack (file, title, adlib_artist, adlib_album, 0, 0, demo);
}

/*
 * scanFile (char* file, int demo)
 *
 * This will scan file [file] for tags and add it. If [demo] is non-zero,
 * changes will not actually be commited.
 *
 */
void
scanFile (char* file, int demo) {
	char* ext = strrchr (file, '.');

	// isolate the extension
	if (ext == NULL)
		return;
	ext++;

#ifdef MP3_SUPPORT
	// mp3?
	if (!strcasecmp (ext, "mp3")) {
		// yes. do it
		scanFile_MP3 (file, demo);
		return;
	}
#endif /* MP3_SUPPORT */

#ifdef OGG_SUPPORT
	// ogg?
	if (!strcasecmp (ext, "ogg")) {
		// yes. do it
		scanFile_ogg (file, demo);
		return;
	}
#endif /* OGG_SUPPORT */

	// mod, s3m, stm, it, xm?
	if ((!strcasecmp (ext, "mod")) || (!strcasecmp (ext, "s3m")) || (!strcasecmp (ext, "stm")) ||
			(!strcasecmp (ext,  "it")) || (!strcasecmp (ext,  "xm"))) {
		// yes. do it
		scanFile_module (file, ext, demo);
		return;
	}

	// rad?
	if (!strcasecmp (ext, "rad")) {
		// yes. do it
		scanFile_rad (file, demo);
		return;
	}

	// raw, laa, lds, sci, hsc, sat, sa2?
	if ((!strcasecmp (ext, "raw")) || (!strcasecmp (ext, "laa")) || (!strcasecmp (ext, "lds")) ||
			(!strcasecmp (ext, "sci")) || (!strcasecmp (ext, "hsc")) || (!strcasecmp (ext, "sat")) ||
			(!strcasecmp (ext, "sa2")) ) {
		// yes. do it
		scanFile_raw (file, demo);
		return;
	}

	// d00, amd?
	if ((!strcasecmp (ext, "d00")) || (!strcasecmp (ext, "amd"))) {
		// yes. do it
		scanFile_adlib (file, ext, demo);
		return;
	}

	// sid?
	if (!strcasecmp (ext, "sid")) {
		// yes. process them
		scanFile_sid (file, demo);
		return;
	}
}

/*
 * scanDirectory (char* dirname, int demo)
 *
 * This will scan directory [dirname] and all its children. If [demo] is
 * non-zero, changes will not really be committed.
 *
 */
void
scanDirectory (char* dirname, int demo) {
	char tmp[PATH_MAX];
	struct dirent* dent;
	struct stat st;
	DIR* dir = opendir (dirname);

	// did this work?
	if (dir == NULL)
		// no. bail out
		return;

	// wade through the tree
	while ((dent = readdir (dir)) != NULL) {
		// skip '.' and '..'
		if ((!strcmp (dent->d_name, ".")) || (!strcmp (dent->d_name, "..")))
			continue;

		// build the file name
		snprintf (tmp, sizeof (tmp) - 1, "%s/%s", dirname, dent->d_name);

		// stat the file/dir
		if (stat (tmp, &st) < 0)
			// this failed. ignore it
			continue;

		// is it... a file ?
		if (st.st_mode & S_IFREG) {
			// yes.  be verbose if needed
			if (verbose > 3)
				printf ("Scanning [%s]\n", tmp);

			// build the name
			scanFile (tmp, demo);

			// silence?
			if (!quiet) {
				// no. show the twirlie
				printf ("%c%c", 8, twirl[tpos++]);
				tpos %= (sizeof (twirl) - 1);
				fflush (stdout);
			}
		}

		// is it, a dir ?
		if (st.st_mode & S_IFDIR) {
			// yes. scan it
			scanDirectory (tmp, demo);
		}
	}

	// a farewell to directories
	closedir (dir);
}

/*
 * usuage()
 *
 * This will display a brief usuage.
 *
 */
void
usuage() {
	fprintf (stderr, "usuage: scan [-w] [-c filename] directory ...\n\n");
	fprintf (stderr, "        -w            Wipe database before adding files\n");
	fprintf (stderr, "        -c filename   Specify configuration filename\n");
	fprintf (stderr, "        -v            Increase verbosity (repeat for more)\n");
	fprintf (stderr, "        -d            Demo mode: don't change anything\n");
	fprintf (stderr, "        -q            Quiet mode: don't print progress\n");
}

/*
 * main (int argc, char** argv)
 *
 * This is the main code.
 *
 */
int
main (int argc, char** argv) {
	int wflag = 0, demo = 0, ch;
	char* configfile = CONFIG_FILENAME;
	int dir_begin;

	// parse the parameters
	while ((ch = getopt (argc, argv, "wc:d?hvq")) != -1) {
		switch (ch) {
			case 'd': // demo mode
			          demo++;
			          break;
			case 'w': // wipe flag
			          wflag++;
			          break;
			case 'c': // config file
			          configfile = optarg;
			          break;
			case 'v': // verbosity
			          verbose++;
			          break;
			case 'q': // quietness
			          quiet++;
			          break;
			case '?':
			case 'h':
			 default: // help
								usuage();
								exit (EXIT_FAILURE);
		}
	}

	// set the first directory
	dir_begin = optind;
	if (argc == dir_begin) {
		// no such directory. complain
		usuage();
		exit (EXIT_FAILURE);
	}

	// load the configuration
	config = new JUKECONFIG();
	if (config->load (configfile) != CONFIGFILE_OK) {
		// this failed. complain
		fprintf (stderr, "JUKECONFIG::load(): unable to load configuration file '%s'\n", CONFIG_FILENAME);
		return EXIT_FAILURE;
	}

	// fetch the quality levels
	config->get_value ("tag_quality", "title", &tq_title);
	config->get_value ("tag_quality", "artist", &tq_artist);
	config->get_value ("tag_quality", "album", &tq_album);
	config->get_value ("tag_quality", "year", &tq_year);
	config->get_value ("tag_quality", "trackno", &tq_trackno);
	config->get_value ("tag_quality", "required", &tq_required);
	config->get_string ("modules", "artist", &module_artist);
	config->get_string ("modules", "album", &module_album);
	config->get_string ("adlib", "artist", &adlib_artist);
	config->get_string ("adlib", "album", &adlib_album);

	// initialize the logger
	logger = new SYSLOG("jukescan");

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

#if 0
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
		if (setgid (config->gid) < 0) {
			// this failed. bail out
			logger->log (LOG_CRIT, "Unable to switch group");
			return EXIT_FAILURE;
		}
	}

	// got an user to run as?
	if (config->destUser != NULL) {
		// yes. make the switch
		if (setuid (config->uid) < 0) {
			// this failed. bail out
			logger->log (LOG_CRIT, "Unable to switch user");
			return EXIT_FAILURE;
		}
	}
#endif

	// need to wipe the database?
	if ((wflag) && (!demo)) {
		// yes. do it
		db->execute ("DELETE FROM albums");
		db->execute ("DELETE FROM artists");
		db->execute ("DELETE FROM queue");
		db->execute ("DELETE FROM tracks");
	}

	// handle the directories
	if (!quiet)
		printf ("- Scanning %c", twirl[tpos++]);
	while (dir_begin < argc)
		scanDirectory (argv[dir_begin++], demo);

	if (!quiet)
		printf ("%c%c... done, %u tracks added, %u updated, %u skipped\n", 8, 8, numNew, numUpdated, skip);

	// remove all objects
	delete db;
	delete logger;

	// all done
	return 0;
}

/* vim:set ts=2 sw=2: */
