/*
 * user_ldap.cc - Jukebox LDAP user management code
 *
 */
#ifdef USERDB_LDAP

#include <sys/types.h>
#include <config.h>
#include <ldap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#include "jukebox.h"
#include "user_ldap.h"

/*
 * USERS_LDAP::USERS_LDAP()
 *
 * This will construct the user manager.
 *
 */
USERS_LDAP::USERS_LDAP() {
	/* defaults for all */
	ldaphost = NULL; basedn = NULL; login_attr = NULL;
	ldap_admin_groups = NULL; nextDB = NULL; ssl = 0;
}

/*
 * USERS_LDAP::~USERS_LDAP()
 *
 * This will destruct the user manager.
 *
 */
USERS_LDAP::~USERS_LDAP() {
	// free all memory
	if (ldap_admin_groups) free (ldap_admin_groups);
	if (ldaphost) free (ldaphost);
	if (basedn) free (basedn);
	if (login_attr) free (login_attr);
}

/*
 * USERS_LDAP::init()
 *
 * This will initialize the user databases. It will return 0 on failure or
 * 1 on success.
 *
 */
int
USERS_LDAP::init() {
	LDAP* ld;

	// load the configuration
	reloadConfig();

	ld = openConnection();
	if (ld == NULL)
		return 0;

	// yeppee!
	closeConnection (ld);
	return 1;
}

/*
 * USERS_LDAP::fetchUser (int id, const char* name, USER* user)
 *
 * This will try to fetch an user to [user]. If [id] is not 0, it is
 * expected to contain the user's name, otherwise [id] is used as the user's ID.
 * This will return 0 on failure or 1 on success.
 *
 */
int
USERS_LDAP::fetchUser (int id, const char* name, USER* user) {
	LDAPMessage* res = NULL;
	LDAPMessage* e;
	BerElement* ber = NULL;
	char* a;
	char** vals;
	char* attrs[] = { "uidNumber", login_attr, NULL };
	char filter[512];
	char* curoffs;
	char* tmp;
	char tempgroup[512];
	LDAP* ld = openConnection();
	if (ld == NULL)
		return 0;

	// clear the user out
	memset (user, 0, sizeof (USER));

	// username given?
	if (id == 0)
		// yes. filter on the username
		snprintf (filter, sizeof (filter), "(%s=%s)", login_attr, name);
	else
		// no. filter on the user id
		snprintf (filter, sizeof (filter), "(uidNumber=%u)", id);

	// search the the user
	if (ldap_search_s (ld, basedn, LDAP_SCOPE_SUBTREE, filter, attrs, 0, &res) != LDAP_SUCCESS)
		return 0;

	// fetch the first entry
	e = ldap_first_entry (ld, res);
	if (e == NULL)
		/* this failed. bail out */
		goto fail;

	/* do all attributes */
	for (a = ldap_first_attribute (ld, e, &ber); a != NULL;
		a = ldap_next_attribute (ld, e, ber)) {

		/* fetch the values */
		vals = ldap_get_values (ld, e, a);
		if (vals == NULL) {
			/* this failed. bail out */
			ldap_memfree (a);
			goto fail;
		}

		/* got an uid? */
		if (!strcmp (a, login_attr))
			/* yes. store it */
			strncpy (user->username, vals[0], USER_MAX_USERNAME_LEN);
		else if (!strcmp (a, "uidNumber")) {
			/* no, we got an user id! store it */
			user->id = atoi (vals[0]);
		}

		/* free the values and attribute name */
		ldap_value_free (vals);
		ldap_memfree (a);
	}

	// fix the rest of the entry
	strcpy (user->password, "*");
	user->status = USER_STATUS_USER;

fail:
	/* free everything */
	if (res) ldap_msgfree (res);
	if (ber) ber_free (ber, 0);
	closeConnection (ld);

	// got everything?
	if ((!user->id) || (!user->username[0]))
		// no. bail out
		return 0;

	// fix the password field and user status
	strcpy (user->password, "*");
	user->status = USER_STATUS_USER;

	// is the admin group known?
	if (ldap_admin_groups == NULL)
		return 1;

	// check whether the user is in the group
	curoffs = ldap_admin_groups;
	while (1) {
		// locate a splitter
		tmp = strchr (curoffs, ',');
		if (tmp == NULL)
			// none found. use the end of the string then
			tmp = strchr (curoffs, 0);

		// copy the group name
		strncpy (tempgroup, curoffs, (tmp - curoffs));

		// does the user live in this group?
		if (isUserInGroup (user->username, tempgroup)) {
			// yes. grant the user admin rights
			user->status = USER_STATUS_ADMIN;

			// all done
			return 1;
		}

		// end of the line?
		if (!*tmp)
			// yes. bail out
			break;

		// no. start at this offset
		curoffs = tmp + 1;
	}

	// all done. return victory
	return 1;
}

