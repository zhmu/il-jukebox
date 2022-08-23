/*
 * jukebox.h - Main jukebox include file
 *
 */

#include <libplusplus/database.h>
#include <libplusplus/log.h>
#include "paths.h"
#include "player.h"
#include "queue.h"
#include "server.h"
#include "config.h"
#include "user.h"
#include "volume.h"

#ifndef __JUKEBOX_H__
#define __JUKEBOX_H__

#ifndef CONFIG_FILENAME
/* CONFIG_FILENAME is the filename of the configuration file! */
#define CONFIG_FILENAME SYSCONFDIR "/jukebox.conf"
#endif /* CONFIG_FILENAME */

#ifndef CONFIG_PORT
/* CONFIG_PORT is the default port we bind to */
#define CONFIG_PORT 4444
#endif /* CONFIG_PORT */

extern JUKECONFIG* config;
extern DATABASE* db;
extern LOG* logger;
extern QUEUE* queue;
extern USERS* users;
extern PLAYER* player;
extern JUKESERVER* server;
extern VOLUME* volume;

#endif // __JUKEBOX_H__

/* vim:set ts=2 sw=2: */
