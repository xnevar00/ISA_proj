CXX = g++    # C++ kompilátor
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude  # Přepínače kompilátoru
LDFLAGS =    # Přepínače pro linkování
SRC_CLIENT = src/client/tftp-client.cpp src/client/client_class.cpp  # Přidáme client_class.cpp
SRC_SERVER = src/server/tftp-server.cpp src/server/server_class.cpp  # Přidáme server_class.cpp
HEADERS_SERVER = include/server/server_class.hpp  # Přidáme server_class.hpp
HEADERS_CLIENT = include/client/client_class.hpp
SRC_HELPER_FUNCTIONS = helper_functions.cpp
HEADERS_HELPER_FUNCTIONS = helper_functions.hpp
SRC_TFTP_PACKET = src/packet/tftp-packet-class.cpp
HEADERS_TFTP_PACKET = include/packet/tftp-packet-class.hpp

all: tftp-client tftp-server

tftp-client: $(SRC_CLIENT) $(HEADERS_CLIENT) $(HEADERS_HELPER_FUNCTIONS) $(HEADERS_TFTP_PACKET)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC_CLIENT) $(SRC_HELPER_FUNCTIONS) $(SRC_TFTP_PACKET) $(LDFLAGS)

tftp-server: $(SRC_SERVER) $(HEADERS_SERVER) $(HEADERS_HELPER_FUNCTIONS) $(HEADERS_TFTP_PACKET)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC_SERVER) $(SRC_HELPER_FUNCTIONS) $(SRC_TFTP_PACKET) $(LDFLAGS)

helper-functions: $(SRC_HELPER_FUNCTIONS) $(HEADERS_HELPER_FUNCTIONS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC_HELPER_FUNCTIONS) $(LDFLAGS)

tftp-packet: $(SRC_TFTP_PACKET) $(HEADERS_TFTP_PACKET)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC_TFTP_PACKET) $(LDFLAGS)

clean:
	rm -f tftp-client tftp-server