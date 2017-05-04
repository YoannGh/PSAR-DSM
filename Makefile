CC       = gcc
LINKER   = gcc
AR       = ar

# change these to proper directories where each file should be
SRCDIR   = src
INCDIR   = src
OBJDIR   = obj
LIBDIR   = lib
TESTDIR	 = tests

LIB_NAME = dsm-psar

# compiling flags here
CFLAGS   = -Wall -Wextra -Wno-unused-parameter -std=c99 -fPIC -D_GNU_SOURCE
# linking flags here
LFLAGS   = -std=c99 -I$(INCDIR) -L$(LIBDIR) -lpthread
ARFLAGS  = -cvq
SOFLAGS  = -shared -Wl,-soname,lib$(LIB_NAME).so

TEST1_NAME = test_dsm_init_master
TEST2_NAME = test_dsm_init_slave
TEST3_NAME = test_dsm_lock_write
TEST4_NAME = test_dsm_lock_read
TEST5_NAME = test_dsm_lock_read2

DEMO1_NAME = demo_master_writer
DEMO2_NAME = demo_slave_writer_reader
DEMO3_NAME = demo_slave_reader

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(INCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
RM        = rm -rf
MKDIR_P   = mkdir -p

all: out_directories lib tests demo

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

.PHONY: lib
lib: out_directories libstatic libdynamic

.PHONY: libstatic
libstatic: out_directories $(LIBDIR)/lib$(LIB_NAME).a

.PHONY: libdynamic
libdynamic: out_directories $(LIBDIR)/lib$(LIB_NAME).so

$(LIBDIR)/lib$(LIB_NAME).a: $(OBJDIR)/binn.o $(OBJDIR)/dsm_socket.o $(OBJDIR)/dsm_protocol.o $(OBJDIR)/dsm_memory.o $(OBJDIR)/dsm_master.o $(OBJDIR)/dsm.o $(OBJDIR)/list.o $(OBJDIR)/dsm_core.o
	$(AR) $(ARFLAGS) $@ $^

$(LIBDIR)/lib$(LIB_NAME).so: $(OBJDIR)/binn.o $(OBJDIR)/dsm_socket.o $(OBJDIR)/dsm_protocol.o $(OBJDIR)/dsm_memory.o $(OBJDIR)/dsm_master.o $(OBJDIR)/dsm.o $(OBJDIR)/list.o $(OBJDIR)/dsm_core.o
	$(LINKER) $(SOFLAGS) -o $@ $^

.PHONY: tests
tests: out_directories libstatic $(TESTDIR)/$(TEST1_NAME) $(TESTDIR)/$(TEST2_NAME) $(TESTDIR)/$(TEST3_NAME) $(TESTDIR)/$(TEST4_NAME) $(TESTDIR)/$(TEST5_NAME)

.PHONY: demo
demo: out_directories libstatic $(TESTDIR)/$(DEMO1_NAME) $(TESTDIR)/$(DEMO2_NAME) $(TESTDIR)/$(DEMO3_NAME)

$(TESTDIR)/$(TEST1_NAME): $(SRCDIR)/test_dsm_init_master.c
	$(LINKER) -o $@ $(LFLAGS) $^ $(LIBDIR)/lib$(LIB_NAME).a

$(TESTDIR)/$(TEST2_NAME): $(SRCDIR)/test_dsm_init_slave.c
	$(LINKER) -o $@ $(LFLAGS) $^ $(LIBDIR)/lib$(LIB_NAME).a

$(TESTDIR)/$(TEST3_NAME): $(SRCDIR)/test_dsm_lock_write.c
	$(LINKER) -o $@ $(LFLAGS) $^ $(LIBDIR)/lib$(LIB_NAME).a

$(TESTDIR)/$(TEST4_NAME): $(SRCDIR)/test_dsm_lock_read.c
	$(LINKER) -o $@ $(LFLAGS) $^ $(LIBDIR)/lib$(LIB_NAME).a

$(TESTDIR)/$(TEST5_NAME): $(SRCDIR)/test_dsm_lock_read2.c
	$(LINKER) -o $@ $(LFLAGS) $^ $(LIBDIR)/lib$(LIB_NAME).a


$(TESTDIR)/$(DEMO1_NAME): $(SRCDIR)/demo_master_writer.c
	$(LINKER) -o $@ $(LFLAGS) $^ $(LIBDIR)/lib$(LIB_NAME).a

$(TESTDIR)/$(DEMO2_NAME): $(SRCDIR)/demo_slave_writer_reader.c
	$(LINKER) -o $@ $(LFLAGS) $^ $(LIBDIR)/lib$(LIB_NAME).a

$(TESTDIR)/$(DEMO3_NAME): $(SRCDIR)/demo_slave_reader.c
	$(LINKER) -o $@ $(LFLAGS) $^ $(LIBDIR)/lib$(LIB_NAME).a

.PHONY: out_directories
out_directories:
	@$(MKDIR_P) $(OBJDIR) $(TESTDIR) $(LIBDIR)

.PHONY: clean
clean:
	@$(RM)	$(OBJDIR)

.PHONY: remove
remove: clean
	@$(RM)		$(LIBDIR)
	@$(RM)		$(TESTDIR)