#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "fat32.h"
#include "superblock.h"
#include "buffer.h"

#define SECTOR_SIZE 512

unsigned char *cache;
unsigned char *cache512;
unsigned char *buf;

struct FAT32 fat;
struct super_block *sb;

/* Usage:
 * buf = read_sec(xxx);
 * Beware size of buf is 512
 */
unsigned char *read_sec(unsigned int sector)
{
	memset(cache, 0, 4096);
	unsigned char *ptr = buf + sector * SECTOR_SIZE;
	memcpy(cache512, ptr, 512);
	return cache512;
}

/* Usage:
 * buf = read_clus(xxx);
 */
unsigned char *read_clus(unsigned int clus)
{
        unsigned int sector = get_sec(clus);
        unsigned char *ptr = buf + sector * SECTOR_SIZE;
        memcpy(cache, ptr, 4096);
	return cache;
}

static inline unsigned int find_first_partition()
{
        struct Partition *partition = (struct Partition*)(buf+0x1be);
        return partition->startlba;
}

void read_content(unsigned int clus)
{
	unsigned int sector = get_sec(clus);
	unsigned char *text = buf+sector*SECTOR_SIZE;
	printf("%s", text);
}

void dump_file(unsigned int first_clus, unsigned int size)
{
	FILE *fp = fopen("e.dat", "wb");
	unsigned int next_clus = first_clus;
	unsigned char *ptr;
        if (size <= 4096) {
		ptr = buf + get_sec(next_clus) * SECTOR_SIZE;
                fwrite(ptr, size, 1, fp);
		fclose(fp);
		return;
	}

	do {
		ptr = buf + get_sec(next_clus) * SECTOR_SIZE;
		fwrite(ptr, 4096, 1, fp);
		size -= 4096;
		next_clus = find_next_cluster(next_clus);
	} while (size >=4096 && next_clus != 0x0FFFFFFF);

	ptr = buf + get_sec(next_clus) * SECTOR_SIZE;
	fwrite(ptr, size, 1, fp);
	fclose(fp);
}

void fmtfname(char *dst, const char *src)
{
//format file name to 8 3 (DOS)
//WARNING: make sure 'dst' must allocated 11 bytes
// and filled with 0x20 (space)

	int count = 0;
	while (1) {
        	if (*src == '.') {
        		dst += 8-count;
	        	++src;
        		continue;
        	} else if (*src == '\0')
        		break;

	        *dst++ = *src++;
        	++count;
	}
}

void test_func()
{
	vfat_find("Thisisa_longfile.mydata.ok");
}

#if 0
void read_file()
{
//	unsigned int datasec = get_sec(2);
//	struct dir_entry *dir = (struct dir_entry*)(buf+(datasec*SECTOR_SIZE));
	struct dir_entry *dir = (struct dir_entry*)read_clus(2);
	unsigned int i = 0;
	unsigned int long_flag = 0;

	for (; i < 16; ++i) {
		if (dir->DIR_Name[0] == 0)	/* no more files */
			break;
		if (dir->DIR_Name[0] == 0xe5) {	/* deleted file */
			++dir;
			continue;
		}
		if (dir->DIR_Attr == 0x0f) {	/* subcomponent long name */
			long_flag = 1;
			++dir;
			continue;
		} else {
			if (long_flag == 0)
				printf("Filename: %s\n", dir->DIR_Name);
			else {
				printf("Filename: %s (It has long name)\n", dir->DIR_Name);
				long_flag = 0;
			}
		}

		unsigned int clus = (dir->DIR_FstClusHI << 16 | dir->DIR_FstClusLO);
		printf("Size: %d\n", dir->DIR_FileSize);
		printf("Data cluster: %d\n", clus);
//		if (dir->DIR_Name[0] == 'E')
//			dump_file(clus, dir->DIR_FileSize);
		++dir;
	}
}
#endif

int main()
{
	int fd = open("fat32.img", O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	struct stat st;
	fstat(fd, &st);

	sb = (struct super_block*)malloc(sizeof(struct super_block));
	init_superblock();

	cache = malloc(4096);
	cache512 = malloc(512);

	buf = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if ((char*)buf == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	init_fat();
	test_func();

	free(cache);
	free(cache512);
	free(sb);

	munmap(buf, st.st_size);
	close(fd);

	return 0;
}

