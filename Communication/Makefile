# Makefile pour generer la bibliotheque de threads
LIB = libcom

# Regle generale
all: $(LIB).a

# Les cibles pour la bibliotheque
%.o: %.c %.h
	$(CC) -c $(CFLAGS) $< -o $@

# Cible de creation de la bibliotheque
%.a: %.o
	$(AR) -q $@ $<

# La cible de nettoyage
clean:
	rm -f core *.o *.a
