# !/bin/bash

if [ $# -eq 1 ] ;then
	file=$1
	gcc -shared -fPIC $file -o ${file%.*}.so  -I /home/oj/include
fi

for file in $(ls *.c)
do
	name=${file%.*} 
	gcc  -shared -fPIC  $file -o ${name}.so	 -I /home/oj/include
done



