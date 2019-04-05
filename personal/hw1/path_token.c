#include <stdlib.h>
#include <string.h>
#include "path_token.h"
#define MAX_PATHS 255     //PATH中可能的最大token数
#define MAX_PATH_LEN 255  //路径的最大字符长度

/* Initalize path_tokens according PATH */
void init_pathtoks(struct path_tokens *path_tokens) {
  path_tokens->paths_size = 0;
  path_tokens->path_tokens = (char**) malloc(sizeof(char*)*MAX_PATHS);
  path_tokens->next_token = 0;

  char *paths = getenv("PATH");
  char *result = strtok(paths, ":");
  while (result != NULL) {
	path_tokens->path_tokens[path_tokens->paths_size] = (char*) malloc(sizeof(char)*MAX_PATH_LEN);
	strcpy(path_tokens->path_tokens[path_tokens->paths_size], result);
	strcat(path_tokens->path_tokens[path_tokens->paths_size++], "/");
    result = strtok(NULL, ":");
  }  
}

/* Get next path_token */
char* next_path_token(struct path_tokens* path_tokens) {
 char *result = NULL;
 if (path_tokens->next_token < path_tokens->paths_size) {
  result = path_tokens->path_tokens[path_tokens->next_token++];
 }
 return result; 
}

/* Destroy path_tokens */
void path_tokens_destroy(struct path_tokens *path_tokens) {
  if (path_tokens == NULL) {
	return;
  }
  for (int i = 0; i < path_tokens->paths_size; i++) {
	free(path_tokens->path_tokens[i]);
  }
  if (path_tokens->path_tokens) {
	free(path_tokens->path_tokens);
  }
  free(path_tokens);
}
