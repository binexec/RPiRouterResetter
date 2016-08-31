main_program: logging.o extern_cfg.o
	gcc -O3 -o network_reset network_reset.c logging.o extern_cfg.o -lpigpio -lrt -lpthread

logging.o:  logging.c 
	gcc -O3 -c logging.c

extern_cfg.o: extern_cfg.c
	gcc -O3 -c extern_cfg.c

.PHONY: clean

clean:
	rm -f *.o
