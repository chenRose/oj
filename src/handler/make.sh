# !/bin/bash


if [ $# -eq 1 ] ;then
	file=$1
	gcc -shared -fPIC $file -o ${file%.*}.so  -I /home/oj/include
	exit 0
fi

old_dir=$(pwd)
module_path=/home/oj/src/handler
cd $module_path
for file in $(ls *.c)
do
	name=${file%.*} 
	gcc  -shared -fPIC  $file -o ${name}.so	 -I /home/oj/include
done

cd $old_dir

