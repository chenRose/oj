all :  main.c ini-sample.c 
	gcc main.c -ldl -o main
	gcc ini-sample.c -o ini  -L /home/oj/lib -liniparser -I /home/oj/include/ 
clean : 
	rm -rf main
	rm -rf ini	
hash :hash.c
	gcc hash.c -I /home/oj/include/ -L /home/oj/lib -lhiredis
mysql :mysql.c
	gcc mysql.c  -L/usr/lib64/mysql -lmysqlclient


