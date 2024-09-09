# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I$(INCDIR) -g

# Directories
SRCDIR=src
INCDIR=inc
EXEDIR=bin
OBJDIR=obj

COMP1=bin/comp1
COMP2=bin/comp2

# Source and test files
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(SRCS:.cpp=.o)

all: $(COMP1) $(COMP2)

$(COMP1): $(OBJDIR)/comp.o $(OBJDIR)/comp1.o $(OBJDIR)/common.o $(OBJDIR)/file_sys_util.o $(OBJDIR)/sockets_util.o $(OBJDIR)/comp1_main.o | $(EXEDIR)
	$(CXX) $(CXXFLAGS) -o $(@) $(^)

$(COMP2): $(OBJDIR)/comp.o $(OBJDIR)/comp2.o $(OBJDIR)/common.o $(OBJDIR)/file_sys_util.o $(OBJDIR)/sockets_util.o $(OBJDIR)/comp2_main.o | $(EXEDIR)
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