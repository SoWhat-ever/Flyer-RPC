PATH_BIN = bin
PATH_LIB = lib
PATH_OBJ = obj

PATH_FLYER = Flyer
PATH_COMM = $(PATH_FLYER)/common
PATH_NET = $(PATH_FLYER)/net
PATH_TCP = $(PATH_FLYER)/net/tcp
PATH_TINYXML = $(PATH_FLYER)/tinyxml
PATH_CODER = $(PATH_FLYER)/net/coder
PATH_RPC = $(PATH_FLYER)/net/rpc



PATH_TESTCASES = ./testcases

PATH_INSTALL_LIB_ROOT = /usr/lib
PATH_INSTALL_INC_ROOT = /usr/include

PATH_INSTALL_INC_COMM = $(PATH_INSTALL_INC_ROOT)/$(PATH_COMM)
PATH_INSTALL_INC_NET = $(PATH_INSTALL_INC_ROOT)/$(PATH_NET)
PATH_INSTALL_INC_TINYXML = $(PATH_INSTALL_INC_ROOT)/$(PATH_TINYXML)
PATH_INSTALL_INC_TCP = $(PATH_INSTALL_INC_ROOT)/$(PATH_TCP)
PATH_INSTALL_INC_CODER = $(PATH_INSTALL_INC_ROOT)/$(PATH_CODER)
PATH_INSTALL_INC_RPC = $(PATH_INSTALL_INC_ROOT)/$(PATH_RPC)


##################

CXX := g++

CXXFLAGS += -g -O0 -std=c++11 -Wall -Wno-deprecated -Wno-unused-but-set-variable -Wno-format-security

CXXFLAGS += -I./ -I$(PATH_FLYER) -I$(PATH_COMM) -I$(PATH_NET) -I$(PATH_TINYXML) -I$(PATH_TCP) -I$(PATH_CODER)  -I$(PATH_RPC)

LIBS += /usr/local/lib/libprotobuf.a

