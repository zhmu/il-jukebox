/*
 * user_sql.cc - Jukebox user management code for SQL
 *
 */
#ifdef USERDB_SQL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "jukebox.h"
#include "user_sql.h"

/*
 * USERS_SQL::init()
 *
 * This will initialize the user databases. It will return 0 on failure or
 * 1 on success.
 *
 */
int
USERS_SQL::init() {
	// reset the default database and leave
	nextDB = NULL;
	return 1;
}

/*
 * USERS_SQL::fetchUser (int id, const char* name, USER* user)
 *
 * This will try to fetch an user to [user]. If [id] is not 0, it is
 * expected to contain the user's name, otherwise [id] is used as the user's ID.
 * This will return 0 on failure or 1 on success.
 *
 */
int
USERS_SQL::fetchUser (int id, const char* name, USER* user) {
	DBRESULT* res;

	// username given?
	if (id == 0) {
		// yes. execute the query based on username
		res = db->query ("SELECT id,username,password,status FROM users WHERE username=?", name);
	} else {
		// no. execute the query based on user id
		res = db->query ("SELECT id,username,password,status FROM users WHERE id=#", id);
	}

	// did this work?
	if (res == NULL)
		// no. return bad status
		return 0;

	// got any results?
	if (res->numRows() == 0) {
		// no. zap the result and return bad status
		delete res;
		return 0;
	}

	// copy the values over
	user->id = res->fetchColumnAsInteger (0);
	strncpy (user->username, res->fetchColumnAsString (1), USER_MAX_USERNAME_LEN);
	strncpy (user->password, res->fetchColumnAsString (2), USER_MAX_PASSWORD_LEN);
	user->status = res->fetchColumnAsInteger (3);

	// all done. drop the result and return victory
	delete res;
	return 1;
}

/*
 * USERS_SQL::fetchUserByName (const char* name, USER* user)
 *
 * This will try to fetch user [name] to [user]. It will return 0 on failure or
 * 1 on success.
 *
 */
int
USERS_SQL::fetchUserByName (const char* name, USER* user) {
	// just pass it through to fetchUser()
	return fetchUser (0, name, user);
}

/*
 * USERS_SQL::fetchUserByID (int id, USER* user)
 *
 * This will try to fetch user [id] to [user]. It will return 0 on failure or
 * 1 on success.
 *
 */
int
USERS_SQL::fetchUserByID (int id, USER* user) {
	// just pass it through to fetchUser()
	return fetchUser (id, NULL, user);
}

/*
 * USERS_SQL::verifyPassword (const char* password, USER* user)
 *
 * This will verify the user [user] with password [password]. It will return
 * 0 on failure or 1 on success.
 *
 */
int
USERS_SQL::verifyPassword (const char* password, USER* user) {
	return (!strcmp (user->password, password)) ? 1 : 0;
}
#endif /* USERDB_SQL */

/* vim:set ts=2 sw=2: */
