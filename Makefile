gdsplit: gdsplit.c
	gcc -O -o gdsplit -lssl -lcrypto gdsplit.c
clean:
	-/bin/rm gdsplit
