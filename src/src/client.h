/*
 * client.h
 *
 * This is the jukebox server client code. It will handle all requests.
 *
 */
#include <stdlib.h>
#include <libplusplus/network.h>
#include "user.h"

#ifndef __JUKECLIENT_H__
#define __JUKECLIENT_H__

// JUKECLIENT_MAX_DATA_LENGTH is the maximum length of a single request
#define JUKECLIENT_MAX_DATA_LENGTH	1024

// JUKECLIENT_STATE_xxx are the states an user can be in
#define JUKECLIENT_STATE_CONN				0
#define JUKECLIENT_STATE_AUTH				1

// JUKECLIENT_MSG_xxx are the messages we can send
#define JUKECLIENT_MSG_WELCOME	"[I] Welcome to JukeServer 0.1\n"
#define JUKECLIENT_MSG_UNKNOWN	"[E] Unknown command\n"
#define JUKECLIENT_MSG_BYE			"[I] Be seeing ya!\n"
#define JUKECLIENT_MSG_NOUSER		"[E] No such user\n"
#define JUKECLIENT_MSG_USEROK		"[I] Username OK\n"
#define JUKECLIENT_MSG_USERFIRST "[E] Tell me the username first\n"
#define JUKECLIENT_MSG_BADPASS	"[E] Incorrect password\n"
#define JUKECLIENT_MSG_PASSOK		"[I] Welcome %s!\n"
#define JUKECLIENT_MSG_MUSTAUTH	"[E] You must be authenticated first\n"
#define JUKECLIENT_MSG_PAUSED		"[I] Playback paused\n"
#define JUKECLIENT_MSG_RESUMED	"[I] Playback resumed\n"
#define JUKECLIENT_MSG_STOPPED	"[I] Playback stopped\n"
#define JUKECLIENT_MSG_STARTED	"[I] Playback started\n"
#define JUKECLIENT_MSG_NOTPLAYING "[E] Not playing\n"
#define JUKECLIENT_MSG_SKIPPED	"[I] Track skipped by %s\n"
#define JUKECLIENT_MSG_USERLIST	"[U] %s\n"
#define JUKECLIENT_MSG_USERSLISTED	"[I] User list end\n"
#define JUKECLIENT_MSG_STATUS				"[S] Status:{%c} Random:{%c} Locked:{%c} Song:{%s} Artist:{%s}\n"
#define JUKECLIENT_MSG_YESORNO	"[E] Arguments must be YES or NO\n"
#define JUKECLIENT_MSG_RANDOMON	"[I] Random play turned on\n"
#define JUKECLIENT_MSG_RANDOMOFF	"[I] Random play turned off\n"
#define JUKECLIENT_MSG_QUEUEITEM	"[Q] ID:{%u} Song:{%s} Artist:{%s}\n"
#define JUKECLIENT_MSG_QUEUEDONE	"[I] Queue listed\n"
#define JUKECLIENT_MSG_REMOVESYN	"[E] Argument must be a queue item ID\n"
#define JUKECLIENT_MSG_REMOVEOK		"[I] Queue item removed\n"
#define JUKECLIENT_MSG_ALREADYPLAYING	"[E] Already playing\n"
#define JUKECLIENT_MSG_NOPRIVS		"[E] Privileged command, and not for you\n"
#define JUKECLIENT_MSG_LOCKED			"[I] Player is now locked\n"
#define JUKECLIENT_MSG_UNLOCKED		"[I] Player is now unlocked\n"
#define JUKECLIENT_MSG_LOCKERR		"[E] Player locked, and you can't override it\n"
#define JUKECLIENT_MSG_CLEARED		"[I] Queue cleared\n"
#define JUKECLIENT_MSG_ALBUM		  "[A] ID:{%u} Artist:{%u} Album:{%s}\n"
#define JUKECLIENT_MSG_ALBUMEND	  "[I] Albums listed\n"
#define JUKECLIENT_MSG_ARTIST		  "[A] ID:{%u} Artist:{%s}\n"
#define JUKECLIENT_MSG_ARTISTEND	"[I] Artists listed\n"
#define JUKECLIENT_MSG_ENQSYN			"[E] Argument must be a number\n"
#define JUKECLIENT_MSG_LISTSYN		JUKECLIENT_MSG_ENQSYN
#define JUKECLIENT_MSG_GETASYN		JUKECLIENT_MSG_ENQSYN
#define JUKECLIENT_MSG_ENQOK			"[I] Enqueued\n"
#define JUKECLIENT_MSG_LISTOK		  "[I] Tracks listed\n"
#define JUKECLIENT_MSG_TRACK			"[T] ID:{%u} Title:{%s}\n"
#define JUKECLIENT_MSG_NOARTIST		"[E] No such artist\n"
#define JUKECLIENT_MSG_NOALBUM		"[E] No such album\n"
#define JUKECLIENT_MSG_NOTRACK		"[E] No such track\n"
#define JUKECLIENT_MSG_NOVOL			"[E] Volume manager unavailable\n"
#define JUKECLIENT_MSG_VOLOK			"[I] Volume changed\n"
#define JUKECLIENT_MSG_VOLUME			"[V] Volume:{%u}\n"
#define JUKECLIENT_MSG_IDENTFAIL	"[E] Identify failed\n"
#define JUKECLIENT_MSG_NOIDENT	  "[E] Ident is not allowed\n"
#define JUKECLIENT_MSG_NOIDENTHOST "[E] Ident is not allowed from this host\n"
#define JUKECLIENT_MSG_UPDATESON	"[I] Updates turned on\n"
#define JUKECLIENT_MSG_UPDATESOFF	"[I] Updates turned off\n"
#define JUKECLIENT_UPDATE_SONG	    "[U] Status:{%c} Artist:{%s} Song:{%s}\n"

