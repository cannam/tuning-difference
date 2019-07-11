
# Edit this to the base name of your plugin library
#
PLUGIN_LIBRARY_NAME := tuning-difference

# Edit this to list the .cpp or .c files in your plugin project
#
PLUGIN_SOURCES := src/TuningDifference.cpp src/plugins.cpp

# Edit this to list the .h files in your plugin project
#
PLUGIN_HEADERS := src/TuningDifference.h


##  Normally you should not edit anything below this line

SRC_DIR		:= .

CFLAGS		:= $(ARCHFLAGS) $(CFLAGS)
CXXFLAGS	:= $(CFLAGS) -I. -I$(VAMPSDK_DIR) -Iconstant-q-cpp $(CXXFLAGS)

LDFLAGS		:= $(ARCHFLAGS) $(LDFLAGS) -Lconstant-q-cpp -lcq
PLUGIN_LDFLAGS	:= $(LDFLAGS) $(PLUGIN_LDFLAGS)

# Defaults, overridden from the platform-specific Makefile
VAMPSDK_DIR	?= ../vamp-plugin-sdk
PLUGIN_EXT	?= .so
CXX 		?= g++
CC 		?= gcc

PLUGIN 		:= $(PLUGIN_LIBRARY_NAME)$(PLUGIN_EXT)

PLUGIN_OBJECTS 	:= $(PLUGIN_SOURCES:.cpp=.o)
PLUGIN_OBJECTS 	:= $(PLUGIN_OBJECTS:.c=.o)

all: constant-q-cpp $(PLUGIN)

.PHONY: constant-q-cpp
constant-q-cpp: 
	$(MAKE) -C $@ -f Makefile$(MAKEFILE_EXT) libcq.a

$(PLUGIN): $(PLUGIN_OBJECTS) 
	   $(CXX) -o $@ $^ $(PLUGIN_LDFLAGS)

$(PLUGIN_OBJECTS): $(PLUGIN_HEADERS)

test:	$(PLUGIN)
	bash test/regression.sh

clean:
	rm -f $(PLUGIN_OBJECTS)
	$(MAKE) -C constant-q-cpp -f Makefile$(MAKEFILE_EXT) clean

distclean:	clean
	rm -f $(PLUGIN)

depend:
	makedepend -Y -fMakefile.inc $(PLUGIN_SOURCES) $(PLUGIN_HEADERS)

# DO NOT DELETE

src/TuningDifference.o: src/TuningDifference.h
src/plugins.o: src/TuningDifference.h
