COMP   = -march=core2 -m64 -fno-common -pipe -pthread -fPIC -fsigned-char -fmax-errors=4 -Isrc
LIB    = -pipe -pthread
OPTS   = -DVGC_MALLOC_DEBUG_LEVEL=4 -DVGC_MALLOC_STACKTRACE -DVGC_MALLOC_MPROTECT -DVGC_MALLOC_STACKTRACE_SIGNAL
LIBDIR = $(HOME)/devel/vgcmalloc/lib64
BINDIR = $(HOME)/devel/vgcmalloc/bin
OBJDIR = /tmp

ifneq ("","$(findstring -DVGC_MALLOC_STACKTRACE,$(OPTS))")
	COMP += -g
endif

all:	$(BINDIR)/t1 $(BINDIR)/t2 $(BINDIR)/t3

clean:
	@rm -f $(OBJDIR)/*.o $(BINDIR)/t1 $(BINDIR)/t2 $(BINDIR)/t3 $(LIBDIR)/*.so

$(BINDIR)/t1:	$(OBJDIR)/test1.o $(LIBDIR)/libvgcmalloc.so
	gcc $(COMP) $(OPTS) -L$(LIBDIR) -Wl,-rpath=$(LIBDIR) -o $@ $< -lvgcmalloc

$(BINDIR)/t2:	$(OBJDIR)/test2.o $(LIBDIR)/libvgcnew.so
	g++ $(COMP) $(OPTS) -Llib64 -Wl,-rpath=$(LIBDIR) -lvgcnew -o $@ $<

$(BINDIR)/t3:	$(OBJDIR)/test3.o $(LIBDIR)/libvgcmalloc.so
	gcc $(COMP) $(OPTS) -Llib64 -Wl,-rpath=$(LIBDIR) -o $@ $< -lvgcmalloc

$(OBJDIR)/test1.o:	test/test1.c Makefile
	gcc $(COMP) $(OPTS) -Wall -c $< -o $@

$(OBJDIR)/test2.o:	test/test2.cpp Makefile
	g++ $(COMP) $(OPTS) -Wall -c $< -o $@

$(OBJDIR)/test3.o:	test/test3.c Makefile
	gcc $(COMP) $(OPTS) -Wall -c $< -o $@

$(LIBDIR)/libvgcmalloc.so:	$(OBJDIR)/vgc_common.o $(OBJDIR)/vgc_pthread.o $(OBJDIR)/vgc_message.o $(OBJDIR)/vgc_malloc.o $(OBJDIR)/vgc_malloc_mprotect.o $(OBJDIR)/vgc_stacktrace.o $(OBJDIR)/vgc_network.o
	gcc $(LIB) -shared -pthread -o $@ $^

$(LIBDIR)/libvgcnew.so:		$(OBJDIR)/vgc_new.o $(OBJDIR)/vgc_memoryManager.o
	gcc $(LIB) -shared -pthread -o $@ $^ -Wl,-rpath=. $(LIBDIR)/libvgcmalloc.so

$(OBJDIR)/vgc_malloc.o:	src/vgc_malloc.c Makefile src/vgc_malloc.h src/vgc_malloc_private.h
	gcc $(COMP) $(OPTS) -Wall -c $< -o $@

$(OBJDIR)/vgc_malloc_mprotect.o:	src/vgc_malloc_mprotect.c Makefile src/vgc_malloc_mprotect.h
	gcc $(COMP) $(OPTS) -Wall -c $< -o $@

$(OBJDIR)/vgc_network.o:	src/vgc_network.c Makefile src/vgc_network.h
	gcc $(COMP) $(OPTS) -Wall -c $< -o $@

$(OBJDIR)/vgc_pthread.o:	src/vgc_pthread.c Makefile src/vgc_pthread.h
	gcc $(COMP) $(OPTS) -Wall -c $< -o $@

$(OBJDIR)/vgc_common.o:	src/vgc_common.c Makefile src/vgc_common.h
	gcc $(COMP) $(OPTS) -Wall -c $< -o $@

$(OBJDIR)/vgc_message.o:	src/vgc_message.c Makefile src/vgc_message.h
	gcc $(COMP) $(OPTS) -Wall -c $< -o $@

$(OBJDIR)/vgc_stacktrace.o:	src/vgc_stacktrace.c Makefile src/vgc_stacktrace.h
	gcc $(COMP) $(OPTS) -Wall -c $< -o $@

$(OBJDIR)/vgc_error.o:	src/vgc_error.c Makefile src/vgc_error.h
	gcc $(COMP) $(OPTS) -Wall -c $< -o $@

$(OBJDIR)/vgc_new.o:	src/vgc_new.cpp Makefile src/vgc_new.hpp
	g++ $(COMP) $(OPTS) -Wall -c $< -o $@

$(OBJDIR)/vgc_memoryManager.o:	src/vgc_memoryManager.cpp Makefile src/vgc_memoryManager.hpp
	g++ $(COMP) $(OPTS) -Wall -c $< -o $@