/*
 * USERS_LDAP::fetchUserByName (const char* name, USER* user)
 *
 * This will try to fetch user [name] to [user]. It will return 0 on failure or
 * 1 on success.
 *
 */
int
USERS_LDAP::fetchUserByName (const char* name, USER* user) {
	// just pass it through to fetchUser()
	return fetchUser (0, name, user);
}

/*
 * USERS_LDAP::fetchUserByID (int id, USER* user)
 *
 * This will try to fetch user [id] to [user]. It will return 0 on failure or
 * 1 on success.
 *
 */
int
USERS_LDAP::fetchUserByID (int id, USER* user) {
	// just pass it through to fetchUser()
	return fetchUser (id, NULL, user);
}

/*
 * USERS_LDAP::verifyPassword (const char* password, USER* user)
 *
 * This will verify the user [user] with password [password]. It will return
 * 0 on failure or 1 on success.
 *
 */
int
USERS_LDAP::verifyPassword (const char* password, USER* user) {
	char filter[512];
	LDAPMessage* res = NULL;
	LDAPMessage* e;
	char* dn = NULL;
	int result = 0;
	LDAP* ld;

	// even got a password?
	if (!strlen (password))
		// no. auto-deny
		return 0;

	// establish an LDAP connection
	ld = openConnection();
	if (ld == NULL)
		return 0;

	/*
	 * Allright. since the base DN we know usually doesn't include the correct
	 * organizationalUnit (since groups and accounts are usually in seperate
	 * ones), we must first search for the user.
	 *
	 * This was inspired by pam_ldap, which does the same thing.
	 */

	// filter on the username
	snprintf (filter, sizeof (filter), "(%s=%s)", login_attr, user->username);

	// search the the user
	if (ldap_search_s (ld, basedn, LDAP_SCOPE_SUBTREE, filter, NULL, 0, &res) != LDAP_SUCCESS)
		// this failed. bail out
		goto leave;

	// fetch the first entry
	e = ldap_first_entry (ld, res);
	if (e == NULL)
		// this failed. bail out
		goto leave;

	// fetch the DN
	dn = ldap_get_dn (ld, e);
	if (dn == NULL)
		// this failed. bail out
		goto leave;

	// bind to the host as user
	if (ldap_bind_s (ld, dn, password, LDAP_AUTH_SIMPLE) != LDAP_SUCCESS)
		// this failed. complain
		goto leave;

	// victory!
	result++;

leave:
	// if we got a DN, free it
	if (dn) ldap_memfree (dn);
	if (res) ldap_msgfree (res);
	closeConnection (ld);

	// return whatever status code we got
	return result;
}

/*
 * Reloads the LDAP configuration
 * 
 */