COMM_OBJ := $(patsubst $(PATH_COMM)/%.cc, $(PATH_OBJ)/%.o, $(wildcard $(PATH_COMM)/*.cc))
NET_OBJ := $(patsubst $(PATH_NET)/%.cc, $(PATH_OBJ)/%.o, $(wildcard $(PATH_NET)/*.cc))
TINYXML_OBJ := $(patsubst $(PATH_TINYXML)/%.cc, $(PATH_OBJ)/%.o, $(wildcard $(PATH_TINYXML)/*.cc))
TCP_OBJ := $(patsubst $(PATH_TCP)/%.cc, $(PATH_OBJ)/%.o, $(wildcard $(PATH_TCP)/*.cc))
CODER_OBJ := $(patsubst $(PATH_CODER)/%.cc, $(PATH_OBJ)/%.o, $(wildcard $(PATH_CODER)/*.cc))
RPC_OBJ := $(patsubst $(PATH_RPC)/%.cc, $(PATH_OBJ)/%.o, $(wildcard $(PATH_RPC)/*.cc))

ALL_TESTS : $(PATH_BIN)/test_log $(PATH_BIN)/test_eventloop $(PATH_BIN)/test_tcp $(PATH_BIN)/test_client $(PATH_BIN)/test_rpc_client $(PATH_BIN)/test_rpc_server $(PATH_BIN)/rpc_server $(PATH_BIN)/rpc_client

TEST_CASE_OUT := $(PATH_BIN)/test_log $(PATH_BIN)/test_eventloop $(PATH_BIN)/test_tcp $(PATH_BIN)/test_client $(PATH_BIN)/test_rpc_client $(PATH_BIN)/test_rpc_server $(PATH_BIN)/rpc_server $(PATH_BIN)/rpc_client

LIB_OUT := $(PATH_LIB)/libflyer.a

$(PATH_BIN)/test_log: $(LIB_OUT)
	$(CXX) $(CXXFLAGS) $(PATH_TESTCASES)/test_log.cc -o $@ $(LIB_OUT) $(LIBS) -ldl -pthread

$(PATH_BIN)/test_eventloop: $(LIB_OUT)
	$(CXX) $(CXXFLAGS) $(PATH_TESTCASES)/test_eventloop.cc -o $@ $(LIB_OUT) $(LIBS) -ldl -pthread

$(PATH_BIN)/test_tcp: $(LIB_OUT)
	$(CXX) $(CXXFLAGS) $(PATH_TESTCASES)/test_tcp.cc -o $@ $(LIB_OUT) $(LIBS) -ldl -pthread

$(PATH_BIN)/test_client: $(LIB_OUT)
	$(CXX) $(CXXFLAGS) $(PATH_TESTCASES)/test_client.cc -o $@ $(LIB_OUT) $(LIBS) -ldl -pthread

$(PATH_BIN)/test_rpc_client: $(LIB_OUT)
	$(CXX) $(CXXFLAGS) $(PATH_TESTCASES)/test_rpc_client.cc $(PATH_TESTCASES)/order.pb.cc -o $@ $(LIB_OUT) $(LIBS) -ldl -pthread

$(PATH_BIN)/test_rpc_server: $(LIB_OUT)
	$(CXX) $(CXXFLAGS) $(PATH_TESTCASES)/test_rpc_server.cc $(PATH_TESTCASES)/order.pb.cc -o $@ $(LIB_OUT) $(LIBS) -ldl -pthread

$(PATH_BIN)/rpc_client: $(LIB_OUT)
	$(CXX) $(CXXFLAGS) $(PATH_TESTCASES)/rpc_client.cc $(PATH_TESTCASES)/order.pb.cc -o $@ $(LIB_OUT) $(LIBS) -ldl -pthread

$(PATH_BIN)/rpc_server: $(LIB_OUT)
	$(CXX) $(CXXFLAGS) $(PATH_TESTCASES)/rpc_server.cc $(PATH_TESTCASES)/order.pb.cc -o $@ $(LIB_OUT) $(LIBS) -ldl -pthread


$(LIB_OUT): $(COMM_OBJ) $(NET_OBJ) $(TINYXML_OBJ) $(TCP_OBJ) $(CODER_OBJ) $(RPC_OBJ)
	cd $(PATH_OBJ) && ar rcv libflyer.a *.o && cp libflyer.a ../lib/

$(PATH_OBJ)/%.o : $(PATH_COMM)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PATH_OBJ)/%.o : $(PATH_NET)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PATH_OBJ)/%.o : $(PATH_TINYXML)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PATH_OBJ)/%.o : $(PATH_TCP)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PATH_OBJ)/%.o : $(PATH_CODER)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PATH_OBJ)/%.o : $(PATH_RPC)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@


# print something test
# like this: make PRINT-PATH_BIN, and then will print variable PATH_BIN
PRINT-% : ; @echo $* = $($*)

# to clean 
clean :
	rm -f $(COMM_OBJ) $(NET_OBJ) $(TINYXML_OBJ) $(TCP_OBJ) $(CODER_OBJ) $(RPC_OBJ) $(TESTCASES) $(TEST_CASE_OUT) $(PATH_LIB)/libflyer.a $(PATH_OBJ)/libflyer.a $(PATH_OBJ)/*.o

# install
install:
	mkdir -p $(PATH_INSTALL_INC_COMM) $(PATH_INSTALL_INC_NET) $(PATH_INSTALL_INC_TINYXML) $(PATH_INSTALL_INC_TCP) $(PATH_INSTALL_INC_CODER) $(PATH_INSTALL_INC_RPC)\
		&& cp $(PATH_COMM)/*.h $(PATH_INSTALL_INC_COMM) \
		&& cp $(PATH_NET)/*.h $(PATH_INSTALL_INC_NET) \
		&& cp $(PATH_TCP)/*.h $(PATH_INSTALL_INC_TCP) \
		&& cp $(PATH_CODER)/*.h $(PATH_INSTALL_INC_CODER) \
		&& cp $(PATH_RPC)/*.h $(PATH_INSTALL_INC_RPC) \
		&& cp $(PATH_TINYXML)/*.h $(PATH_INSTALL_INC_TINYXML) \
		&& cp $(LIB_OUT) $(PATH_INSTALL_LIB_ROOT)/

# uninstall
uninstall:
	rm -rf $(PATH_INSTALL_INC_ROOT)/Flyer && rm -f $(PATH_INSTALL_LIB_ROOT)/libflyer.a
