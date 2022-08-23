/*
 * user_ldap.h
 *
 * This is the jukebox LDAP user manager.
 *
 */
#include <stdlib.h>
#include "user.h"

#ifndef __USERLDAP_H__
#define __USERLDAP_H__

#ifdef USERDB_LDAP
#include <ldap.h>
#include <lber.h>

/*!
 * \class USERS_LDAP
 * \brief This will handle the user pool using LDAP
 */
class USERS_LDAP : public USERS {
public:
	//! \brief Constructs a new LDAP user pool
	USERS_LDAP();

	//! \brief Destructrs the LDAP user pool
	~USERS_LDAP();

	/*!
	 * \brief Initialises the LDAP user database.
	 *
	 * This will return zero on failure or non-zero on success.
	 *
	 */
	int init();

	/*!
	 * \brief This will fetch user information by ID.
	 *
	 * This will return zero on failure or non-zero on success.
	 *
	 * \param id The user ID to fetch information about.
	 * \param user Buffer to put the information in.
	 *
	 */
	int		fetchUserByID (int id, USER* user);

	/*!
	 * \brief This will fetch user information by name.
	 *
	 * This will return zero on failure or non-zero on success.
	 *
	 * \param name The user name to fetch information about.
	 * \param user Buffer to put the information in.
	 *
	 */
	int		fetchUserByName (const char* name, USER* user);

	/*!
	 * \brief Verifies an username and password combination
	 *
	 * This will return zero on failure or non-zero on success.
	 *
	 * \param user The user to verify the password of
	 * \param password The password to verify
	 */
	int verifyPassword (const char* password, USER* user);

	/*!
	 * \brief Reloads the user database configuration
	 */
	void reloadConfig();

private:
	/*!
	 * \brief This will parse a pam.conf-alike config file
	 *
	 * This will return zero on failure or non-zero on success.
	 *
	 * \param fname The configuration file to parse
	 */
	int parse_config (char* fname);

	/*!
	 * \brief This will fetch user information by name or ID.
	 *
	 * This will return zero on failure or non-zero on success. If name is NULL,
	 * id will be used.
	 *
	 * \param id The user ID to fetch information about, if name is NULL
	 * \param name The user name to fetch information about, if not NULL
	 * \param user Buffer to put the information in.
	 *
	 */
	int		fetchUser (int id, const char* name, USER* user);	

	/*! \brief Checks whether an user is in a LDAP group
	 *  \returns Non-zero if user is in the group, zero otherwise
	 *  \param username The name of the user
	 *  \param groupname The name of the group
	 */
	int isUserInGroup (char* username, char* groupname);

	/*! \brief Returns a LDAP connection handle
   *  \return The handle on success or NULL on failure.
   */
	LDAP* openConnection();

	/*! \brief Closes an LDAP connection
   *  \param ld Handle to close
   */
	void closeConnection(LDAP* ld);

	char* ldaphost;
	char* basedn;
	char* login_attr;
	int ssl;

	char* ldap_admin_groups;

	//! \brief LDAP Protocol version used
	int protocol_version;
};

#endif /* USERDB_LDAP */

#endif /* __USERLDAP_H__ */

/* vim:set ts=2 sw=2: */