// JUKECLIENT_CMD_xxx are the commands we support
#define JUKECLIENT_CMD_EXIT					"exit"
#define JUKECLIENT_CMD_DISCONNECT1	"disconnect"
#define JUKECLIENT_CMD_DISCONNECT2	"disc"
#define JUKECLIENT_CMD_BYE					"bye"
#define JUKECLIENT_CMD_USER					"user"
#define JUKECLIENT_CMD_PASSWORD			"pass"
#define JUKECLIENT_CMD_PASSWORD2		"password"
#define JUKECLIENT_CMD_PAUSE				"pause"
#define JUKECLIENT_CMD_CONTINUE			"continue"
#define JUKECLIENT_CMD_CONTINUE2		"cont"
#define JUKECLIENT_CMD_STOP					"stop"
#define JUKECLIENT_CMD_PLAY					"play"
#define JUKECLIENT_CMD_NEXT					"next"
#define JUKECLIENT_CMD_NEXT2				"skip"
#define JUKECLIENT_CMD_USERS				"users"
#define JUKECLIENT_CMD_STATUS				"stat"
#define JUKECLIENT_CMD_STATUS2			"status"
#define JUKECLIENT_CMD_RANDOM				"random"
#define JUKECLIENT_CMD_RANDOM2			"rand"
#define JUKECLIENT_CMD_QUEUE				"queue"
#define JUKECLIENT_CMD_QUEUE2				"q"
#define JUKECLIENT_CMD_REMOVE				"remove"
#define JUKECLIENT_CMD_REMOVE2			"rem"
#define JUKECLIENT_CMD_LOCK					"lock"
#define JUKECLIENT_CMD_UNLOCK				"unlock"
#define JUKECLIENT_CMD_CLEAR				"clear"
#define JUKECLIENT_CMD_HELP					"help"
#define JUKECLIENT_CMD_ALBUMS				"albums"
#define JUKECLIENT_CMD_ARTISTALBUMS	"artistalbums"
#define JUKECLIENT_CMD_ARTISTS			"artists"
#define JUKECLIENT_CMD_ENQUEUETR	  "enqueuetrack"
#define JUKECLIENT_CMD_ENQUEUEAL	  "enqueuealbum"
#define JUKECLIENT_CMD_LISTALBUM		"listalbum"
#define JUKECLIENT_CMD_GETARTIST		"getartist"
#define JUKECLIENT_CMD_GETALBUM			"getalbum"
#define JUKECLIENT_CMD_GETTRACK			"gettrack"
#define JUKECLIENT_CMD_VOLUME				"volume"
#define JUKECLIENT_CMD_VOLUP				"volup"
#define JUKECLIENT_CMD_VOLDN				"voldn"
#define JUKECLIENT_CMD_IDENT				"ident"
#define JUKECLIENT_CMD_UPDATES			"updates"

