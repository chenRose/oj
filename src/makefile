all : core.c judger.c 
	gcc -o core core.c -I /home/oj/include
	gcc -o judger judger.c -I /home/oj/include  -liniparser  -L /home/oj/lib -lhiredis -ldl
clean : 
	rm core -rf 
	rm judger -rf 
