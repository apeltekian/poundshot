poundshot: poundshot.o
	$(CC) $(CFLAGS) poundshot.o -lncurses -o poundshot
clean:
	$(RM) *.o poundshot
