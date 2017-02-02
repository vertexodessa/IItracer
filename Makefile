
INSTR = \
-finstrument-functions \
-finstrument-functions-exclude-file-list=/usr/  \
-finstrument-functions-exclude-function-list=static_initialization_and_destruction

# FIXME: naming!
LIBS = -Wl,-lunwind -Wl,-lwtf -Wl,-rpath=/usr/local/lib/
TEST_LIBS += -pthread -L. -Wl,-lunwind_wtf

CXXFLAGS += -g2 -std=c++11 

all: clean libunwind_wtf.so test install

clean:
	rm -f test libunwind_wtf.so

libunwind_wtf.so: libunwind_wtf.cc
	$(CXX) -shared -fPIC $(CXXFLAGS) $(INSTR) libunwind_wtf.cc $(LDFLAGS) $(LIBS) -o libunwind_wtf.so

test: libunwind_wtf.so test.cc
	$(CXX) $(CXXFLAGS) $(INSTR) -o test test.cc  $(LDFLAGS) $(TEST_LIBS) -Wl,-rpath=`pwd`

install: test
	./test
	scp *wtf-trace home:/tmp/
