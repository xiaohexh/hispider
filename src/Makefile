CC	= gcc
CXX	= g++

INC = .
LIB = -lpthread -levent
CFLAGS = -g -Wall -O2 -pipe -fno-ident -MMD -D_GNU_SOURCE -D_REENTRANT -Wno-deprecated
#CFLAGS = -g -Wall -pipe -fno-ident -MMD -D_GNU_SOURCE -D_REENTRANT -Wno-deprecated
CXXFLAGS += $(CFLAGS)

OBJ = hispider.o	\
	  hs_log.o		\
	  hs_config.o	\
	  hs_signal.o	\
	  hs_thread.o	\
	  hs_event.o	\
	  hs_net.o		\
	  hs_urlquery.o	\
	  hs_urlparser.o

TARGET = hispider 

all	: $(TARGET)

$(TARGET) : $(OBJ)
	@echo
	@echo "========== Building $@ =========="
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIB)

clean:
	@echo
	@echo "cleaning ..."
	rm -rf $(TARGET) *.o *.d *~ *.orig
