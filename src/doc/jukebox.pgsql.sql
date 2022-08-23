DROP TABLE artists CASCADE;
DROP TABLE albums CASCADE;
DROP TABLE tracks CASCADE;
DROP TABLE queue CASCADE;
DROP TABLE users CASCADE;
DROP TABLE collections CASCADE;
DROP TABLE collection_contents CASCADE;
DROP SEQUENCE artists_id_seq;
DROP SEQUENCE albums_id_seq;
DROP SEQUENCE tracks_id_seq;
DROP SEQUENCE queue_id_seq;
DROP SEQUENCE collections_id_seq;
DROP SEQUENCE collections_contents_id_seq;

CREATE TABLE artists (
	id SERIAL NOT NULL PRIMARY KEY,
	name VARCHAR(255) NOT NULL
);
CREATE INDEX artists_name_index ON artists (name);

CREATE TABLE albums (
	id SERIAL NOT NULL PRIMARY KEY,
	artistid BIGINT NOT NULL,
	name VARCHAR(255) NOT NULL,
	FOREIGN KEY (artistid) REFERENCES artists (id) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE INDEX albums_artistid_index ON albums (artistid);

CREATE TABLE tracks (
	id SERIAL NOT NULL PRIMARY KEY,
	artistid BIGINT,
	albumid BIGINT,
	title VARCHAR(255) NOT NULL,
	filename TEXT NOT NULL,
	year INTEGER,
	trackno INTEGER NOT NULL,
	playcount INTEGER NOT NULL,
	FOREIGN KEY (artistid) REFERENCES artists (id) ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY (albumid) REFERENCES albums (id) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE INDEX tracks_artistid_index ON tracks (artistid);
CREATE INDEX tracks_albumid_index ON tracks (albumid);

CREATE TABLE queue (
	id SERIAL NOT NULL PRIMARY KEY,
	trackid BIGINT NOT NULL,
	timestamp TIMESTAMP NOT NULL,
	playtime TIMESTAMP,
	FOREIGN KEY (trackid) REFERENCES tracks (id) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE TABLE users (
	id SERIAL NOT NULL PRIMARY KEY,
	username VARCHAR(64) NOT NULL,
	password VARCHAR(32) NOT NULL,	
	status INTEGER NOT NULL
);
CREATE INDEX users_username_index ON users (username);

CREATE TABLE collections (
	id SERIAL NOT NULL PRIMARY KEY,
	owner BIGINT NOT NULL,
	name VARCHAR(128) NOT NULL
);

CREATE TABLE collection_contents (
	id SERIAL NOT NULL PRIMARY KEY,
	collectionid BIGINT NOT NULL,
	trackid BIGINT NOT NULL,
	FOREIGN KEY (collectionid) REFERENCES collections (id) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE INDEX collection_contents_collectionid_index ON collection_contents (collectionid);
