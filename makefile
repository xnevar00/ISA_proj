CXX = g++    # C++ kompilátor
CXXFLAGS = -std=c++11 -Wall -Wextra -Iinclude  # Přepínače kompilátoru
LDFLAGS =    # Přepínače pro linkování
SRC_CLIENT = src/client/tftp-client.cpp src/client/client_class.cpp  # Přidáme client_class.cpp
SRC_SERVER = src/server/tftp-server.cpp src/server/server_class.cpp  # Přidáme server_class.cpp
HEADERS_SERVER = include/server/server_class.hpp  # Přidáme server_class.hpp
HEADERS_CLIENT = include/client/client_class.hpp
SRC_HELPER_FUNCTIONS = helper_functions.cpp
HEADERS_HELPER_FUNCTIONS = helper_functions.hpp

all: tftp-client tftp-server

tftp-client: $(SRC_CLIENT) $(HEADERS_CLIENT) $(HEADERS_HELPER_FUNCTIONS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC_CLIENT) $(SRC_HELPER_FUNCTIONS) $(LDFLAGS)

tftp-server: $(SRC_SERVER) $(HEADERS_SERVER) $(HEADERS_HELPER_FUNCTIONS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC_SERVER) $(SRC_HELPER_FUNCTIONS) $(LDFLAGS)

helper_functions: $(SRC_HELPER_FUNCTIONS) $(HEADERS_HELPER_FUNCTIONS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC_HELPER_FUNCTIONS) $(LDFLAGS)

clean:
	rm -f tftp-client tftp-server