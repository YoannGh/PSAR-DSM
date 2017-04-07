CC       = gcc
# compiling flags here
CFLAGS   = -Wall -Wextra -std=c99 -D_GNU_SOURCE -DDEBUG

LINKER   = gcc
# linking flags here
LFLAGS   = -lpthread

# change these to proper directories where each file should be
SRCDIR   = src
INCDIR   = src
OBJDIR   = obj
BINDIR   = bin
TESTDIR	 = test

BIN_NAME = dsm

TEST1_NAME = test_dsm_mmap
TEST2_NAME = test_dsm_socket_serv
TEST3_NAME = test_dsm_socket_client
TEST4_NAME = test_dsm_protocol

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(INCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
RM        = rm -f
RMDIR     = rmdir
MKDIR_P   = mkdir -p

all: out_directories tests

$(BINDIR)/$(BIN_NAME): $(OBJDIR)/$(BIN_NAME).o
	$(LINKER) $(LFLAGS) -o $@ $^

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

.PHONY: tests
tests: $(TESTDIR)/$(TEST1_NAME) $(TESTDIR)/$(TEST2_NAME) $(TESTDIR)/$(TEST3_NAME) $(TESTDIR)/$(TEST4_NAME)

$(TESTDIR)/$(TEST1_NAME): $(OBJDIR)/dsm.o $(OBJDIR)/test_dsm_mmap.o
	$(LINKER) $(LFLAGS) -o $@ $^

$(TESTDIR)/$(TEST2_NAME): $(OBJDIR)/binn.o $(OBJDIR)/dsm_socket.o $(OBJDIR)/dsm_protocol.o $(OBJDIR)/test_dsm_socket_serv.o
	$(LINKER) $(LFLAGS) -o $@ $^

$(TESTDIR)/$(TEST3_NAME): $(OBJDIR)/binn.o $(OBJDIR)/dsm_socket.o $(OBJDIR)/dsm_protocol.o $(OBJDIR)/test_dsm_socket_client.o
	$(LINKER) $(LFLAGS) -o $@ $^

$(TESTDIR)/$(TEST4_NAME): $(OBJDIR)/binn.o $(OBJDIR)/dsm.o $(OBJDIR)/dsm_protocol.o $(OBJDIR)/dsm_socket.o $(OBJDIR)/test_dsm_protocol.o
	$(LINKER) $(LFLAGS) -o $@ $^

.PHONY: out_directories
out_directories:
	@$(MKDIR_P) $(OBJDIR) $(TESTDIR) $(BINDIR)

.PHONY: clean
clean:
	@$(RM)		$(OBJECTS)
	@$(RMDIR)	$(OBJDIR)

.PHONY: remove
remove: clean
	@$(RM)		$(BINDIR)/$(BIN_NAME)
	@$(RMDIR)	$(BINDIR)
	@$(RM)		$(TESTDIR)/$(TEST1_NAME)
	@$(RM)		$(TESTDIR)/$(TEST2_NAME)
	@$(RM) 		$(TESTDIR)/$(TEST3_NAME)
	@$(RM) 		$(TESTDIR)/$(TEST4_NAME)
	@$(RMDIR)	$(TESTDIR)