#include<stdio.h>
#include<stdlib.h>
#include<string.h>

void main()
{
	char d[20]="hello";
	char s[10]="world";
	printf("%s,%s",strcat(d,s),d);
}
