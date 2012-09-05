#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

void remChar(char* s, int index){
	int len = strlen(s);
	int i = index+1;
	for(;i<len;i++){
		s[i-1] = s[i];
	}
	//move the \0 character into place. i = strlen(s) right now.
	s[i-1] = s[i];
}

void removewhitespace(char* s){
	int i = 0;
	while(s[i]!='\0'){
		if(isspace(s[i])){
			remChar(s,i);
			i--;
		}
		i++;
	}	
}

char* pascal2c(char* s){
	char* newStr = strdup(s);
	int len = strlen(s);	
	int i = 0;
	for(;i<len;i++){
		newStr[i]=newStr[i+1];
	}
	newStr[i]='\0';
	return newStr;
}


char* c2pascal(char* s){
	char* newStr = strdup(s);
	int len = strlen(s);	
	int i = len;
	for(;i>0;i--){
		newStr[i] = newStr[i-1];
	}
	newStr[i] = len;
	return newStr;
}
char** tokenify(char* s){
	char* str = strdup(s);
	char* temp = strtok(str, " \n\t");
	if(temp == NULL) return NULL;	
	int count =	0;
	while(temp!=NULL){
		count++;
		temp = strtok(NULL, " \n\t");
	}
	char** toks = malloc((count+1)*sizeof(char));
	toks[0] = strtok(s, " \n\t");
	int i = 1;
	for(;i<=count;i++){
		toks[i] = strtok(NULL, " \n\t");
	}
	return toks;
}
