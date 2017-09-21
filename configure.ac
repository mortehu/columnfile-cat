AC_INIT(columnfile-cat,1.1.0)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

PKG_CHECK_MODULES([CAS], [ca-cas >= 1.2.0])
PKG_CHECK_MODULES([CAPNP_RPC], [capnp-rpc])
PKG_CHECK_MODULES([COLUMNFILE], [libcolumnfile >= 1.2.0])

AC_OUTPUT(Makefile)
