#include "get_next_line.h"

size_t	ft_strlen(const char *s)
{
	size_t	cnt;

	if (!s)
		return (0);
	cnt = 0;
	while (s[cnt])
		++cnt;
	return (cnt);
}

size_t	ft_strlcpy(char *dst, const char *src, size_t dstsize)
{
	size_t	i;
	size_t	n;

	if (!dst || !src)
		return (0);
	n = ft_strlen(src);
	if (dstsize == 0)
		return (n);
	i = 0;
	while (src[i] && i < dstsize - 1)
	{
		dst[i] = src[i];
		++i;
	}
	dst[i] = '\0';
	return (n);
}

char	*get_line(char *s, char **line)
{
	int		i;
	char	*ret;

	i = 0;
	while (s[i] && s[i] != '\n')
		i++;
	*line = (char *)malloc(i + 1);
	if (!*line)
	{
		free(s);
		return (NULL);
	}
	ft_strlcpy(*line, s, i + 1);
	ret = (char *)malloc(ft_strlen(s + i));
	if (!ret)
	{
		free(s);
		return (NULL);
	}
	ft_strlcpy(ret, s + i + 1, ft_strlen(s + i));
	free(s);
	return (ret);
}

char	*strjoin(char *s1, char *s2)
{
	size_t	len1;
	size_t	len2;
	char	*ret;

	len1 = ft_strlen(s1);
	len2 = ft_strlen(s2);
	ret = (char *)malloc(len1 + len2 + 1);
	if (!ret)
	{
		free(s1);
		return (NULL);
	}
	ft_strlcpy(ret, s1, len1 + 1);
	ft_strlcpy(ret + len1, s2, len2 + 1);
	free(s1);
	return (ret);
}

int	has_new_line(char *s)
{
	if (!s)
		return (0);
	while (*s)
		if (*s++ == '\n')
			return (1);
	return (0);
}

int	free_buf_and_return(char *buf, int num, char **next, int last)
{
	free(buf);
	if (num == -1 || !*next)
	{
		*next = NULL;
		return (-1);
	}
	else if (last)
	{
		if (num == 0)
		{
			free(*next);
			*next = NULL;
		}
		return (0);
	}
	else
		return (1);
}

int	get_next_line(int fd, char **line)
{
	static char	*next;
	char		*buf;
	int			num;
	int			last;

	if (fd < 0 || !line || BUFFER_SIZE <= 0)
		return (-1);
	buf = (char *)malloc(BUFFER_SIZE + 1);
	if (!buf)
		return (-1);
	num = 1;
	while (!has_new_line(next) && num != 0)
	{
		num = read(fd, buf, BUFFER_SIZE);
		if (num == -1)
			return (free_buf_and_return(buf, num, &next, 0));
		buf[num] = '\0';
		next = strjoin(next, buf);
		if (!next)
			return (free_buf_and_return(buf, num, &next, 0));
	}
	last = !has_new_line(next);
	next = get_line(next, line);
	return (free_buf_and_return(buf, num, &next, last));
}
