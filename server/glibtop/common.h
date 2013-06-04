
#include <stdint.h>
#include <string.h>

char* skip_token(char *p);
char* skip_multiple_token(char *p, const uint32_t n);
void file_to_buffer(char* buffer, const uint32_t bufsize, const char *filename);
uint64_t get_scaled(const char *buffer, const char *key);
size_t strlcpy(char * dst, const char * src, size_t size);

