# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I$(INCDIR) -g

# Directories
SRCDIR=src
INCDIR=inc
EXEDIR=bin
OBJDIR=obj

CLIENT=bin/client
SERVER=bin/server

# Source and test files
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(SRCS:.cpp=.o)

all: $(CLIENT) $(SERVER) $(TEST_TARGET)

$(CLIENT): $(OBJDIR)/client.o $(OBJDIR)/common.o $(OBJDIR)/file_sys_util.o $(OBJDIR)/sockets_util.o | $(EXEDIR)
	$(CXX) $(CXXFLAGS) -o $(@) $(^)

$(SERVER): $(OBJDIR)/server.o $(OBJDIR)/common.o $(OBJDIR)/file_sys_util.o $(OBJDIR)/sockets_util.o | $(EXEDIR)
	$(CXX) $(CXXFLAGS) -o $(@) $(^)

# create .o file from .cpp files in src directory
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# create obj directory
$(OBJDIR):
	mkdir $@

# create bin directory
$(EXEDIR):
	mkdir $@

clean:
	rm obj/*
	rm bin/*

.PHONY: all clean