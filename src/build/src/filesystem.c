/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file has been modified from Ken Silverman's original release
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if WIN32
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#endif

#include "build/filesystem.h"
#include "build/platform.h"
#include "cache.h"
#include "build/fixedPoint_math.h"
#include "global.h"
#include "memory/cache.h"



//The multiplayer module in game.dll needs direct access to the crc32 (sic).
int32_t groupefil_crc32[MAXGROUPFILES];


// A typical GRP index entry:
//     - 12 bytes for filename
//     -  4 for filesize
typedef uint8_t grpIndexEntry_t[16];

typedef struct grpArchive_s {
    // Number of files in the archive.
    int32_t numFiles;

    // Array containing the filenames.
    grpIndexEntry_t* gfilelist;

    // Array containing the file offsets.
    int32_t* fileOffsets;

    // Array containing the file offsets.
    int32_t* filesizes;

    // The fd used for open,read operations.
    int fileDescriptor;

    // Hash to recognize GRP: Duke Shareware, Duke plutonimum etc...
    uint32_t crc32;
} grpArchive_t;

//All GRP opened are in this structure
typedef struct grpSet_s {
    grpArchive_t archives[MAXGROUPFILES];
    int32_t num;
} grpSet_t;

// Marking it static gurantee not only invisility outside module
// but also that the content will be set to 0.
static grpSet_t grpSet;


//
// Compute CRC32 of the whole GRP file and implicitly caches
// the GRP in memory through windows caching service.
//
static void FS_ComputeCRC(grpArchive_t* archive) {
    static uint8_t crcBuffer[1 << 20];

    // Rewind the fileDescriptor.
    const int fd = archive->fileDescriptor;
    lseek(fd, 0, SEEK_SET);

    //Load the full GRP in RAM.
    int32_t j = (int32_t) read(fd, crcBuffer, sizeof(crcBuffer));
    while (j != 0) {
        archive->crc32 = crc32_update(crcBuffer, j, archive->crc32);
        j = (int32_t) read(fd, crcBuffer, sizeof(crcBuffer));
    }

    // The game layer seems to absolutely need to access an
    // array int[4] groupefil_crc32 so we need to store the
    // crc32 in there too.
    const int archive_num = grpSet.num - 1;
    groupefil_crc32[archive_num] = (int32_t) archive->crc32;
}

static void FS_ReadEntries(grpArchive_t* archive) {
    const int32_t num_files = archive->numFiles;
    const size_t size_list = num_files * sizeof(grpIndexEntry_t);

    archive->gfilelist = malloc(size_list);
    archive->fileOffsets = malloc(num_files * sizeof(int32_t));
    archive->filesizes = malloc(num_files * sizeof(int32_t));

    // Load the full index 16 bytes per file (12bytes for name + 4 bytes for the size).
    read(archive->fileDescriptor, archive->gfilelist, num_files * 16);

    //Initialize all file offset and pointers.
    int32_t offset = (int) (12 + 4 + size_list);
    for (int32_t i = 0; i < num_files; i++) {
        uint8_t* entry = archive->gfilelist[i];

        const int32_t size = BUILDSWAP_INTEL32(*((int32_t*) &entry[12]));
        // Now that the filesize has been read, we can replace it
        // with '0' and hence have a valid, null terminated character
        // string that will be usable.
        entry[12] = '\0';

        archive->filesizes[i] = size;
        archive->fileOffsets[i] = offset; // absolute offset list of all files.

        offset += size;
    }
}

static int FS_ReadHeader(grpArchive_t* archive) {
    uint8_t buf[16];
    read(archive->fileDescriptor, buf, 16);
    // Check the magic number (12 bytes header).
    if (strncmp(buf, "KenSilverman", 12) != 0) {
        return -1;
    }
    // The next 4 bytes of the header feature the
    // number of files in the GRP archive.
    archive->numFiles = BUILDSWAP_INTEL32(*((int32_t*) &buf[12]));
    return 0;
}

