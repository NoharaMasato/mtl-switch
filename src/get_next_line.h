#ifndef GET_NEXT_LINE_H_
#define GET_NEXT_LINE_H_

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 1024

int		get_next_line(int fd, char **line);

#endif
