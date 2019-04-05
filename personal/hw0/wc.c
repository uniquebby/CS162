#include<stdio.h>
#include<unistd.h>
#include<sys/stat.h>
#include<stdlib.h>
int main(int argc, char *argv[]) {
	int lines=0, words=0, chars=0;	

	if(argc == 2){
		char* filename = argv[1];
		FILE *fp = fopen(filename, "r");
       		
		char ch;
		int flag = 0;
		while((ch =fgetc(fp)) != EOF){
			chars++;

			if(ch == '\n'){
				lines++;
			}
			if(ch == '\t' || ch == ' ' || ch == '\n'){
				flag = 0;
				continue;
			}
			else{
				if(flag == 0){
					words++;
					flag =1;

				}
			}
		}
		fclose(fp);
	}
	else{
		char ch;
		int flag = 0;
		while((ch = getchar())!=EOF){
			chars++;

			if(ch == '\n'){
				lines++;
			}
			if(ch == '\t' || ch == ' ' || ch == '\n'){
				flag = 0;
				continue;
			}
			else{
				if(flag == 0){
					words++;
					flag = 1;

				}
			}
		}
	
	}
	printf("chars:%d, lines:%d, words:%d", chars, lines, words);
    return 0;
}
