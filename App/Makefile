LOCAL_LIBS  = -L ../Communication
LOCAL_LIBS += -lcom
CFLAGS     += -I ../Communication

OBJS = $(APP).o

# Cible generale
all: $(APP)

# Les cibles pour l'executable
$(APP): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LOCAL_LIBS) $(LIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

# La cible de nettoyage
clean:
	rm -f core *.o $(APP)
