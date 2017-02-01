

INSTR = -finstrument-functions -finstrument-functions-exclude-file-list=/usr/  -finstrument-functions-exclude-function-list=static_initialization_and_destruction
LIBS = -Wl,-lunwind -Wl,-ldl -Wl,-rpath=/usr/local/lib/ -Wl,-lwtf

all: libunwind_wtf.cc
	g++ -std=c++11 -g2  $(INSTR) libunwind_wtf.cc $(LIBS)  && ./a.out
