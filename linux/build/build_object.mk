#########################################################
#
#	Telechips Linux Application reset variable
#
#########################################################

local_s_sources := $(filter %.s, $(LOCAL_SRC_FILES))
local_s_objects := $(local_s_sources:.s=.o)

local_c_sources := $(filter %.c, $(LOCAL_SRC_FILES))
local_c_objects := $(local_c_sources:.c=.o)

local_cpp_sources := $(filter %.cpp, $(LOCAL_SRC_FILES))
local_cpp_objects := $(local_cpp_sources:.cpp=.o)

LOCAL_OBJECTS = $(local_s_objects) $(local_c_objects) $(local_cpp_objects)

.s.o:
	$(CC) $(foreach incdir,$(LOCAL_INCLUDE_HEADERS),-I $(incdir)) $(CFLAGS) $(LOCAL_CFLAGS) -c $(INCLUDE) -o $@ $<

.c.o:
	$(CC) $(foreach incdir,$(LOCAL_INCLUDE_HEADERS),-I $(incdir)) $(CFLAGS) $(LOCAL_CFLAGS) -c $(INCLUDE) -o $@ $<

.cpp.o:
	$(CXX) $(foreach incdir,$(LOCAL_INCLUDE_HEADERS),-I $(incdir)) $(CXXFLAGS) $(LOCAL_CXXFLAGS) -c $(INCLUDE) -o $@ $<		