// JUKECLIENT_HANDLE_PRIV is just for ease
#define JUKECLIENT_HANDLE_PRIV(x) if (!checkPriv(x)) return;

/*!
 * \class JUKECLIENT
 * \brief A connected client
 *
 */
class JUKECLIENT : public SERVICECLIENT {
public:
	//! \brief Greets the client and sets him up
	void			welcome();

	//! \brief Handles incoming data
	void			incoming();

	//! \brief Returns the state the user is in
	int				getState();

	//! \brief Need to send the user updates?
	inline int wantsUpdates() { return sendUpdates; }

	/*!
	 * \brief Returns the current user authenticated
	 *
	 * If no user is authenticated, NULL will be returned.
	 *
	 */
	USER*			getUser();

protected:
	//! \brief The state the user is in
	int				state;

	/*!
	 * \brief The user ID of the connected client
	 *
	 * This is -1 if the user hasn't yet authenticated himself.
	 */
	int				userid;

	//! \brief User information, if the user is authenticated
	USER			user;

	//! \brief Flag: Keep the user updated on track changes?
	int				sendUpdates;

private:
	/*!
	 * \brief Checks the client for enough privileges.
	 *
	 * This will return zero if the user lacks the needed privileges. If the user
	 * has enough privileges, non-zero will be returned.
	 *
	 * \parm cmd The command to check for
	 *
	 */
	int				checkPriv (char* cmd);

	//! \brief This will handle the DISCONNECT command
	void			cmdDisconnect();

	//! \brief This will handle the USER command
	void			cmdUser (char*);

	//! \brief This will handle the PASSWORD command
	void			cmdPassword (char*);

	//! \brief This will handle the PAUSE command
	void			cmdPause();

	//! \brief This will handle the RESUME command
	void			cmdResume();

	//! \brief This will handle the STOP command
	void			cmdStop();

	//! \brief This will handle the PLAY command
	void			cmdPlay();

	//! \brief This will handle the SKIP command
	void			cmdSkip();

	//! \brief This will handle the USERS command
	void			cmdUsers();

	//! \brief This will handle the STATUS command
	void			cmdStatus();

	//! \brief This will handle the RANDOM command
	void			cmdRandom(char*);

	//! \brief This will handle the QUEUE command
	void			cmdQueue();

	//! \brief This will handle the REMOVE command
	void			cmdRemove(char*);

	//! \brief This will handle the LOCK command
	void			cmdLock();

	//! \brief This will handle the UNLOCK command
	void			cmdUnlock();

	//! \brief This will handle the HELP command
	void			cmdHelp();

	//! \brief This will handle the CLEAR command
	void			cmdClear();

	//! \brief This will handle the ALBUMS command
	void			cmdAlbums();

	//! \brief This will handle the ARTISTS command
	void			cmdArtists();

	//! \brief This will handle the ENQUEUETRACK command
	void			cmdEnqueueTrack(char*);

	//! \brief This will handle the ENQUEUEALBUM command
	void			cmdEnqueueAlbum(char*);

	//! \brief This will handle the LISTALBUM command
	void			cmdListAlbum(char*);

	//! \brief This will handle the GETALBUM command
	void			cmdGetAlbum(char*);

	//! \brief This will handle the GETARTIST command
	void			cmdGetArtist(char*);

	//! \brief This will handle the GETTRACK command
	void			cmdGetTrack(char*);

	//! \brief This will handle the ARTISTALBUMS command
	void			cmdArtistAlbums(char*);

	//! \brief This will handle the VOLUP command
	void			cmdVolumeUp();

	//! \brief This will handle the VOLDN command
	void			cmdVolumeDown();

	//! \brief This will handle the VOLUME command
	void			cmdVolume(char*);

	//! \brief This will handle the IDENT command
	void			cmdIdent();

	//! \brief This will handle the UPDATES command
	void			cmdUpdates(char*);
};

#endif // __JUKECLIENT_H__

/* vim:set ts=2 sw=2: */
