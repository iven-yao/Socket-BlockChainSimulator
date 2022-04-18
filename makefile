all: serverM serverA serverB serverC clientA clientB

serverM:serverM.c
	gcc -o serverM serverM.c

serverA:serverA.c
	gcc -o serverA serverA.c

serverB:serverB.c
	gcc -o serverB serverB.c
	
serverC:serverC.c
	gcc -o serverC serverC.c

clientA:clientA.c
	gcc -o clientA clientA.c
	
clientB:clientB.c
	gcc -o clientB clientB.c
