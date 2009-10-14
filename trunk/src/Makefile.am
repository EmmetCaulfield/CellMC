SHARED=@top_srcdir@/data/include

bin_PROGRAMS = cellmc

cellmc_SOURCES  = autodirs.h cellmc.c runconfig.h runconfig.c platspec.c 
cellmc_SOURCES += platspec.h xml-utils.h xml-utils.c utils.h utils.c appconfig.h
cellmc_SOURCES += compile.h compile.c helptext.h helptext.c
cellmc_SOURCES += $(SHARED)/cellmc.h $(SHARED)/error-macros.h
cellmc_SOURCES += $(SHARED)/option-mapping.h $(SHARED)/option-mapping.c

cellmc_LDFLAGS  = $(libxslt_LIBS)

cellmc_CFLAGS =	$(libXML_CFLAGS)	\
	$(libxslt_CFLAGS)		\
	$(WARN_CFLAGS)

cellmc_CPPFLAGS = -I@top_srcdir@/data/include

nobase_dist_pkgdata_DATA  = test/test.sh
nobase_dist_pkgdata_DATA += test/Makefile

cellmc.c: autodirs.h

purge: clean
	rm -f core core.* autodirs.h *~ *.[aois] cellm??????.c
	rm -f *.out *.stdout
	rm -f {hsr,me,cr,dd,ddm,bd,vilar}{,p}{,.xsl,.xml}
	rm -f {hsr,me,cr,dd,ddm,bd,vilar}{,p}-new{,.c,.out,.xml}

DISTCLEANFILES=autodirs.h
autodirs.h: Makefile
	@echo '#define DATADIR "$(pkgdatadir)"' > $@
	@echo '#define SRCDIR  "@srcdir@"' >> $@
	@echo '#define TOPDIR  "@abs_top_srcdir@"' >> $@
