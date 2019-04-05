#pragma once

/* A struct that represents a list of words. */
struct tokens {
  size_t tokens_length;
  char **tokens;
  size_t buffers_length;
  char **buffers;
};


/* Turn a string into a list of words. */
struct tokens *tokenize(const char *line);

/* How many words are there? */
size_t tokens_get_length(struct tokens *tokens);

/* Get me the Nth word (zero-indexed) */
char *tokens_get_token(struct tokens *tokens, size_t n);

/* Free the memory */
void tokens_destroy(struct tokens *tokens);

/* Return the token that is an exact match of r */
int is_direct_tok(struct tokens *tokens, char *r);

/* Judge if the tokens is a backgroud tokens */
bool is_bg_tok(struct tokens *tokens);
