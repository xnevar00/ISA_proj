CXX = g++    # C++ kompilátor
CXXFLAGS = -std=c++11 -Wall -Wextra   # Přepínače kompilátoru
LDFLAGS =    # Přepínače pro linkování
SRC_CLIENT = tftp-client.cpp
SRC_SERVER = tftp-server.cpp

all: tftp-client tftp-server

tftp-client: $(SRC_CLIENT)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC_CLIENT) $(LDFLAGS)

tftp-server: $(SRC_SERVER)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC_SERVER) $(LDFLAGS)

clean:
	rm -f tftp-client tftp-server