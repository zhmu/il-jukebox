bin_PROGRAMS = jukebox jukectl scan
jukebox_SOURCES = album.cc artist.cc client.cc collection.cc \
		  config.cc main.cc player.cc queue.cc server.cc \
		  track.cc user_sql.cc user_ldap.cc volume.cc ident.cc
jukebox_LDADD	= @LIBPLUSPLUS_LIBS@

jukectl_SOURCES = jukectl.cc
jukectl_LDADD	= @LIBPLUSPLUS_LIBS@

scan_SOURCES	= scan.cc vcedit.c album.cc artist.cc config.cc track.cc
scan_LDADD	= @LIBPLUSPLUS_LIBS@ @LIBS_OGGVORBIS@ @LIBS_ID3@

CFLAGS		= @LIBPLUSPLUS_CFLAGS@ @HAVE_OGGVORBIS@ @HAVE_ID3@ @USERDB@
CXXFLAGS	= @LIBPLUSPLUS_CFLAGS@ @HAVE_OGGVORBIS@ @HAVE_ID3@ @USERDB@

DISTCLEANFILES	= paths.h


paths.h:	Makefile
		echo "#define SYSCONFDIR \"$(sysconfdir)\""  > $@
		echo "#define LOCALEDIR  \"$(localedir)\""  >> $@
		echo "#define PKGDATADIR \"$(pkgdatadir)\"" >> $@
		echo "#define PKGLIBDIR  \"$(pkglibdir)\""  >> $@

main.o:		paths.h


EXTRA_DIST	= album.h artist.h client.h collection.h config.h ident.h \
		  jukebox.h jukectl.h player.h queue.h server.h track.h \
		  user.h user_ldap.h user_sql.h vcedit.h volume.h
