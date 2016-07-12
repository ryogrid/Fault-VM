CC = gcc
CFLAGS = -I /root/local/include/apr-0/ -g -o
LIBS = -L/root/work/xenaccess/trunk/libxa/xenaccess/.libs /root/work/xenaccess/trunk/libxa/xenaccess/.libs/libxenaccess.so -lm -lxenctrl -lxenstore  -Wl,--rpath -Wl,/usr/local/lib
LPR_LIBS = /root/local/lib/libapr-0.a -lpthread
MYLIBS = file_util.o tcp_util.o experiment_base.o $(LPR_LIBS)


test_random_access.out: test_random_access.c file_util.h $(MYLIBS)
	$(CC) $(CFLAGS) $@ $< $(LIBS) $(MYLIBS)
	etags *.[ch]

from_head_access.out: from_head_access.c file_util.h $(MYLIBS)
	$(CC) $(CFLAGS) $@ $< $(LIBS) $(MYLIBS)
	etags *.[ch]

test_memory_alloc.out: test_memory_alloc.c file_util.h $(MYLIBS)
	$(CC) $(CFLAGS) $@ $< $(LIBS) $(MYLIBS)

random_with_pause.out: random_with_pause.c file_util.h $(MYLIBS)
	$(CC) $(CFLAGS) $@ $< $(LIBS) $(MYLIBS)
	etags *.[ch]

experiment_once.out: experiment_once.c file_util.h $(MYLIBS)
	$(CC) $(CFLAGS) $@ $< $(LIBS) $(MYLIBS)
	etags *.[ch]

experiment_once_lvs.out: experiment_once_lvs.c file_util.h $(MYLIBS)
	$(CC) $(CFLAGS) $@ $< $(LIBS) $(MYLIBS)
	etags *.[ch]

.c.o:
	$(CC) -c $<
