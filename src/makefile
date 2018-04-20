all : core.c judger.c 
	gcc -o core core.c -I /home/oj/include -L /home/oj/lib -liniparser	
	gcc -o judger judger.c -I /home/oj/include  -liniparser  -L /home/oj/lib -lhiredis -ldl -lmysqlclient -L /usr/lib64/mysql
clean : 
	rm core -rf 
	rm judger -rf 
