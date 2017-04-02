CC       = gcc
# compiling flags here
CFLAGS   = -Wall -Wextra -std=c99 -D_GNU_SOURCE

LINKER   = gcc
# linking flags here
LFLAGS   = 

# change these to proper directories where each file should be
SRCDIR   = src
INCDIR   = src
OBJDIR   = obj
BINDIR   = bin

BIN_NAME = test_dsm

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(INCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
RM        = rm -f
MKDIR_P   = mkdir -p

all: out_directories $(BINDIR)/$(BIN_NAME) 

$(BINDIR)/$(BIN_NAME): $(OBJECTS) #$(OBJDIR)/$(BIN_NAME).o
	$(LINKER) $(LFLAGS) -o $@ $^

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

.PHONY: directories
out_directories:
	$(MKDIR_P) $(OBJDIR) $(BINDIR)

.PHONY: clean
clean:
	@$(RM) $(OBJECTS)

.PHONY: remove
remove: clean
	@$(RM) $(BINDIR)/$(BIN_NAME)
