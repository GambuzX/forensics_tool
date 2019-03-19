#include "forensic.h"
#include "parse.h"
#include "log.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <wait.h>
#include <errno.h>

void concat(char *str1[], char *str2, size_t n)
{
	const size_t len1 = strlen(*str1);
	const size_t len2 = n < strlen(str2) ? n : strlen(str2);
	*str1 = (char *)realloc(*str1, len1 + len2 + 1); // +1 for the null-terminator

	/* Check for errors */
	if (*str1 == NULL)
	{
		perror("realloc");
		exit(1);
	}

	/* Concatenate str2 */
	strncat(*str1, str2, len2);
}

int file_forensic(char flag, char *start_point, struct stat stat_buf, char *outfile)
{
	// file_name,file_type,file_size,file_access,file_created_date,file_modification_date,md5,sha1,sha256
	// Ex: hello.txt,ASCII text,100,rw,2018-01-04T16:30:19,2018-01-04T16:34:13,
	//		4ff75ff116aad93549013ef70f41e59c,d2d29b8b66e3ef44f3412224de6624edd09cdb0c,7605bf
	//		c397c3d78b15a8719ddbfa28e751b81c6127c61a9c171e3db60dd9d046
	printf("vou escrever ficheiro \n");
	struct tm ts;
	char buf[BUFFER_SIZE];
	char *full_cmd = (char *)malloc(1 + strlen(start_point) + strlen("sha256sum "));
	memset(full_cmd, '\0', 1);
	int n;
	FILE *fp;
	int filedes = STDOUT_FILENO;
	char *ret_string = (char *)malloc(1);
	memset(ret_string, '\0', 1);

	/* Open file */
	if (flag & FLAGS_O)
	{
		filedes = open(outfile, O_WRONLY | O_APPEND | O_CREAT, 0644);
	}

	strcpy(full_cmd, "file ");
	strcat(full_cmd, start_point);

	if ((fp = popen(full_cmd, "r")) == NULL)
	{
		printf("Error opening file pipe!\n");
		return -1;
	}
	// sleep(2);
	int started = 0;
	char *next;
	while (fgets(buf, BUFFER_SIZE, fp) != NULL)
	{
		if (!started)
		{
			next = strtok(buf, ":");
			concat(&ret_string, next, strlen(next));
			concat(&ret_string, ",", 1);
			next = strtok(NULL, ",");
			started = 1;
		}
		else
			next = strtok(buf, ",");

		while (next != NULL)
		{
			if ('\n' == next[strlen(next) - 1])
			{
				concat(&ret_string, next + 1, strlen(next + 1) - 1);
			}
			else
			{
				concat(&ret_string, next + 1, strlen(next + 1));
			}
			concat(&ret_string, ",", 1);
			next = strtok(NULL, ",");
		}
	}

	if (pclose(fp))
	{
		printf("entrei na condiçao pclose(fp)\n");
		if (errno != 0)
		{
			perror("pclose (file)");
			return -1;
		}
	}

	n = sprintf(buf, "%ld,", stat_buf.st_size);
	concat(&ret_string, buf, n);

	// PERMISSOES  , DEPOIS TEMOS QUE UTILIZAR PROVAVELMENTE O WRITE() PARA ISTO
	// printf( (S_ISDIR(stat_buf.st_mode)) ? "d" : "-");
	// printf( (stat_buf.st_mode & S_IRUSR) ? "r" : "-");
	// printf( (stat_buf.st_mode & S_IWUSR) ? "w" : "-");
	// printf( (stat_buf.st_mode & S_IXUSR) ? "x" : "-");
	// printf( (stat_buf.st_mode & S_IRGRP) ? "r" : "-");
	// printf( (stat_buf.st_mode & S_IWGRP) ? "w" : "-");
	// printf( (stat_buf.st_mode & S_IXGRP) ? "x" : "-");
	// printf( (stat_buf.st_mode & S_IROTH) ? "r" : "-");
	// printf( (stat_buf.st_mode & S_IWOTH) ? "w" : "-");
	// printf( (stat_buf.st_mode & S_IXOTH) ? "x" : "-");

	if (stat_buf.st_mode & S_IRUSR)
		concat(&ret_string, "r", 1);
	if (stat_buf.st_mode & S_IWUSR)
		concat(&ret_string, "w", 1);
	if (stat_buf.st_mode & S_IXUSR)
		concat(&ret_string, "x", 1);
	concat(&ret_string, ",", 1);
	fflush(stdout);

	// TODO: Both dates are the same
	ts = *localtime(&stat_buf.st_atime);
	strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S,", &ts);
	concat(&ret_string, buf, strlen(buf));

	ts = *localtime(&stat_buf.st_mtime);
	strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &ts);
	concat(&ret_string, buf, strlen(buf));

	if (flag & FLAGS_SHA1)
	{
		strcpy(full_cmd, "sha1sum ");
		strcat(full_cmd, start_point);

		if ((fp = popen(full_cmd, "r")) == NULL)
		{
			printf("Error opening sha1sum pipe!\n");
			return -1;
		}

		while (fgets(buf, BUFFER_SIZE, fp) != NULL)
		{
			char *next = strtok(buf, " ");
			concat(&ret_string, ",", 1);
			concat(&ret_string, next, strlen(next));
		}

		if (pclose(fp))
		{
			perror("pclose (SHA1SUM)");
			return -1;
		}
	}
	if (flag & FLAGS_SHA256)
	{
		strcpy(full_cmd, "sha256sum ");
		strcat(full_cmd, start_point);

		if ((fp = popen(full_cmd, "r")) == NULL)
		{
			printf("Error opening sha256sum pipe!\n");
			return -1;
		}

		while (fgets(buf, BUFFER_SIZE, fp) != NULL)
		{
			char *next = strtok(buf, " ");
			concat(&ret_string, ",", 1);
			concat(&ret_string, next, strlen(next));
		}

		if (pclose(fp))
		{
			perror("pclose (SHA256SUM)");
			return -1;
		}
	}

	if (flag & FLAGS_MD5)
	{
		strcpy(full_cmd, "md5sum ");
		strcat(full_cmd, start_point);

		if ((fp = popen(full_cmd, "r")) == NULL)
		{
			printf("Error opening md5sum pipe!\n");
			return -1;
		}

		while (fgets(buf, BUFFER_SIZE, fp) != NULL)
		{
			char *next = strtok(buf, " ");
			concat(&ret_string, ",", 1);
			concat(&ret_string, next, strlen(next));
		}

		if (pclose(fp))
		{
			perror("pclose (MD5)");
			return -1;
		}
	}

	concat(&ret_string, "\n", 1);
	write(filedes, ret_string, strlen(ret_string));
	free(ret_string);

	if (flag & FLAGS_O)
	{
		close(filedes);
	}

	char *logDesc = (char *)malloc(9 + strlen(start_point) + 1);
	if (logDesc == NULL)
	{
		perror("logDesc");
		return -2;
	}

	memset(logDesc, '\0', 1);
	strcat(logDesc, "ANALIZED ");
	strcat(logDesc, start_point);

	write_in_log(logDesc);
	free(logDesc);

	free(full_cmd);
	printf("sai do ficheiro\n");
	return 0;
}
