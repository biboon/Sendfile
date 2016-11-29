LOCAL_LIBS  = -L ../Communication
LOCAL_LIBS += -lcom
CFLAGS     += -I ../Communication

# Les cibles pour l'executable
$(APP): client.o server.o
	$(CC) $(CFLAGS) client.o -o client $(LOCAL_LIBS) $(LIBS)
	$(CC) $(CFLAGS) server.o -o server $(LOCAL_LIBS) $(LIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

# La cible de nettoyage
clean:
	rm -f core *.o client server
