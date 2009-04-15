// Plugin Defs
#include "Locals.h"
// ISO9660 Filesystem Defs
#include "iso_fs.h"

#define	MAX_NUM_FILE_ENTRIES	8192	// max number of files
#define	MAX_NUM_DIR_ENTRIES		512		// max number of unreaded directories

typedef struct {
	int		start_sector;				// starting sector
	int		end_sector;					// ending sector
#ifdef	_DEBUG
	int		dir_link;					// link to the parent directory entry
	char	*file_name;					// ptr. to the file name on 'file_names' buffer
#endif//_DEBUG
} FILE_ENTRY;

typedef struct {
	int		start_sector;				// dir first unreaded sector
	int		end_sector;					// dir last unreaded sector
#ifdef	_DEBUG
	int		file_link;					// link to the directory entry on 'found_files'
#endif//_DEBUG
} DIR_ENTRY;

#ifdef	_DEBUG
typedef struct _name_buffers {
	struct _name_buffers	*next_name_buffer;
	int		name_offset;
	char	name_buffer[32768];
} NAME_BUFFERS;
#endif//_DEBUG

static FILE_ENTRY	*found_files = NULL;	// found files
static WORD		*sorted_files_idx = NULL;	// index of 'found_files' sorted by location
static DIR_ENTRY	*missing_dirs = NULL;	// unreaded dirs
#ifdef	_DEBUG
static NAME_BUFFERS	*file_names = NULL;	// buffer for file names (linked list)
#endif//_DEBUG
static int		num_found_files = 0;	// number of entries on 'found_files'
static int		num_missing_dirs = 0;	// number of entries on 'missing_dirs'
static int		last_sorted_file = 0;	// last updated entry on 'sorted_files_idx'
static ISO_PRIMARY_DESCRIPTOR	saved_iso_primary_descriptor;	// saved copy of the primary descriptor (to detect disk changes)

BOOL	iso9660_Alloc(void);
void	iso9660_Free(void);
BOOL	iso9660_get_sector(int loc, int *start, int *end);
void	iso9660_put_sector(const BYTE *buffer, int loc);
BOOL	iso9660_read_file(const char *file_path, char *data, int *size);

#ifndef	_DEBUG
static BOOL	iso9660_add_new_dir(int extent, int size);
static BOOL	iso9660_add_new_file(int extent, int size);
#else
static BOOL	iso9660_add_new_dir(int extent, int size, const char *name, int name_len, int parent_dir);
static BOOL	iso9660_add_new_file(int extent, int size, const char *name, int name_len, int parent_dir);
static void	PRINT_FILE_INFO(int file_idx);
#endif//_DEBUG
static void	iso9660_update_sorted_files_index(void);
static void	iso9660_flush_all(void);


BOOL iso9660_Alloc(void)
{
	found_files = (FILE_ENTRY *)HeapAlloc(hProcessHeap, 0, MAX_NUM_FILE_ENTRIES * sizeof(*found_files));
	sorted_files_idx = (WORD *)HeapAlloc(hProcessHeap, 0, MAX_NUM_FILE_ENTRIES * sizeof(*sorted_files_idx));
	missing_dirs = (DIR_ENTRY *)HeapAlloc(hProcessHeap, 0, MAX_NUM_DIR_ENTRIES * sizeof(*missing_dirs));
#ifdef	_DEBUG
	file_names = (NAME_BUFFERS *)HeapAlloc(hProcessHeap, 0, sizeof(*file_names));
#endif//_DEBUG
	if (found_files == NULL || sorted_files_idx == NULL || missing_dirs == NULL
#ifdef	_DEBUG
		|| file_names == NULL
#endif//_DEBUG
		) {
		iso9660_Free();
		return FALSE;
	}
#ifdef	_DEBUG
	file_names->next_name_buffer = NULL;
#endif//_DEBUG
	iso9660_flush_all();
	return TRUE;
}

void iso9660_Free(void)
{
	iso9660_flush_all();				// cleanup nr. of dirs/files entries & free name buffers chain
	HeapFree(hProcessHeap, 0, found_files);
	found_files = NULL;
	HeapFree(hProcessHeap, 0, sorted_files_idx);
	sorted_files_idx = NULL;
	HeapFree(hProcessHeap, 0, missing_dirs);
	missing_dirs = NULL;
#ifdef	_DEBUG
	HeapFree(hProcessHeap, 0, file_names);
	file_names = NULL;
#endif//_DEBUG
}