void
USERS_LDAP::reloadConfig() {
	char* ptr = NULL;

	// got admin groups?
	if (ldap_admin_groups)
		// yes. free them
		free (ldap_admin_groups);

	// fetch the LDAP configuration file name
	if (config->get_string ("userdb", "ldap_admin_groups", &ptr) != CONFIGFILE_OK)
		// this failed. no admin groups then
		ldap_admin_groups = NULL;
	else
		// this worked. copy them over
		ldap_admin_groups = strdup (ptr);

	// fetch the LDAP version to use
	if (config->get_value ("userdb", "ldap_version", &protocol_version) != CONFIGFILE_OK)
		// this failed. default to 2
		protocol_version = 2;

	// fetch the LDAP host
	if (config->get_string ("userdb", "ldap_host", &ptr) != CONFIGFILE_OK)
		ldaphost = "localhost";
	else
		ldaphost = strdup (ptr);

	// base dn
	if (config->get_string ("userdb", "ldap_basedn", &ptr) != CONFIGFILE_OK)
		basedn = "";
	else
		basedn = strdup (ptr);

	// login attribute
	if (config->get_string ("userdb", "ldap_login_attr", &ptr) != CONFIGFILE_OK)
		login_attr = "uid";
	else
		login_attr = strdup (ptr);

	// ssl
	if (config->get_string ("userdb", "ldap_ssl", &ptr) != CONFIGFILE_OK)
		ssl = ((!strcasecmp (ptr, "yes")) || (!strcasecmp (ptr, "on"))) ? 1 : 0;
	else
		ssl = 0;
}

int 
USERS_LDAP::isUserInGroup (char* username, char* groupname) {
	LDAPMessage* res = NULL;
	LDAPMessage* e;
	BerElement* ber = NULL;
	char* a;
	char** vals;
	char* attrs[] = { "memberUid", NULL };
	char filter[512];
	int i, result = 0;
	LDAP* ld = openConnection();
	if (ld == NULL)
		return 0;

	// build the filter on the username
	snprintf (filter, sizeof (filter), "(cn=%s)", groupname);

	// search the the group
	if (ldap_search_s (ld, basedn, LDAP_SCOPE_SUBTREE, filter, attrs, 0, &res) != LDAP_SUCCESS)
		// this failed. bail out
		return 0;

	// fetch the first entry
	e = ldap_first_entry (ld, res);
	if (e == NULL)
		// this failed. bail out
		goto leave;

	/* do all attributes */
	for (a = ldap_first_attribute (ld, e, &ber); a != NULL;
		a = ldap_next_attribute (ld, e, ber)) {

		/* fetch the values */
		vals = ldap_get_values (ld, e, a);
		if (vals != NULL) {
			/* wade through all values */
			for (i = 0; vals[i] != NULL; i++)
				/* match? */
				if (!strcasecmp (vals[i], username))
					/* yes. victory */
					result++;

			/* free the values and attribute name */
			ldap_value_free (vals);
		}

		/* free the memort */
		ldap_memfree (a);
	}

leave:
	/* free everything */
	if (res) ldap_msgfree (res);
	if (ber) ber_free (ber, 0);
	closeConnection (ld);

	// return the result
	return result;
}

/*
 * Creates a new LDAP connection.
 *
 */
LDAP*
USERS_LDAP::openConnection() {
	LDAP* ld;

	// connect to the LDAP server
	ld = ldap_init (ldaphost, 0);
	if (ld == NULL)
		return 0;

	// we want LDAP ... whatever the user said
	ldap_set_option (ld, LDAP_OPT_PROTOCOL_VERSION, &protocol_version);

	// SSL?
	if (ssl)
		// yes. force a start
		if (ldap_start_tls_s (ld, NULL, NULL) != LDAP_SUCCESS)
			return NULL;

	// yay
	return ld;
}

/*
 * Closes an LDAP connection.
 */
void
USERS_LDAP::closeConnection(LDAP* ld) {
	ldap_unbind (ld);
}

#endif /* USERDB_LDAP */

/* vim:set ts=2 sw=2: */