static void FS_OpenArchive(grpArchive_t* archive, const char* filename) {
    archive->fileDescriptor = open(filename, O_BINARY | O_RDONLY, S_IREAD);
    if (archive->fileDescriptor < 0) {
        printf("Error: Unable to open file %s.\n", filename);
        getchar();
        exit(0);
    }
}

static grpArchive_t* FS_NewArchive(void) {
    if (grpSet.num == MAXGROUPFILES) {
        return NULL;
    }
    grpArchive_t* archive = &grpSet.archives[grpSet.num];
    grpSet.num++;
    memset(archive, 0, sizeof(*archive));
    return archive;
}

//
// FCS: The ".grp" file format is just a collection of a lot of
// files stored into 1 big one.
//
// KS doc: I tried to make the format as simple as possible:
// The first 12 bytes contains my name, "KenSilverman".
// The next 4 bytes is the number of files that were compacted
// into the group file. Then for each file, there is a 16 byte
// structure, where the first 12 bytes are the filename, and the
// last 4 bytes are the file's size. The rest of the group file
// is just the raw data packed one after the other in the same
// order as the list of files. - ken
//
int32_t initgroupfile(const char* filename) {
    printf("Loading %s ...\n", filename);
    grpArchive_t* archive = FS_NewArchive();
    if (!archive) {
        printf("Error: Unable to open an extra GRP archive <= No more slot available.\n");
        return -1;
    }
    FS_OpenArchive(archive, filename);
    if (FS_ReadHeader(archive) == -1) {
        printf("Error: File %s is not a GRP archive.\n", filename);
        return -1;
    }
    FS_ReadEntries(archive);
    FS_ComputeCRC(archive);
    return grpSet.num - 1;
}

void uninitgroupfile(void) {
    for (int i = 0; i < grpSet.num; i++) {
        free(grpSet.archives[i].gfilelist);
        free(grpSet.archives[i].fileOffsets);
        free(grpSet.archives[i].filesizes);
        memset(&grpSet.archives[i], 0, sizeof(grpArchive_t));
    }
}