BOOL iso9660_get_sector(int loc, int *start, int *end)
{
	int first = 0;
	int last = last_sorted_file - 1;
	while (first <= last) {
		int mid = (first + last) / 2;
		FILE_ENTRY	*fil = &found_files[sorted_files_idx[mid]];
		if (fil->start_sector <= loc && fil->end_sector >= loc) {
#ifdef	_DEBUG
			static int last_found = -1;
			if (sorted_files_idx[mid] != last_found) {
				last_found = sorted_files_idx[mid];
				PRINT_FILE_INFO(last_found);
			}
#endif//_DEBUG
			if (start) *start = fil->start_sector;
			if (end) *end = fil->end_sector;
			return TRUE;
		}
		if (fil->start_sector > loc)
			last = mid - 1;
		else
			first = mid + 1;
	}
	//PRINTF(("@"));
	return FALSE;
}

void iso9660_put_sector(const BYTE *buffer, int loc)
{
	const ISO_PRIMARY_DESCRIPTOR	*ipd;
	const ISO_DIRECTORY_RECORD	*idr;
	DIR_ENTRY		*dir;
	int		i, idx;

	buffer += ((buffer[12+3]==2) ? 12+4+8 : 12+4);	// skip all data headers (Mode2/Mode1)

	if (loc == 16)
	{
		ipd = (ISO_PRIMARY_DESCRIPTOR *)buffer;
		idr = (ISO_DIRECTORY_RECORD *)ipd->root_directory_record;
		if (memcmp(ipd, &saved_iso_primary_descriptor, sizeof(*ipd)))
			iso9660_flush_all();		// disk changed: flush any file & dir info
		if (num_found_files == 0 &&		// if root dir not already present ...
			!strncmp(ipd->id, ISO_STANDARD_ID, sizeof(ipd->id))) {	// ... and disk in ISO9660 format:
			memcpy(&saved_iso_primary_descriptor, ipd, sizeof(*ipd));	// save the primary descriptor to detect disk changes
			iso9660_add_new_dir(ISO733(idr->extent), ISO733(idr->size)
#ifdef	_DEBUG
				, "", 0, 0				// (parent_dir = 0)
#endif//_DEBUG
				);	// add the root directory entry location
			iso9660_update_sorted_files_index();
		}
		return;
	}
	for (i = 0, dir = missing_dirs; i < num_missing_dirs; i++, dir++) {
		if (dir->start_sector != loc)
			continue;
		for (idx = 0; idx <= (ISOFS_BLOCK_SIZE - sizeof(ISO_DIRECTORY_RECORD)); idx += idr->length[0]) {
			idr = (ISO_DIRECTORY_RECORD *)&buffer[idx];
			if (idr->length[0] == 0)
				break;
			if (idr->name_len[0] == 1 &&	// skip '.' and '..' entries
				(idr->name[0] == 0 || idr->name[0] == 1))
				continue;
			((idr->flags[0] & ISO_DIRECTORY) ? iso9660_add_new_dir : iso9660_add_new_file)
				(ISO733(idr->extent), ISO733(idr->size)
#ifdef	_DEBUG
				 , idr->name, idr->name_len[0], dir->file_link
#endif//_DEBUG
				);
		}
		iso9660_update_sorted_files_index();
		if (dir->start_sector < dir->end_sector)
			dir->start_sector++;
		else if (--num_missing_dirs > i)
			memcpy(dir, dir + 1, (num_missing_dirs - i) * sizeof(*dir));
		break;
	}
}

static BYTE *iso9660_read_sector_cooked(BYTE *read_buffer, int loc)
{
	if (!if_drive.StartReadSector(loc, &read_buffer) ||
		!if_drive.WaitEndReadSector())
		return NULL;
	return &read_buffer[(read_buffer[12+3]==2) ? 12+4+8 : 12+4];	// skip all data headers (Mode2/Mode1)
}

