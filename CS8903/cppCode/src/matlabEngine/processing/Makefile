#####################
# TODO WARNING !!!!!
# Run the following command before compiling: LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/MATLAB/R2016a/bin/glnxa64
# Run the following command before running: export PATH=$PATH:/usr/local/MATLAB/R2016a/bin
#####################

MATLABROOT := /usr/local/MATLAB/R2016a/
SRC:= processing.cpp
OBJ:= $(patsubst %.cpp,%.o, $(SRC))
CXX:= g++
CXXFLAGS:= -I$(MATLABROOT)extern/include
LDLIBS:= -L$(MATLABROOT)extern/lib -L$(MATLABROOT)bin/glnxa64 -leng -lmx

.PHONY: clean

all: processing

processing:
	$(CXX) $(SRC) -o processing $(CXXFLAGS) $(LDLIBS) 

clean:
	$(RM) processing *~ *.o
