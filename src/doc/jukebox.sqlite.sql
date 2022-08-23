/* artists: holds all artists available */
CREATE TABLE artists (
	id INTEGER NOT NULL PRIMARY KEY,
	name VARCHAR(255) NOT NULL
);

/* albums: holds all albums available */
CREATE TABLE albums (
	id INTEGER NOT NULL PRIMARY KEY,
	artistid INTEGER,
	name VARCHAR(255) NOT NULL
);

/* tracks: holds all tracks available */
CREATE TABLE tracks (
	id INTEGER NOT NULL PRIMARY KEY,
	artistid INTEGER,
	albumid INTEGER,
	title VARCHAR(255) NOT NULL,
	filename TEXT NOT NULL,
	year YEAR,
	trackno INTEGER NOT NULL,
	playcount INTEGER NOT NULL
);

/* queue: holds the tracks to play */
CREATE TABLE queue (
	id INTEGER NOT NULL PRIMARY KEY,
	trackid INTEGER NOT NULL,
	timestamp DATETIME NOT NULL,
	playtime DATETIME
);

/* users: holds all users */
CREATE TABLE users (
	id INTEGER NOT NULL PRIMARY KEY,
	username VARCHAR(64) NOT NULL,
	password VARCHAR(32) NOT NULL,	
	status INTEGER NOT NULL
);

/* collections: holds all available collections */
CREATE TABLE collections (
	id INTEGER NOT NULL PRIMARY KEY,
	owner INTEGER NOT NULL,
	name VARCHAR(128) NOT NULL
);

/* collection_contents: holds all available */
CREATE TABLE collection_contents (
	id INTEGER NOT NULL PRIMARY KEY,
	collectionid INTEGER NOT NULL,
	trackid INTEGER NOT NULL
);

/* vim:set ts=2 sw=2: */