static BOOL iso9660_find_root_entry(int *dir_sector, int *dir_size)
{
	BYTE	read_buffer[RAW_SECTOR_SIZE], *pbuf;
	const ISO_PRIMARY_DESCRIPTOR	*ipd;
	const ISO_DIRECTORY_RECORD	*idr;

	if ((pbuf = iso9660_read_sector_cooked(read_buffer, 16)) == NULL)
		return FALSE;
	ipd = (ISO_PRIMARY_DESCRIPTOR *)pbuf;
	idr = (ISO_DIRECTORY_RECORD *)ipd->root_directory_record;
	if (strncmp(ipd->id, ISO_STANDARD_ID, sizeof(ipd->id)))	// ensure disk in ISO9660 format
		return FALSE;
	// get root directory entry / size
	*dir_sector = ISO733(idr->extent);
	*dir_size = ISO733(idr->size);
	return TRUE;
}

static BOOL iso9660_find_dir_entry(const char *name, int name_len, int *dir_sector, int *dir_size)
{
	BYTE	read_buffer[RAW_SECTOR_SIZE], *pbuf;
	const ISO_DIRECTORY_RECORD	*idr;
	int		sector = *dir_sector;
	int		size = *dir_size;
	int		i, idx, len;

	for ( ; size > 0; size -= len, sector++) {
		len = (size > COOKED_SECTOR_SIZE) ? COOKED_SECTOR_SIZE : size;
		if ((pbuf = iso9660_read_sector_cooked(read_buffer, sector)) == NULL)
			return FALSE;
		for (idx = 0; idx <= (COOKED_SECTOR_SIZE - sizeof(ISO_DIRECTORY_RECORD)) && idx < len; idx += idr->length[0]) {
	        idr = (ISO_DIRECTORY_RECORD *)&pbuf[idx];
			if (idr->length[0] == 0)
				break;
			if (idr->name_len[0] == 1 &&	// skip '.' and '..' entries
				(idr->name[0] == 0 || idr->name[0] == 1))
				continue;
			for (i = 0; i < idr->name_len[0]; i++)
				if (idr->name[i] == ';')
					break;	// name stops before version field (";1")
			if (i != name_len || strnicmp(name, idr->name, name_len))
				continue;
			*dir_sector = ISO733(idr->extent);
			*dir_size = ISO733(idr->size);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL iso9660_read_file(const char *file_path, char *data_buff, int *data_size)
{
	const char	*p;
	int		sector, size, len;

	if (!iso9660_find_root_entry(&sector, &size))
		return FALSE;
	for (;;) {
		if (*file_path == '/') file_path++;
		if (!*file_path) break;
		len = ((p = strchr(file_path,'/')) != NULL) ? (int)(p - file_path) : strlen(file_path);
		if (!iso9660_find_dir_entry(file_path, len, &sector, &size))
			return FALSE;
		file_path += len;
	}
	if (size > *data_size) size = *data_size;
	*data_size = size;
	for ( ; size > 0; data_buff += len, size -= len, sector++) {
		BYTE	read_buffer[RAW_SECTOR_SIZE], *pbuf;
		len = (size > COOKED_SECTOR_SIZE) ? COOKED_SECTOR_SIZE : size;
		if ((pbuf = iso9660_read_sector_cooked(read_buffer, sector)) == NULL)
			return FALSE;
		memcpy(data_buff, pbuf, len);
	}
	return TRUE;
}

static BOOL iso9660_add_new_dir(int extent, int size
#ifdef	_DEBUG
						 , const char *name, int name_len, int parent_dir
#endif//_DEBUG
						 )
{
	DIR_ENTRY	*dir;
#ifdef	_DEBUG
	int		tmp_file_link;
#endif//_DEBUG

	if (missing_dirs == NULL || num_missing_dirs >= MAX_NUM_DIR_ENTRIES)
		return FALSE;
#ifdef	_DEBUG
	tmp_file_link = num_found_files;	// save the current file idx (where 'iso9660_add_new_file()' puts the new entry)
#endif//_DEBUG
	if (!iso9660_add_new_file(extent, size
#ifdef	_DEBUG
		, name, name_len, parent_dir
#endif//_DEBUG
		))
		return FALSE;
	dir = &missing_dirs[num_missing_dirs++];
	dir->start_sector = extent;
	dir->end_sector = extent + ((size - 1) / ISOFS_BLOCK_SIZE);
#ifdef	_DEBUG
	dir->file_link = tmp_file_link;
#endif//_DEBUG
	return TRUE;
}

static BOOL iso9660_add_new_file(int extent, int size
#ifdef	_DEBUG
						  , const char *name, int name_len, int parent_dir
#endif//_DEBUG
						  )
{
	FILE_ENTRY	*fil;
#ifdef	_DEBUG
	NAME_BUFFERS	*nbuf;
#endif//_DEBUG

	if (found_files == NULL || num_found_files >= MAX_NUM_FILE_ENTRIES)
		return FALSE;
	fil = &found_files[num_found_files++];
	fil->start_sector = extent;
	fil->end_sector = extent + ((size - 1) / ISOFS_BLOCK_SIZE);
#ifdef	_DEBUG
	fil->dir_link = parent_dir;
	fil->file_name = NULL;
	if (file_names->name_offset + name_len < sizeof(file_names->name_buffer))
		fil->file_name = &file_names->name_buffer[file_names->name_offset];
	else if ((nbuf = (NAME_BUFFERS *)HeapAlloc(hProcessHeap, 0, sizeof(*nbuf))) != NULL) {
		nbuf->next_name_buffer = file_names;
		nbuf->name_offset = 0;
		file_names = nbuf;
		fil->file_name = &file_names->name_buffer[0];
	} else
		fil->file_name = NULL;
	if (fil->file_name != NULL) {
		strncpy(fil->file_name, name, name_len);
		fil->file_name[name_len] = '\0';
		file_names->name_offset += (name_len + 1);
	} else
		fil->file_name = "???";
#endif//_DEBUG
	return TRUE;
}

static void iso9660_update_sorted_files_index(void)
{
	int		i, j, k;
	WORD	new_files_list[ISOFS_BLOCK_SIZE / sizeof(ISO_DIRECTORY_RECORD)];
	int		num_new_files = num_found_files - last_sorted_file;

	// first, create the new entries list with the files added from previous sort
	for (i = 0, j = last_sorted_file; i < num_new_files; i++, j++)
		new_files_list[i] = j;
	// second, sort the new entries list by file location
	for (i = num_new_files >> 1; i > 0; i >>= 1)
		for (j = i; j < num_new_files; j++)
			for (k = j - i; k >= 0; k -= i)
            {
                WORD	tmp_idx;
				if (found_files[new_files_list[k]].start_sector < found_files[new_files_list[k + i]].end_sector)
					break;
				tmp_idx = new_files_list[k];
				new_files_list[k] = new_files_list[k + i];
				new_files_list[k + i] = tmp_idx;
			}
	// third, put the new entries into the 'sorted_files_idx'
	for (i = num_new_files - 1, j = last_sorted_file; i >= 0; i--)
	{
		int first = 0;
		int last = j - 1;
		int target = found_files[new_files_list[i]].start_sector;
		while (first <= last) {
			int mid = (first + last) / 2;
			if (found_files[sorted_files_idx[mid]].start_sector > target)
				last = mid - 1;
			else
				first = mid + 1;
		}
		memcpy(&sorted_files_idx[first + i + 1], &sorted_files_idx[first], (j - first) * sizeof(*sorted_files_idx));
		sorted_files_idx[first + i] = new_files_list[i];
		j = first;
	}
	last_sorted_file = num_found_files;
}

static void iso9660_flush_all(void)
{
	// flush all dir/file entries
	num_found_files = 0;
	num_missing_dirs = 0;
	last_sorted_file = 0;
	memset(&saved_iso_primary_descriptor, 0, sizeof(saved_iso_primary_descriptor));
#ifdef	_DEBUG
	if (file_names == NULL)
		return;
	// cleanup name buffers chain (leave only the first allocated entry)
	while (file_names->next_name_buffer != NULL)
	{
		NAME_BUFFERS	*tmp_namebuf = file_names;
		file_names = file_names->next_name_buffer;
		HeapFree(hProcessHeap, 0, tmp_namebuf);
	}
	file_names->name_offset = 0;
#endif//_DEBUG
}

#ifdef	_DEBUG
static void	PRINT_FILE_INFO(int file_idx)
{
	int		i, j, parent_dirs[16];

	for (i = 0, j = file_idx; i < 16 && j > 0; i++, j = found_files[j].dir_link)
		parent_dirs[i] = j;
	PRINTF(("File: /"));
	for (j = i - 1; j >= 0; j--)
		PRINTF(((j) ? "%s/" : "%s", found_files[parent_dirs[j]].file_name));
	PRINTF((" [%d - %d]\n", found_files[file_idx].start_sector, found_files[file_idx].end_sector));
}
#endif//_DEBUG

