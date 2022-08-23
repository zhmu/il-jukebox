DROP TABLE IF EXISTS artists;
DROP TABLE IF EXISTS albums;
DROP TABLE IF EXISTS tracks;
DROP TABLE IF EXISTS queue;
DROP TABLE IF EXISTS users;
DROP TABLE IF EXISTS collections;
DROP TABLE IF EXISTS collection_contents;

/* artists: holds all artists available */
CREATE TABLE artists (
	id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
	name VARCHAR(255) NOT NULL,
	INDEX (name)
);

/* albums: holds all albums available */
CREATE TABLE albums (
	id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
	artistid BIGINT,
	name VARCHAR(255) NOT NULL,
	INDEX (artistid),
	INDEX (name)
);

/* tracks: holds all tracks available */
CREATE TABLE tracks (
	id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
	artistid BIGINT,
	albumid BIGINT,
	title VARCHAR(255) NOT NULL,
	filename TEXT NOT NULL,
	year YEAR,
	trackno INTEGER NOT NULL,
	playcount INTEGER NOT NULL,
	INDEX (artistid),
	INDEX (title),
	INDEX (albumid),
	INDEX (albumid, trackno)
);

/* queue: holds the tracks to play */
CREATE TABLE queue (
	id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
	trackid BIGINT NOT NULL,
	timestamp DATETIME NOT NULL,
	playtime DATETIME
);

/* users: holds all users */
CREATE TABLE users (
	id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
	username VARCHAR(64) NOT NULL,
	password VARCHAR(32) NOT NULL,	
	status INTEGER NOT NULL,
	INDEX (username)
);

/* collections: holds all available collections */
CREATE TABLE collections (
	id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
	owner BIGINT NOT NULL,
	name VARCHAR(128) NOT NULL
);

/* collection_contents: holds all available */
CREATE TABLE collection_contents (
	id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
	collectionid BIGINT NOT NULL,
	trackid BIGINT NOT NULL,
	INDEX (collectionid)
);

/* vim:set ts=2 sw=2: */
