# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

# The default target runs when you just type 'make'
all: server client

# Compile the server
server: server.cpp
	$(CXX) $(CXXFLAGS) server.cpp -o server

# Compile the client
client: client.cpp
	$(CXX) $(CXXFLAGS) client.cpp -o client

# Clean up the compiled binaries
clean:
	rm -f server client

# Mark targets that don't represent physical files as 'phony'
.PHONY: all clean