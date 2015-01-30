AC_PREREQ([2.63])
AC_INIT([mate-applet-livescore], [0.3.0], [assen.totin@gmail.com])
AC_CONFIG_HEADERS([config.h])

# AM_PROG_AR is not available on RHEL, so allow it
m4_pattern_allow([AM_PROG_AR])
AM_PROG_AR
LT_INIT

AM_INIT_AUTOMAKE([-Wall])

# prefix
if test "x$prefix" = "xNONE" ; then
	prefix="/usr/local"
fi
if test "x$exec_prefix" = "xNONE" ; then
	exec_prefix=$prefix
fi
PREFIX=$prefix
AC_SUBST([PREFIX])
BINDIR=$exec_prefix/bin
AC_SUBST([BINDIR])
SBINDIR=$exec_prefix/sbin
AC_SUBST([SBINDIR])
DATAROOTDIR=$prefix/share
AC_SUBST([DATAROOTDIR])
LIBEXECDIR=$exec_prefix/libexec
if test "$libexecdir" != '${exec_prefix}/libexec'; then
        LIBEXECDIR=$libexecdir
fi
AC_SUBST([LIBEXECDIR])
LIBDIR=${exec_prefix}/lib
if test -d /usr/lib64 ; then
        LIBDIR=${exec_prefix}/lib64
fi
if test "$libdir" != '${exec_prefix}/lib'; then
        if test "$libdir" != '${exec_prefix}/lib64'; then
                LIBDIR=$libdir
        fi
fi
AC_DEFINE_UNQUOTED([LIBDIR], ["$LIBDIR"], ["Directory where application-specific libraries are installed"])
PIXMAPDIR=$prefix/share/pixmaps
AC_SUBST([PIXMAPDIR])
ICONSDIR=$prefix/share/icons
AC_SUBST([ICONSDIR])
LOCALEDIR=$prefix/share/locale
AC_SUBST([LOCALEDIR])
AC_DEFINE_UNQUOTED([LOCALEDIR],["$LOCALEDIR"],[Locale files])
TOPLEVELDIR=`pwd`
AC_SUBST([TOPLEVELDIR])

# gettext
AM_GNU_GETTEXT_VERSION(0.18)
AM_GNU_GETTEXT([external])
if test $USE_NLS != "yes" ; then
        echo "*** gettext not found. You're losing internationalization."
fi
AC_DEFINE([GETTEXT_PACKAGE], ["gtk20"], ["GTK gettext"])
AC_DEFINE_UNQUOTED([MATELOCALEDIR], ["$LOCALEDIR"], ["Locale Dir MATE"])

# Checks for programs
AC_PROG_CC
MAKEFILE_CC=`which gcc`
AC_SUBST([MAKEFILE_CC])

# Check for packages
PKG_CHECK_MODULES([GTK], [gtk+-2.0])

PKG_CHECK_MODULES([GLIB], [glib-2.0])

RPM_BUILDREQUIRES_PANEL=""
RPM_REQUIRES_PANEL=""
HAVE_MATE="0"
HAVE_GNOME_2="0"
PKG_CHECK_MODULES([LIBMATEPANELAPPLET_4], [libmatepanelapplet-4.0], [FOUND_LMPA_4="yes"], [FOUND_LMPA_4="no"])
PKG_CHECK_MODULES([LIBMATEPANELAPPLET_3], [libmatepanelapplet-3.0], [FOUND_LMPA_3="yes"], [FOUND_LMPA_3="no"])
PKG_CHECK_MODULES([LIBPANELAPPLET_2], [libpanelapplet-2.0], [FOUND_LPA_2="yes"], [FOUND_LPA_2="no"])
if test $FOUND_LMPA_4 = "yes" ; then
        LIBMATEPANELAPPLET="libmatepanelapplet-4.0"
        HAVE_MATE="1"
        AC_DEFINE([HAVE_MATE], [1], [Define if we have MATE Desktop Environment])
	AC_SUBST([HAVE_MATE])
        RPM_BUILDREQUIRES_PANEL="BuildRequires: mate-panel-devel >= 1.3.0"
        RPM_REQUIRES_PANEL="Requires: mate-panel >= 1.3.0"
elif test $FOUND_LMPA_3 = "yes" ; then
        LIBMATEPANELAPPLET="libmatepanelapplet-3.0"
        HAVE_MATE="1"
        AC_DEFINE([HAVE_MATE], [1], [Define if we have MATE Desktop Environment])
	AC_SUBST([HAVE_MATE])
        RPM_BUILDREQUIRES_PANEL="BuildRequires: mate-panel-devel >= 1.3.0"
        RPM_REQUIRES_PANEL="Requires: mate-panel >= 1.3.0"
elif test $FOUND_LPA_2 = "yes" ; then
        LIBMATEPANELAPPLET="libpanelapplet-2.0"
        HAVE_GNOME_2="1"
        AC_DEFINE([HAVE_GNOME_2], [1], [Define if we have GNOME-2 Desktop Environment])
        AC_DEFINE_UNQUOTED([GNOMELOCALEDIR], ["$LOCALEDIR"], [GNOME-2 specific locale dir variable])
        RPM_BUILDREQUIRES_PANEL="BuildRequires: gnome-panel-devel >= 2.30.0"
        RPM_REQUIRES_PANEL="Requires: gnome-panel >= 2.30.0"
else
        AC_MSG_ERROR(["*** libmatepanelapplet not found."])
