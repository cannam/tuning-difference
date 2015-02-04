
# Edit this to the base name of your plugin library
#
PLUGIN_LIBRARY_NAME := tuning-difference

# Edit this to list the .cpp or .c files in your plugin project
#
PLUGIN_SOURCES := TuningDifference.cpp plugins.cpp

# Edit this to list the .h files in your plugin project
#
PLUGIN_HEADERS := TuningDifference.h


##  Normally you should not edit anything below this line

SRC_DIR		:= .

CFLAGS		:= $(ARCHFLAGS) $(CFLAGS)
CXXFLAGS	:= $(CFLAGS) -I. -I$(VAMPSDK_DIR) -I../../constant-q-cpp $(CXXFLAGS)

LDFLAGS		:= $(ARCHFLAGS) $(LDFLAGS) -L../../constant-q-cpp -lcq
PLUGIN_LDFLAGS	:= $(LDFLAGS) $(PLUGIN_LDFLAGS)

# Defaults, overridden from the platform-specific Makefile
VAMPSDK_DIR	?= ../vamp-plugin-sdk
PLUGIN_EXT	?= .so
CXX 		?= g++
CC 		?= gcc

PLUGIN 		:= $(PLUGIN_LIBRARY_NAME)$(PLUGIN_EXT)

PLUGIN_OBJECTS 	:= $(PLUGIN_SOURCES:.cpp=.o)
PLUGIN_OBJECTS 	:= $(PLUGIN_OBJECTS:.c=.o)

$(PLUGIN): $(PLUGIN_OBJECTS) 
	   $(CXX) -o $@ $^ $(PLUGIN_LDFLAGS)

$(PLUGIN_OBJECTS): $(PLUGIN_HEADERS)

clean:
	rm -f $(PLUGIN_OBJECTS)

distclean:	clean
	rm -f $(PLUGIN)

depend:
	makedepend -Y -fMakefile.inc $(PLUGIN_SOURCES) $(PLUGIN_HEADERS)

# DO NOT DELETE

TuningDifference.o: TuningDifference.h
plugins.o: TuningDifference.h
