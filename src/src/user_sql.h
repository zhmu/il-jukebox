/*
 * user_sql.h
 *
 * This is the jukebox SQL user manager.
 *
 */
#include <stdlib.h>
#include "user.h"

#ifndef __USERSQL_H__
#define __USERSQL_H__

/*!
 * \class USERS_SQL
 * \brief This will handle the user pool using SQL
 */
class USERS_SQL : public USERS {
public:
	/*!
	 * \brief Initialises the SQL user database.
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

private:
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
};

#endif /* __USERSQL_H__ */
