INCLUDE = ./includes

CFLAGS = -I$(INCLUDE)

OBJS = sniffer.o queue/ADTQueue.o worker/worker.o
# CFLAGS = -I$(INCLUDE)
# Το εκτελέσιμο πρόγραμμα
EXEC = sniffer


$(EXEC): $(OBJS)
	gcc  $(OBJS)  -o $(EXEC)
	
clean:
	rm -f $(OBJS) $(EXEC)

run: $(EXEC)
	./$(EXEC)