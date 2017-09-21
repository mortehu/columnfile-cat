bin_PROGRAMS = columnfile-cat

AM_CXXFLAGS = -std=c++17
AM_CPPFLAGS = $(CAPNP_RPC_CFLAGS) $(CAS_CFLAGS) $(COLUMNFILE_CFLAGS)

columnfile_cat_SOURCES = columnfile-cat.cc
columnfile_cat_LDFLAGS = $(CAPNP_RPC_LIBS) $(CAS_LIBS) -lca-cas-columnfile $(COLUMNFILE_LIBS)
