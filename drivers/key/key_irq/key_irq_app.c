/**
 * @file key_irq_app.c
 * @author panxingyuan (panxingyuan1@163.com)
 * @brief ker_irq,ko test application.
 * @version 0.1
 * @date 2023-12-08
 *       Create this file.
 * @copyright Copyright (c) 2023
 * @note
 *       Toolchain: arm-xilinx-linux-gnueabi-gcc (Sourcery CodeBench Lite 2012.09-104) 4.7.2
 *                  (需安装Xilinx SDK 2013.1)
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	int fd = -1;
	int key_val = 0;

    if (2 != argc)
    {
		printf("Usage:\n\t./keyApp /dev/key\n");
		return -1;
	}

	fd = open(argv[1], O_RDONLY);
	if(fd == -1)
    {
		fprintf(stderr, "ERROR: %s file open failed!\n", argv[1]);
		return -1;
	}

	while (1)
    {
		read(fd, &key_val, sizeof(int));
		if (0 == key_val)
        {
            printf("Key Press\n");
        }
		else if (1 == key_val)
        {
            printf("Key Release\n");
        }
	}

	close(fd);

	return 0;
}

