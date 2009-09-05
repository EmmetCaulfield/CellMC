# If you are having problems with old versions of libxml2 
# or libxslt that predate 'pkg-config', source this file
# into the shell before running configure.
libXML_LIBS=$(xml2-config --libs)
libXML_CFLAGS=$(xml2-config --cflags)
libxslt_LIBS=$(xslt-config --libs)
libxslt_CFLAGS=$(xslt-config --cflags)

export libXML_LIBS libXML_CFLAGS
export libxslt_LIBS libxslt_CFLAGS