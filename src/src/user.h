/*
 * user.h
 *
 * This is the jukebox user manager.
 *
 */
#include <stdlib.h>

#ifndef __USER_H__
#define __USER_H__

// USER_STATUS_xxx are the known privilege levels
#define USER_STATUS_ANON				0
#define USER_STATUS_USER				1
#define USER_STATUS_ADMIN				2

// USER_MAX_xxx_LEn define the maximum lengths of the fields
#define USER_MAX_USERNAME_LEN		64
#define USER_MAX_PASSWORD_LEN		32

/*!
 * \struct USER
 * \brief Capable of storing information about an user.
 */
struct USER {
	//! \brief The unique ID of the user
	int		id;

	//! \brief The current user status
	int		status;

	//! \brief The username of the user

	char	username[USER_MAX_USERNAME_LEN];

	//! \brief The password of the user
	char	password[USER_MAX_PASSWORD_LEN];
};

/*!
 * \class USERS
 * \brief This will handle the user pool.
 */
class USERS {
public:
	/*!
	 * \brief Initialises the user database.
	 *
	 * This will return zero on failure or non-zero on success.
	 *
	 */
	virtual int init() = NULL;

	/*!
	 * \brief This will fetch user information by ID.
	 *
	 * This will return zero on failure or non-zero on success.
	 *
	 * \param id The user ID to fetch information about.
	 * \param user Buffer to put the information in.
	 *
	 */
	virtual int fetchUserByID (int id, USER* user) = NULL;

	/*!
	 * \brief This will fetch user information by name.
	 *
	 * This will return zero on failure or non-zero on success.
	 *
	 * \param name The user name to fetch information about.
	 * \param user Buffer to put the information in.
	 *
	 */
	virtual int fetchUserByName (const char* name, USER* user) = NULL;

	/*!
	 * \brief Verifies an username and password combination
	 *
	 * This will return zero on failure or non-zero on success.
	 *
	 * \param user The user to verify the password of
	 * \param password The password to verify
	 */
	virtual int verifyPassword (const char* password, USER* user) = NULL;

	/*!
	 * \brief Reloads the user database configuration
	 */
	inline virtual void reloadConfig() { };

	//! \brief Fetches the next user database from the list, or NULL
	inline virtual USERS* getNextDB() { return nextDB; }

	/* !\brief Stores the next user database in the list
	 *  \param db The new next user database
	 */
	inline virtual void setNextDB(USERS* db) { nextDB = db; }

protected:
	//! \brief Next user database to chain through
	USERS* nextDB;
};

#endif // __USER_H__

/* vim:set ts=2 sw=2: */