//
// Build CRC32 table.
//
static void crc32_table_gen(unsigned int* crc32_table) {
    static const unsigned int poly = 0xEDB88320L;
    for (int i = 0; i < 256; i++) {
        unsigned int crc = i;
        for (int j = 8; j > 0; j--) {
            if (crc & 1) {
                crc = (crc >> 1) ^ poly;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
}

unsigned int crc32_update(
    const uint8_t* buf,
    unsigned int length,
    unsigned int crc_to_update
) {
    unsigned int crc32_table[256];

    crc32_table_gen(crc32_table);

    crc_to_update ^= 0xFFFFFFFF;

    while (length--) {
        crc_to_update = crc32_table[(crc_to_update ^ *buf++) & 0xFF] ^ (crc_to_update >> 8);
    }

    return crc_to_update ^ 0xFFFFFFFF;
}


/*
 *                                      16   12   5
 * this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
 * This is 0x1021 when x is 2, but the way the algorithm works
 * we use 0x8408 (the reverse of the bit pattern).  The high
 * bit is always assumed to be set, thus we only use 16 bits to
 * represent the 17 bit value.
 */

#define POLY 0x8408   /* 1021H bit reversed */

uint16_t crc16(uint8_t  *data_p, uint16_t length)
{
    uint8_t  i;
    unsigned int data;
    unsigned int crc = 0xffff;
    
    if (length == 0)
        return (~crc);
    do
    {
        for (i=0, data=(unsigned int)0xff & *data_p++;
             i < 8;
             i++, data >>= 1)
        {
            if ((crc & 0x0001) ^ (data & 0x0001))
                crc = (crc >> 1) ^ POLY;
            else  crc >>= 1;
        }
    } while (--length);
    
    crc = ~crc;
    data = crc;
    crc = (crc << 8) | (data >> 8 & 0xff);
    
    return (crc);
}

// The engine can open files transparently on the filesystem or on the GRPsystem
enum fileType_e {
    SYSTEM_FILE,
    GRP_FILE,
};

//An entry in the array tracking open files
typedef struct {
    enum fileType_e type;
    // Either the fileDescriptor or the fileIndex in a GRP depending on the type.
    int fd;
    // lseek cursor
    int cursor;
    // GRP id
    int grpID;
    // Marker 1=used
    int used;
} openFile_t;


#define MAXOPENFILES 64
static openFile_t openFiles[MAXOPENFILES];

//
// Returns the index of the file in the GRP that
// has the given name, or -1 on failure.
//
static int FS_SearchGrp(
    const grpArchive_t* grp,
    const char* file_name
) {
    for (int32_t i = grp->numFiles - 1; i >= 0; i--) {
        const char* name = (char*) grp->gfilelist[i];
        if (strncasecmp(name, file_name, 12) == 0) {
            return i;
        }
    }
    return -1;
}

//
// Try to look in the GRP archives.
// In this case, fd = index of the file in the GRP.
//
static int FS_OpenGrpFile(const char* filename, openFile_t* file) {
    for (int32_t k = grpSet.num - 1; k >= 0; k--) {
        const grpArchive_t* grp = &grpSet.archives[k];
        const int file_id = FS_SearchGrp(grp, filename);
        if (file_id >= 0) {
            file->type = GRP_FILE;
            file->grpID = k;
            file->fd = file_id;
            file->used = 1;
            file->cursor = 0;
            return 1;
        }
    }
    return 0;
}

//
// Try to look in the filesystem first.
// In this case fd = file descriptor.
//
static int FS_OpenSysFile(const char* filename, openFile_t* file) {
    file->fd = open(filename, O_BINARY | O_RDONLY);
    if (file->fd == -1) {
        return 0;
    }
    file->type = SYSTEM_FILE;
    file->cursor = 0;
    file->used = 1;
    return 1;
}

static int32_t FS_NewFile(void) {
    // Search a free slot.
    for (int32_t i = MAXOPENFILES - 1; i >= 0; i--) {
        if (!openFiles[i].used) {
            return i;
        }
    }
    return -1;
}

int32_t kopen4load(const char* filename, int openOnlyFromGRP) {
    const int32_t file_id = FS_NewFile();
    if (file_id < 0) {
        Error(EXIT_FAILURE, "Too Many files open!\n");
    }
    openFile_t* file = &openFiles[file_id];
    if (!openOnlyFromGRP && FS_OpenSysFile(filename, file)) {
        return file_id;
    }
    if (FS_OpenGrpFile(filename, file)) {
        return file_id;
    }
    return -1;
}

int32_t kread(int32_t handle, void* buffer, int32_t leng) {
    openFile_t* file = &openFiles[handle];
    if (!file->used) {
        printf("Invalide handle. Unrecoverable error.\n");
        getchar();
        exit(0);
    }

    if (file->type == SYSTEM_FILE) {
        // FILESYSTEM ? OS takes care of it !
        return (int32_t) read(file->fd, buffer, leng);
    }

    // File is actually in the GRP.
    grpArchive_t* archive = &grpSet.archives[file->grpID];

    lseek(archive->fileDescriptor,
          archive->fileOffsets[file->fd] + file->cursor,
          SEEK_SET);

    //Adjust leng so we cannot read more than filesystem-cursor location.
    leng = SDL_min(leng, archive->filesizes[file->fd]-file->cursor);

    leng = read(archive->fileDescriptor, buffer, leng);

    file->cursor += leng;

    return leng;

}

int kread16(int32_t handle, short *buffer){
    if (kread(handle, buffer, 2) != 2)
        return(0);
    
    *buffer = BUILDSWAP_INTEL16(*buffer);
    return(1);
}

int kread32(int32_t handle, int32_t *buffer){
    if (kread(handle, buffer, 4) != 4)
        return(0);
    
    *buffer = BUILDSWAP_INTEL32(*buffer);
    return(1);
}

int kread8(int32_t handle, uint8_t  *buffer){
    if (kread(handle, buffer, 1) != 1)
        return(0);
    
    return(1);
}

int32_t klseek(int32_t handle, int32_t offset, int whence){
    
    grpArchive_t* archive;
	
    if (!openFiles[handle].used){
        printf("Invalide handle. Unrecoverable error.\n");
        getchar();
        exit(0);
    }
    
    // FILESYSTEM ? OS will take care of it.
    if (openFiles[handle].type == SYSTEM_FILE){
        return lseek(openFiles[handle].fd,offset,whence);
    }
    
    
    archive = & grpSet.archives [   openFiles[handle].grpID ];
	
    switch(whence){
        case SEEK_SET: openFiles[handle].cursor = offset; break;
        case SEEK_END: openFiles[handle].cursor = archive->filesizes[openFiles[handle].fd]; break;
        case SEEK_CUR: openFiles[handle].cursor += offset; break;
    }
    
    return(openFiles[handle].cursor);
	
	
}

#if defined(__APPLE__) || defined(__linux__)
int32_t filelength(int32_t fd){
    struct stat stats;
    fstat(fd, &stats);
    return (int32_t )stats.st_size;
}
#endif

int32_t kfilelength(int32_t handle)
{
	openFile_t* openFile = &openFiles[handle];
    
    if (!openFile->used){
        printf("Invalide handle. Unrecoverable error.\n");
        getchar();
        exit(0);
    }
    
    if (openFile->type == SYSTEM_FILE){
        return(filelength(openFile->fd));
    }
    
    else{
        grpArchive_t* archive = &grpSet.archives[ openFile->grpID ];
        return archive->filesizes[openFile->fd];
    }
    
}

void kclose(int32_t handle) {
    openFile_t* openFile = &openFiles[handle];

    //This is a typical handle for a non existing file.
    if (handle == -1)
        return;

    if (!openFile->used) {
        printf("Invalide handle. Unrecoverable error.\n");
        getchar();
        exit(0);
    }

    if (openFile->type == SYSTEM_FILE) {
        close(openFile->fd);
    }

    memset(openFile, 0, sizeof(openFile_t));
}




/* Internal LZW variables */
#define LZWSIZE 16384           /* Watch out for shorts! */
static uint8_t  *lzwbuf1, *lzwbuf4, *lzwbuf5;
static uint8_t  lzwbuflock[5];
static short *lzwbuf2, *lzwbuf3;



int32_t compress(uint8_t  *lzwinbuf, int32_t uncompleng, uint8_t  *lzwoutbuf)
{
	int32_t i, addr, newaddr, addrcnt, zx, *longptr;
	int32_t bytecnt1, bitcnt, numbits, oneupnumbits;
	short *shortptr;
    
	for(i=255;i>=0;i--) { lzwbuf1[i] = (uint8_t ) i; lzwbuf3[i] = (short) ((i+1)&255); }
	clearbuf((void *) (lzwbuf2),256>>1,0xffffffff);
	clearbuf((void *) (lzwoutbuf),((uncompleng+15)+3)>>2,0L);
    
	addrcnt = 256; bytecnt1 = 0; bitcnt = (4<<3);
	numbits = 8; oneupnumbits = (1<<8);
	do
	{
		addr = lzwinbuf[bytecnt1];
		do
		{
			bytecnt1++;
			if (bytecnt1 == uncompleng) break;
			if (lzwbuf2[addr] < 0) {lzwbuf2[addr] = (short) addrcnt; break;}
			newaddr = lzwbuf2[addr];
			while (lzwbuf1[newaddr] != lzwinbuf[bytecnt1])
			{
				zx = lzwbuf3[newaddr];
				if (zx < 0) {lzwbuf3[newaddr] = (short) addrcnt; break;}
				newaddr = zx;
			}
			if (lzwbuf3[newaddr] == addrcnt) break;
			addr = newaddr;
		} while (addr >= 0);
		lzwbuf1[addrcnt] = lzwinbuf[bytecnt1];
		lzwbuf2[addrcnt] = -1;
		lzwbuf3[addrcnt] = -1;
        
		longptr = (int32_t *)&lzwoutbuf[bitcnt>>3];
		longptr[0] |= (addr<<(bitcnt&7));
		bitcnt += numbits;
		if ((addr&((oneupnumbits>>1)-1)) > ((addrcnt-1)&((oneupnumbits>>1)-1)))
			bitcnt--;
        
		addrcnt++;
		if (addrcnt > oneupnumbits) { numbits++; oneupnumbits <<= 1; }
	} while ((bytecnt1 < uncompleng) && (bitcnt < (uncompleng<<3)));
    
	longptr = (int32_t *)&lzwoutbuf[bitcnt>>3];
	longptr[0] |= (addr<<(bitcnt&7));
	bitcnt += numbits;
	if ((addr&((oneupnumbits>>1)-1)) > ((addrcnt-1)&((oneupnumbits>>1)-1)))
		bitcnt--;
    
	shortptr = (short *)lzwoutbuf;
	shortptr[0] = (short)uncompleng;
	if (((bitcnt+7)>>3) < uncompleng)
	{
		shortptr[1] = (short)addrcnt;
		return((bitcnt+7)>>3);
	}
	shortptr[1] = (short)0;
	for(i=0;i<uncompleng;i++) lzwoutbuf[i+4] = lzwinbuf[i];
	return(uncompleng+4);
}

int32_t uncompress(uint8_t  *lzwinbuf, int32_t compleng, uint8_t  *lzwoutbuf)
{
	int32_t strtot, currstr, numbits, oneupnumbits;
	int32_t i, dat, leng, bitcnt, outbytecnt, *longptr;
	short *shortptr;
    
	shortptr = (short *)lzwinbuf;
	strtot = (int32_t )shortptr[1];
	if (strtot == 0)
	{
		copybuf((void *)((lzwinbuf)+4),(void *)((lzwoutbuf)),((compleng-4)+3)>>2);
		return((int32_t )shortptr[0]); /* uncompleng */
	}
	for(i=255;i>=0;i--) { lzwbuf2[i] = (short) i; lzwbuf3[i] = (short) i; }
	currstr = 256; bitcnt = (4<<3); outbytecnt = 0;
	numbits = 8; oneupnumbits = (1<<8);
	do
	{
		longptr = (int32_t *)&lzwinbuf[bitcnt>>3];
		dat = ((longptr[0]>>(bitcnt&7)) & (oneupnumbits-1));
		bitcnt += numbits;
		if ((dat&((oneupnumbits>>1)-1)) > ((currstr-1)&((oneupnumbits>>1)-1)))
        { dat &= ((oneupnumbits>>1)-1); bitcnt--; }
        
		lzwbuf3[currstr] = (short) dat;
        
		for(leng=0;dat>=256;leng++,dat=lzwbuf3[dat])
			lzwbuf1[leng] = (uint8_t ) lzwbuf2[dat];
        
		lzwoutbuf[outbytecnt++] = (uint8_t ) dat;
		for(i=leng-1;i>=0;i--) lzwoutbuf[outbytecnt++] = lzwbuf1[i];
        
		lzwbuf2[currstr-1] = (short) dat; lzwbuf2[currstr] = (short) dat;
		currstr++;
		if (currstr > oneupnumbits) { numbits++; oneupnumbits <<= 1; }
	} while (currstr < strtot);
	return((int32_t )shortptr[0]); /* uncompleng */
}


void kdfread(void *buffer, size_t dasizeof, size_t count, int32_t fil)
{
	size_t i, j;
	int32_t k, kgoal;
	short leng;
	uint8_t  *ptr;
    
	lzwbuflock[0] = lzwbuflock[1] = lzwbuflock[2] = lzwbuflock[3] = lzwbuflock[4] = 200;
	if (lzwbuf1 == NULL) allocache(&lzwbuf1,LZWSIZE+(LZWSIZE>>4),&lzwbuflock[0]);
	if (lzwbuf2 == NULL) allocache((uint8_t**)&lzwbuf2,(LZWSIZE+(LZWSIZE>>4))*2,&lzwbuflock[1]);
	if (lzwbuf3 == NULL) allocache((uint8_t**)&lzwbuf3,(LZWSIZE+(LZWSIZE>>4))*2,&lzwbuflock[2]);
	if (lzwbuf4 == NULL) allocache(&lzwbuf4,LZWSIZE,&lzwbuflock[3]);
	if (lzwbuf5 == NULL) allocache(&lzwbuf5,LZWSIZE+(LZWSIZE>>4),&lzwbuflock[4]);
    
	if (dasizeof > LZWSIZE) { count *= dasizeof; dasizeof = 1; }
	ptr = (uint8_t  *)buffer;
    
	kread(fil,&leng,2); kread(fil,lzwbuf5,(int32_t )leng);
	k = 0;
	kgoal = uncompress(lzwbuf5,leng,lzwbuf4);
    
	copybufbyte(lzwbuf4,ptr,(int32_t )dasizeof);
	k += (int32_t )dasizeof;
    
	for(i=1;i<count;i++)
	{
		if (k >= kgoal)
		{
			kread(fil,&leng,2); kread(fil,lzwbuf5,(int32_t )leng);
			k = 0; kgoal = uncompress(lzwbuf5,(int32_t )leng,lzwbuf4);
		}
		for(j=0;j<dasizeof;j++) ptr[j+dasizeof] = (uint8_t ) ((ptr[j]+lzwbuf4[j+k])&255);
		k += dasizeof;
		ptr += dasizeof;
	}

    assert(k == kgoal);
	
	lzwbuflock[0] = lzwbuflock[1] = lzwbuflock[2] = lzwbuflock[3] = lzwbuflock[4] = 1;
}

void dfread(void *buffer, size_t dasizeof, size_t count, FILE *fil)
{
	size_t i, j;
	int32_t k, kgoal;
	short leng;
	uint8_t  *ptr;
    
	lzwbuflock[0] = lzwbuflock[1] = lzwbuflock[2] = lzwbuflock[3] = lzwbuflock[4] = 200;
	if (lzwbuf1 == NULL) allocache(&lzwbuf1,LZWSIZE+(LZWSIZE>>4),&lzwbuflock[0]);
	if (lzwbuf2 == NULL) allocache((uint8_t**)&lzwbuf2,(LZWSIZE+(LZWSIZE>>4))*2,&lzwbuflock[1]);
	if (lzwbuf3 == NULL) allocache((uint8_t**)&lzwbuf3,(LZWSIZE+(LZWSIZE>>4))*2,&lzwbuflock[2]);
	if (lzwbuf4 == NULL) allocache(&lzwbuf4,LZWSIZE,&lzwbuflock[3]);
	if (lzwbuf5 == NULL) allocache(&lzwbuf5,LZWSIZE+(LZWSIZE>>4),&lzwbuflock[4]);
    
	if (dasizeof > LZWSIZE) {
        count *= dasizeof;
        dasizeof = 1;
    }
    
	ptr = (uint8_t  *)buffer;
    
	fread(&leng,2,1,fil);
    fread(lzwbuf5,(int32_t )leng,1,fil);
    
	k = 0;
    kgoal = uncompress(lzwbuf5,(int32_t )leng,lzwbuf4);
    
	copybufbyte(lzwbuf4,ptr,(int32_t )dasizeof);
	k += (int32_t )dasizeof;
    
	for(i=1;i<count;i++)
	{
		if (k >= kgoal)
		{
			fread(&leng,2,1,fil); fread(lzwbuf5,(int32_t )leng,1,fil);
			k = 0; kgoal = uncompress(lzwbuf5,(int32_t )leng,lzwbuf4);
		}
		for(j=0;j<dasizeof;j++) ptr[j+dasizeof] = (uint8_t ) ((ptr[j]+lzwbuf4[j+k])&255);
		k += dasizeof;
		ptr += dasizeof;
	}
	lzwbuflock[0] = lzwbuflock[1] = lzwbuflock[2] = lzwbuflock[3] = lzwbuflock[4] = 1;
}

void dfwrite(void *buffer, size_t dasizeof, size_t count, FILE *fil)
{
	size_t i, j, k;
	short leng;
	uint8_t  *ptr;
    
	lzwbuflock[0] = lzwbuflock[1] = lzwbuflock[2] = lzwbuflock[3] = lzwbuflock[4] = 200;
	if (lzwbuf1 == NULL) allocache(&lzwbuf1,LZWSIZE+(LZWSIZE>>4),&lzwbuflock[0]);
	if (lzwbuf2 == NULL) allocache((uint8_t**)&lzwbuf2,(LZWSIZE+(LZWSIZE>>4))*2,&lzwbuflock[1]);
	if (lzwbuf3 == NULL) allocache((uint8_t**)&lzwbuf3,(LZWSIZE+(LZWSIZE>>4))*2,&lzwbuflock[2]);
	if (lzwbuf4 == NULL) allocache(&lzwbuf4,LZWSIZE,&lzwbuflock[3]);
	if (lzwbuf5 == NULL) allocache(&lzwbuf5,LZWSIZE+(LZWSIZE>>4),&lzwbuflock[4]);
    
	if (dasizeof > LZWSIZE) { count *= dasizeof; dasizeof = 1; }
	ptr = (uint8_t  *)buffer;
    
	copybufbyte(ptr,lzwbuf4,(int32_t )dasizeof);
	k = dasizeof;
    
	if (k > LZWSIZE-dasizeof)
	{
		leng = (short)compress(lzwbuf4,k,lzwbuf5); k = 0;
		fwrite(&leng,2,1,fil); fwrite(lzwbuf5,(int32_t )leng,1,fil);
	}
    
	for(i=1;i<count;i++)
	{
		for(j=0;j<dasizeof;j++) lzwbuf4[j+k] = (uint8_t ) ((ptr[j+dasizeof]-ptr[j])&255);
		k += dasizeof;
		if (k > LZWSIZE-dasizeof)
		{
			leng = (short)compress(lzwbuf4,k,lzwbuf5); k = 0;
			fwrite(&leng,2,1,fil); fwrite(lzwbuf5,(int32_t )leng,1,fil);
		}
		ptr += dasizeof;
	}
	if (k > 0)
	{
		leng = (short)compress(lzwbuf4,k,lzwbuf5);
		fwrite(&leng,2,1,fil); fwrite(lzwbuf5,(int32_t )leng,1,fil);
	}
	lzwbuflock[0] = lzwbuflock[1] = lzwbuflock[2] = lzwbuflock[3] = lzwbuflock[4] = 1;
}


/*
================================================================================

GAME DIRECTORY

================================================================================
*/

#define DIR_LENGTH 512
static char game_dir[DIR_LENGTH] = {0};

int32_t TCkopen4load(const char* filename, int readfromGRP) {
    char fullfilename[DIR_LENGTH];
    if (game_dir[0] != '\0' && !readfromGRP) {
        sprintf(fullfilename, "%s\\%s", game_dir, filename);
        if (!SafeFileExists(fullfilename)) {
            // try root
            sprintf(fullfilename, "%s", filename);
        }
    } else {
        sprintf(fullfilename, "%s", filename);
    }
    return kopen4load(fullfilename, readfromGRP);
}


void setGameDir(const char* dir) {
    if (dir) {
        strncpy(game_dir, dir, DIR_LENGTH);
    }
}

const char* getGameDir(void) {
    return game_dir;
}

//==============================================================================


int getGRPcrc32(int grpID) {
    return grpSet.archives[grpID].crc32;
}
