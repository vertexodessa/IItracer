
INSTR = \
-finstrument-functions \
-finstrument-functions-exclude-file-list=/usr/  \
-finstrument-functions-exclude-function-list=static_initialization_and_destruction

LIBS = -Wl,-lunwind -Wl,-lwtf -Wl,-rpath=/usr/local/lib/
TEST_LIBS += -pthread -L. -Wl,-lunwind_wtf

all: clean libunwind_wtf.so test install

clean:
	rm -f test libunwind_wtf.so

libunwind_wtf.so: libunwind_wtf.cc
	g++ -std=c++11 -g2 -fPIC $(INSTR) libunwind_wtf.cc $(LIBS) -shared -o libunwind_wtf.so

test: libunwind_wtf.so test.cc
	g++ -std=c++11 -g2 -o test test.cc $(INSTR) $(TEST_LIBS) -Wl,-rpath=`pwd`

install: test
	./test
	scp *wtf-trace home:/tmp/