fi
AC_SUBST([RPM_REQUIRES_PANEL])
AC_SUBST([RPM_BUILDREQUIRES_PANEL])

# libmatenotify or libnotify?
RPM_BUILDREQUIRES_NOTIFY=""
RPM_REQUIRES_NOTIFY=""
FORCE_NOTIFY="no"
AC_ARG_ENABLE([notify],[AS_HELP_STRING([--enable-notify],[use the specified notificatin library: libnotify, libmatenotify (default: pick up the first available from the list)])],[FORCE_NOTIFY=$enableval])
PKG_CHECK_MODULES([LIBMATENOTIFY], [libmatenotify], [FOUND_LMN="yes"], [FOUND_LMN="no"])
PKG_CHECK_MODULES([LIBNOTIFY], [libnotify], [FOUND_LN="yes"], [FOUND_LN="no"])
if test $FORCE_NOTIFY = "libmatenotify" ; then
        if test $FOUND_LMN = "yes" ; then
                SELECTED_NOTIFY="libmatenotify"
		AC_DEFINE([HAVE_LIBMATENOTIFY],[1],[Define if libmatenotify was found])
		RPM_BUILDREQUIRES_NOTIFY="BuildRequires: libmatenotify-devel >= 1.2.0"
		RPM_REQUIRES_NOTIFY="Requires: libmatenotify >= 1.2.0"
        else
                AC_MSG_ERROR(["Forced notification library $FORCE_NOTIFY not found. Cannot continue."])
        fi
elif test $FORCE_NOTIFY = "libnotify" ; then
        if test $FOUND_LN = "yes" ; then
                SELECTED_NOTIFY="libnotify"
		AC_DEFINE([HAVE_LIBNOTIFY],[1],[Define if libnotify was found])
		if test $HAVE_MATE = "1" ; then
	                RPM_BUILDREQUIRES_NOTIFY="BuildRequires: libnotify-devel >= 0.7.0"
        	        RPM_REQUIRES_NOTIFY="Requires: libnotify >= 0.7.0"
		elif test $HAVE_GNOME_2 = "1" ; then
			RPM_BUILDREQUIRES_NOTIFY="BuildRequires: libnotify-devel >= 0.5.0"
			RPM_REQUIRES_NOTIFY="Requires: libnotify >= 0.5.0"
		fi
        else
		AC_MSG_ERROR(["Forced notification library $FORCE_NOTIFY not found. Cannot continue."])
        fi
else
	if test $FOUND_LN = "yes" ; then
                SELECTED_NOTIFY="libnotify"
		AC_DEFINE([HAVE_LIBNOTIFY],[1],[Define if libnotify was found])	
		if test $HAVE_MATE = "1" ; then
	                RPM_BUILDREQUIRES_NOTIFY="BuildRequires: libnotify-devel >= 0.7.0"
        	        RPM_REQUIRES_NOTIFY="Requires: libnotify >= 0.7.0"
		elif test $HAVE_GNOME_2 = "1" ; then
			RPM_BUILDREQUIRES_NOTIFY="BuildRequires: libnotify-devel >= 0.5.0"
			RPM_REQUIRES_NOTIFY="Requires: libnotify >= 0.5.0"
		fi
	elif test $FOUND_LMN = "yes" ; then
		SELECTED_NOTIFY="libmatenotify"
		AC_DEFINE([HAVE_LIBMATENOTIFY],[1],[Define if libmatenotify was found])
                RPM_BUILDREQUIRES_NOTIFY="BuildRequires: libmatenotify-devel >= 1.2.0"
                RPM_REQUIRES_NOTIFY="Requires: libmatenotify >= 1.2.0"
	else
		AC_MSG_ERROR(["Neither libnotify nor libmatenotify found. Cannot continue."])
	fi
fi
AC_SUBST([RPM_BUILDREQUIRES_NOTIFY])
AC_SUBST([RPM_REQUIRES_NOTIFY])

# Checks for libraries.
PKG_CHECK_MODULES([LIBXML2], [libxml-2.0])
PKG_CHECK_MODULES([LIBSOUP], [libsoup-2.4])

APPLET_FLAGS=`$PKG_CONFIG --cflags-only-I glib-2.0 libxml-2.0 libsoup-2.4 $SELECTED_NOTIFY $LIBMATEPANELAPPLET`
APPLET_LIBS=`$PKG_CONFIG --libs glib-2.0 libxml-2.0 libsoup-2.4 $SELECTED_NOTIFY $LIBMATEPANELAPPLET`
AC_SUBST([APPLET_FLAGS])
AC_SUBST([APPLET_LIBS])

# Makefiles
AC_CONFIG_FILES([Makefile src/Makefile feed-enetpulse/Makefile feed-iddaa/Makefile feed-omnibet/Makefile po/Makefile.in po/Makefile images/Makefile])

# Service files
AC_CONFIG_FILES([src/org.mate.applets.LivescoreApplet.mate-panel-applet src/org.mate.panel.applet.LivescoreApplet.service src/org.mate.panel.applet.LivescoreApplet.gschema.xml src/LivescoreAppletFactory.server mate-applet-livescore.spec])

# Icons
AC_DEFINE_UNQUOTED([APPLET_ICON_PATH], ["$PIXMAPDIR"], [Define icons location])

# Grand finale
AC_OUTPUT

echo
echo "Configuration complete:"
echo " * Selected notification library: $SELECTED_NOTIFY"
echo
echo "Now, run make."
echo 
