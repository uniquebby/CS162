/* A struct that represents a list of path in PATH */
struct path_tokens {
  size_t paths_size;
  char **path_tokens;
  int next_token;
};

/* Initalize path_tokens according PATH */
void init_pathtoks(struct path_tokens *path_tokens);

/* Get next path_token */
char* next_path_token(struct path_tokens*);

/* Destroy path_tokens */
void path_tokens_destroy(struct path_tokens *path_tokens);

