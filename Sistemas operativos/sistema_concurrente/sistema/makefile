CC      = gcc
CFLAGS  = -Wall -Wextra -g

PROGS   = gestor surtidor caudalimetro monitor sumidero llenaDeposito

all: $(PROGS)

gestor: gestor.o
	$(CC) $(CFLAGS) -o gestor gestor.o

surtidor: surtidor.o
	$(CC) $(CFLAGS) -o surtidor surtidor.o

caudalimetro: caudalimetro.o
	$(CC) $(CFLAGS) -o caudalimetro caudalimetro.o

monitor: monitor.o
	$(CC) $(CFLAGS) -o monitor monitor.o

sumidero: sumidero.o
	$(CC) $(CFLAGS) -o sumidero sumidero.o

llenaDeposito: llenaDeposito.o
	$(CC) $(CFLAGS) -o llenaDeposito llenaDeposito.o

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(PROGS)
