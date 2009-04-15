// Plugin Defs
#include "Locals.h"

//#define	DEBUG_CACHE_READ

// Cache buffers defs:
#define	NUM_CACHE_BUFFERS_SMALL		4	// number of 64K buffers x 'cache size small'
#define	NUM_CACHE_BUFFERS_LARGE		32	// number of 64K buffers x 'cache size large'

#define	MAX_CDROM_SECTOR_LOC	(74*60*75)	// max. sector location value for a PSX cdrom (the ending of a 74min. cdrom)

typedef struct SC_DATA_REC {
	int		loc;
	struct SC_DATA_REC	*next;
	Q_SUBCHANNEL_DATA	q;
} SC_DATA_REC;

typedef struct SC_DATA_INDEX {
	int		loc;
	SC_DATA_REC	*data;
} SC_DATA_INDEX;

// Cache data:
static eCacheLevel	cur_cache_level = CacheLevel_Disabled;
static int	cur_cache_size = 0;
static BOOL	cur_track_fsys = FALSE;
static BOOL	cache_toc_is_valid = FALSE;
static CACHE_BUFFER_LIST	cache_buffers_list;
static CACHE_BUFFER	*last_cache_buffer = NULL;	// cache buffer used on last sector read
static BOOL	cache_read_pending = FALSE;	// called 'start_read_sector_to_cache()', and waiting for 'finish_read_sector_to_cache()'
static BOOL	cache_last_read_esito = FALSE;	// result of last sector read op (TRUE if no read error)
static int	last_data_sector;			// last CD data sector (from TOC)
static int	last_failed_multi_read_start;
static int	last_failed_multi_read_end;
static int	file_start_loc;
static int	file_end_loc;
// Async reads handling:
static int	async_last_req_loc = -1;	// last sector read loc (for async trigger handling)
static int	async_cache_hit_count = 0;	// seq. sector reads hit counter
static int	async_cache_hit_trigger;
static int	async_cache_prefetch_size = 0;
static BOOL	async_read_pending = FALSE;
static BOOL	async_read_finished = FALSE;
static BOOL	async_first_read_triggered = FALSE;
// Subchannel read:
static int	subq_loc_offset = -1;
static SC_DATA_REC	*sc_data_list = NULL;
static SC_DATA_INDEX	*sc_data_index = NULL;
static int	sc_data_entries = 0;
// Async worker thread data:
static BOOL	async_read_esito = FALSE;
static int	async_thread_running = 0;	// 0 = stopped, 1 = running, -1 = stop request
static HANDLE	async_start_event = NULL;
static HANDLE	async_wait_event = NULL;


// Pending cache read data:
static BYTE	*cache_read_sector_buffer;
static int	cache_read_sector_loc;
static int	cache_read_num_sectors;

static CACHE_BUFFER *GET_CACHE_BUFFER(CACHE_BUFFER *cb, bool from_list_top);
static void PUT_CACHE_BUFFER(CACHE_BUFFER *cb, bool to_list_top);
static void cache_UnloadSubFile(void);
static BOOL	load_sbi_file(FILE *fp);
static BOOL	load_m3s_file(FILE *fp);
static BOOL scan_sub_file_data(Q_SUBCHANNEL_DATA *subq_buf, int loc);
static int	compute_num_sectors_to_read(int loc, CACHE_BUFFER *cb);
static BOOL	get_readed_sector_from_cache(int loc, BYTE **sector_buf);
static void init_next_read_sector_cache_buffer(int loc, BYTE **sector_buf);
static BOOL start_next_read_sector_to_cache_buffer(void);
static BOOL	start_read_sector_to_cache(int loc, BYTE **sector_buf);
static BOOL	finish_read_sector_to_cache(void);
static BOOL	start_async_read_sector_to_cache(int loc, BYTE **sector_buf);
static BOOL	finish_async_read_sector_to_cache(void);
static BOOL start_async_read_thread(void);
static void stop_async_read_thread(void);
static void async_read_thread_main(void *);


static CACHE_BUFFER *GET_CACHE_BUFFER(CACHE_BUFFER *cb, bool from_list_top)
{
	if (cb != NULL) {
		if (cache_buffers_list.top == cb)
			cache_buffers_list.top = cb->next;
		if (cache_buffers_list.bottom == cb)
			cache_buffers_list.bottom = cb->prev;
		if (cb->prev != NULL)
			cb->prev->next = cb->next;
		if (cb->next != NULL)
			cb->next->prev = cb->prev;
	} else if (from_list_top) {
		if ((cb = cache_buffers_list.top) != NULL) {
			if ((cache_buffers_list.top = cb->next) != NULL)
				cache_buffers_list.top->prev = NULL;
			else
				cache_buffers_list.bottom = NULL;
			cb->prev = cb->next = NULL;
		}
	} else {	//from_list_top
		if ((cb = cache_buffers_list.bottom) != NULL) {
			if ((cache_buffers_list.bottom = cb->prev) != NULL)
				cache_buffers_list.bottom->next = NULL;
			else
				cache_buffers_list.top = NULL;
			cb->prev = cb->next = NULL;
		}
	}
	return cb;
}

static void PUT_CACHE_BUFFER(CACHE_BUFFER *cb, bool to_list_top)
{
	if (to_list_top) {
		cb->next = cache_buffers_list.top;
		cb->prev = NULL;
		if (cache_buffers_list.top != NULL)
			cache_buffers_list.top->prev = cb;
		cache_buffers_list.top = cb;
		if (cache_buffers_list.bottom == NULL)
			cache_buffers_list.bottom = cb;
	} else {	//to_list_top
		cb->prev = cache_buffers_list.bottom;
		cb->next = NULL;
		if (cache_buffers_list.bottom != NULL)
			cache_buffers_list.bottom->next = cb;
		cache_buffers_list.bottom = cb;
		if (cache_buffers_list.top == NULL)
			cache_buffers_list.top = cb;
	}
}



BOOL cache_Alloc(const CFG_DATA *cfg)
{
	eCacheLevel	cache_level = cfg->cache_level;
	int			cache_size = NUM_CACHE_BUFFERS_LARGE;
	BOOL		track_fsys = cfg->track_fsys;
	CACHE_BUFFER	*cb;

	// Ensure that the values in 'cache_level' & 'cache_size' are OK
	switch (cache_level) {
	case CacheLevel_Disabled:
		// Reset 'cur_cache_level' & 'cur_track_fsys' values & exit
		cur_cache_level = CacheLevel_Disabled;
		cur_cache_size = 0;
		cur_track_fsys = FALSE;
		return TRUE;
	case CacheLevel_ReadOne:
		cache_size = NUM_CACHE_BUFFERS_SMALL;
		track_fsys = FALSE;
		break;
	case CacheLevel_ReadMulti:
	case CacheLevel_ReadAsync:
		break;
	default:
		cache_level = CacheLevel_ReadAsync;
		break;
	}

	// Note: The 'Async Read' feature uses a 'worker thread' for read prefetching
	if (cache_level == CacheLevel_ReadAsync) {
		if ((cfg->interface_type != InterfaceType_Aspi &&
			 cfg->interface_type != InterfaceType_NtSpti) ||
			!start_async_read_thread())	// for ASPI / NT SPTI, use 'worker thread' for async reads
			cache_level = CacheLevel_ReadMulti;	// for NT RAW / MSCDEX, don't use async reads
	}

	while ((cb = GET_CACHE_BUFFER(NULL, TRUE)) != NULL)
		HeapFree(hProcessHeap, 0, cb);
	// Allocate cache buffers for sector data reads
	for (int i = 0; i < cache_size; i++) {
		if ((cb = (CACHE_BUFFER *)HeapAlloc(hProcessHeap, 0, sizeof(CACHE_BUFFER))) == NULL)
			return FALSE;
		PUT_CACHE_BUFFER(cb, FALSE);
	}
	// Allocate data buffers for ISO9660 filesystem tracking
	if (track_fsys && !iso9660_Alloc())
		track_fsys = FALSE;				// if alloc failed, disable fsys tracking

	async_cache_hit_trigger = MAX_SECTORS_PER_READ * cfg->cache_asynctrig/100;
	if (async_cache_hit_trigger < (MAX_SECTORS_PER_READ * 25/100))
		async_cache_hit_trigger = (MAX_SECTORS_PER_READ * 25/100);
	else if (async_cache_hit_trigger > (MAX_SECTORS_PER_READ * 75/100))
		async_cache_hit_trigger = (MAX_SECTORS_PER_READ * 75/100);

	// Save current 'cache_level', 'cache_size' & 'track_fsys' values
	cur_cache_level = cache_level;
	cur_cache_size = cache_size;
	cur_track_fsys = track_fsys;
	cache_FlushBuffers();
	return TRUE;
}

void cache_Free(void)
{
	CACHE_BUFFER	*cb;

	// Finish any pending read operation
	if (async_read_pending)
		finish_async_read_sector_to_cache();
	if (cache_read_pending)
		finish_read_sector_to_cache();
	cache_read_pending = async_read_pending = FALSE;
	cache_last_read_esito = FALSE;
	stop_async_read_thread();
	// If needed, free subchannel file data
	cache_UnloadSubFile();
	// Free data buffers for ISO9660 filesystem tracking
	if (cur_track_fsys)
		iso9660_Free();
	// Free cache buffers
	while ((cb = GET_CACHE_BUFFER(NULL, TRUE)) != NULL)
		HeapFree(hProcessHeap, 0, cb);
	// Reset the current 'cache_level' & 'track_fsys' values
	cur_cache_level = CacheLevel_Disabled;
	cur_track_fsys = FALSE;
}

void cache_FlushBuffers(void)
{
	CACHE_BUFFER	*cb;

	PRINTF((" CFLUSH"));
	// Invalidate the cached sectors data buffer (if allocated)
	for (cb = cache_buffers_list.top; cb != NULL; cb = cb->next)
		cb->num_loc_slots = cb->num_data_slots = 0;
	// reset drive ready flag (see drive_IsReady())
	drive_is_ready = FALSE;
	// Invalidate cached TOC data & Subchannel offset adjust
	cache_toc_is_valid = FALSE;
	subq_loc_offset = -1;
	// Reset last data sector location to the ending of a 74min. cdrom
	last_data_sector = MAX_CDROM_SECTOR_LOC;
	// Reset failed multi-read sectors list & iso9660 last file location
	last_failed_multi_read_start = file_start_loc = last_data_sector;
	last_failed_multi_read_end = file_end_loc = -1;
	// Reset async cache hit counter & location
	//async_cache_hit_count = async_cache_hit_trigger;
	async_last_req_loc = -1;
	async_first_read_triggered = FALSE;
	// Invalidate 'last_cache_buffer' (prevents a pending sector read to be cached)
	last_cache_buffer = NULL;
}

BOOL cache_ReadToc(BOOL use_cache)
{
	if (cache_toc_is_valid && use_cache)
		return TRUE;
	PRINTF((" RTOC"));
	if (!if_layer.ReadToc())
		return FALSE;
	cache_toc_is_valid = TRUE;			// no further *_ReadToc() neededs
	// save the address of the last DATA sector (TrackData[1] = first audio track / leadout location)
	last_data_sector = MSF2INT((MSF *)&toc.TrackData[1].Address[1]) - 1;
	return TRUE;
}

BOOL cache_StartReadSector(int loc, BYTE **sector_buf)
{
	// if cache not inizialized properly, return with error
	if (cur_cache_level == CacheLevel_Disabled || cache_buffers_list.top == NULL)
		return FALSE;

	if (cache_read_pending)				// ensure no missing '*_WaitEndReadSector()' call
		if_layer.WaitEndReadSector();
	if (cur_track_fsys && (loc < file_start_loc || loc > file_end_loc))
		iso9660_get_sector(loc, &file_start_loc, &file_end_loc);	// track file system access

	// complete async read pending if already finished
	if (async_read_pending && async_read_finished) {
		finish_async_read_sector_to_cache();
		async_read_pending = FALSE;
#ifdef	DEBUG_CACHE_READ
		PRINTF((" <a>"));
#endif//DEBUG_CACHE_READ
	}

	// check if the sector is already in cache
	if (get_readed_sector_from_cache(loc, sector_buf) ||
		// if not in cache, wait until any pending async read has finished, and check the cache again
		(async_read_pending &&
		 (async_read_pending = FALSE, finish_async_read_sector_to_cache()) &&
		 (DWORD)(loc - cache_read_sector_loc) < (DWORD)cache_read_num_sectors &&
		 get_readed_sector_from_cache(loc, sector_buf))) {
#ifdef	DEBUG_CACHE_READ
		PRINTF((" C%d", loc));
#endif//DEBUG_CACHE_READ
		cache_read_pending = FALSE;
		// check if the sector data contains an unreaded directory entry
		if (cur_track_fsys) iso9660_put_sector(*sector_buf, loc);
		return cache_last_read_esito = TRUE;
	}
	// if the sector is not in cache, start reading it into a cache buffer
#ifdef	DEBUG_CACHE_READ
	PRINTF((" R%d", loc));
#endif//DEBUG_CACHE_READ
	if (!start_read_sector_to_cache(loc, sector_buf))
		return cache_last_read_esito = FALSE;

	cache_read_pending = TRUE;
	return cache_last_read_esito = TRUE;
}

BOOL cache_WaitEndReadSector(void)
{
	// if cache not inizialized properly, return with error
	if (cur_cache_level == CacheLevel_Disabled || cache_buffers_list.top == NULL)
		return FALSE;

	if (!cache_read_pending)
		return cache_last_read_esito;
	cache_read_pending = FALSE;
	if (!finish_read_sector_to_cache())
		return cache_last_read_esito = FALSE;
	// check if the sector data contains an unreaded directory entry
	if (cur_track_fsys) iso9660_put_sector(cache_read_sector_buffer, cache_read_sector_loc);
	return cache_last_read_esito = TRUE;
}

void cache_FinishAsyncReadsPending(void)
{
	// Finish any pending read operation
	if (async_read_pending)
		finish_async_read_sector_to_cache();
	if (cache_read_pending)
		finish_read_sector_to_cache();
	cache_read_pending = async_read_pending = FALSE;
}

BOOL cache_ReadSubQ(Q_SUBCHANNEL_DATA *subq_buf, int loc)
{
#ifdef	DEBUG_CACHE_READ
	PRINTF((" S%d", loc));
#endif//DEBUG_CACHE_READ
	memset(subq_buf, 0, sizeof(*subq_buf));
// loc == -1 -> ret play location
	if (loc < 0) {
		MSF		play_loc;
		BYTE	audio_status;

		if (!if_layer.GetPlayLocation(&play_loc, &audio_status))
			return FALSE;
		scsi_BuildSubQData(subq_buf, MSF2INT(&play_loc));
		return TRUE;
	}
// loc >= 0  -> ret Q Subchannel data
	if (sc_data_list != NULL) {			// if loaded, scan subchannel file (SBI/M3S)
		if (!scan_sub_file_data(subq_buf, loc))
			scsi_BuildSubQData(subq_buf, loc);	// if sector not found, ret 'default' data
		return TRUE;
	}

	// if subchannel read disabled, can't do anything
	if (SUBQ_MODE(scsi_subQmode) == SubQMode_Disabled)
		return FALSE;

	// if subchannel offset not available, try to find it...
	if (subq_loc_offset < 0) {
		Q_SUBCHANNEL_DATA	tmp_buf;
		int		loc, delta;

		for (loc = 16, delta = 0; loc < (16 + 3); loc++) {
			// read sector #n & check if AbsAddress[] > n
			memset(&tmp_buf, 0, sizeof(tmp_buf));
			if (!if_layer.ReadSubQ(&tmp_buf, loc))
				return FALSE;
			// check if 'tmp_buf' contains a valid subQ data
			if (!scsi_ValidateSubQData(&tmp_buf))
				continue;	// if not valid, retry with next sector (max. 3 times)
			if ((delta = MSFBCD2INT((MSF *)&tmp_buf.AbsoluteAddress[0]) - loc) < 0 || delta > 3)
				delta = 0;	// use the 'delta' value only when in range 0-3 frames
			break;
		}
		subq_loc_offset = delta;	// save subchannel offset
	}
	// reads subchannel data (from requested sector - offset adjust)
	if (loc < subq_loc_offset) {
		scsi_BuildSubQData(subq_buf, loc);
		return TRUE;
	}
	return if_layer.ReadSubQ(subq_buf, loc - subq_loc_offset);
}

// Load subchannel file if present (called from 'drive_IsReady()')
void cache_LoadSubFile(const char *psx_boot_file_name)
{
	const char	*search_dirs[] = { ".", "patches" };
	const char	*search_exts[] = { ".sbi", ".m3s" };
	int		idxd, idxe;
	FILE	*fp;
	char	buffer[50];
	BOOL	res;

	cache_UnloadSubFile();				// free any previous readed data
	if (!psx_boot_file_name[0])
		return;							// leave if no boot file specified (failed reading 'system.cnf' file)

	PRINTF(("Loading SUB file %s\n", psx_boot_file_name));
	for (idxe = 0; idxe < (sizeof(search_exts) / sizeof(search_exts[0])); idxe++) {
		for (idxd = 0; idxd < (sizeof(search_dirs) / sizeof(search_dirs[0])); idxd++) {
			sprintf(buffer, "%s\\%s%s",
				search_dirs[idxd], psx_boot_file_name, search_exts[idxe]);
			if ((fp = fopen(buffer, "rb")) == NULL)
				continue;
			fread(buffer, 4, 1, fp);
			if (!strcmp(buffer, "SBI"))
				res = load_sbi_file(fp);
			else
				res = load_m3s_file(fp);
			fclose(fp);
			if (res)
				return;
			cache_UnloadSubFile();
		}
	}
}

static void cache_UnloadSubFile(void)
{
	SC_DATA_REC	*cur, *next;

	for (cur = sc_data_list; cur != NULL; cur = next) {
		next = cur->next;
		HeapFree(hProcessHeap, 0, cur);
	}
	sc_data_list = NULL;
	if (sc_data_index)
		HeapFree(hProcessHeap, 0, sc_data_index);
	sc_data_index = NULL;
	sc_data_entries = 0;
}

static BOOL load_sbi_file(FILE *fp)
{
	char	buffer[4];
	SC_DATA_REC	*data, *prev;

	fseek(fp, 4, SEEK_SET);
	for (sc_data_entries = 0, prev = NULL; fread(buffer, 4, 1, fp) == 1; sc_data_entries++, prev = data)
	{
		if ((data = (SC_DATA_REC *)HeapAlloc(hProcessHeap, 0, sizeof(*data))) == NULL)
			return FALSE;
		data->loc = MSFBCD2INT((MSF *)buffer);
		data->next = NULL;
		scsi_BuildSubQData(&data->q, data->loc);
		switch (buffer[3]) {
		case 1: fread(&data->q, 10, 1, fp); break;
		case 2: fread(&data->q.TrackRelativeAddress[0], 3, 1, fp); break;
		case 3: fread(&data->q.AbsoluteAddress[0], 3, 1, fp); break;
		default: break;
		}
		if (prev == NULL)
			sc_data_list = data;
		else
			prev->next = data;
	}
	if (sc_data_entries != 0) {
		int		i;
		if ((sc_data_index = (SC_DATA_INDEX *)HeapAlloc(hProcessHeap, 0, sc_data_entries * sizeof(*sc_data_index))) == NULL)
			return FALSE;
		for (i = 0, data = sc_data_list; i < sc_data_entries; i++, data = data->next) {
			sc_data_index[i].loc = data->loc;
			sc_data_index[i].data = data;
		}
	}
	return TRUE;
}

static BOOL load_m3s_file(FILE *fp)
{
	int		i;

	fseek(fp, 0, SEEK_END);
	if (ftell(fp) != (60 * 75) * sizeof(Q_SUBCHANNEL_DATA))
		return FALSE;
	fseek(fp, 0, SEEK_SET);
	sc_data_entries = (60 * 75);
	if ((sc_data_list = (SC_DATA_REC *)HeapAlloc(hProcessHeap, 0, sc_data_entries * sizeof(sc_data_list))) == NULL)
		return FALSE;
	sc_data_list->next = NULL;	// linked list is not used !!!
	if ((sc_data_index = (SC_DATA_INDEX *)HeapAlloc(hProcessHeap, 0, sc_data_entries * sizeof(*sc_data_index))) == NULL)
		return FALSE;
	for (i = 0; i < sc_data_entries; i++) {
		sc_data_list[i].loc = (3 * 60 * 75) + i;
		sc_data_list[i].next = NULL;
		fread(&sc_data_list[i].q, sizeof(Q_SUBCHANNEL_DATA), 1, fp);
		sc_data_index[i].loc = sc_data_list[i].loc;
		sc_data_index[i].data = &sc_data_list[i];
	}
	return TRUE;
}

static BOOL scan_sub_file_data(Q_SUBCHANNEL_DATA *subq_buf, int loc)
{
	int first = 0;
	int last = sc_data_entries - 1;
	while (first <= last) {
		int mid = (first + last) / 2;
		if (sc_data_index[mid].loc == loc) {
			SC_DATA_REC	*data = sc_data_index[mid].data;
			memcpy(subq_buf, &data->q, sizeof(subq_buf));
			return TRUE;
		}
		if (sc_data_index[mid].loc > loc)
			last = mid - 1;
		else
			first = mid + 1;
	}
	return FALSE;
}

static int compute_num_sectors_to_read(int loc, CACHE_BUFFER *cb)
{
	int		nsec, max_nsec;

	if (cur_cache_level == CacheLevel_ReadOne)
		return 1;
	// if the sector was in a previous 'read-multi failed' range, force a single-sector read
	if (loc >= last_failed_multi_read_start && loc <= last_failed_multi_read_end)
		return 1;
	// compute the maximum number of sectors to be readed
	max_nsec = MAX_SECTORS_PER_READ - cb->num_data_slots;
	// ensure that the sectors to be read aren't outside the cdrom data area
	if ((loc + max_nsec) > last_data_sector && loc <= last_data_sector)
		max_nsec = last_data_sector - loc;
	// Note: When reading a file, PSX stops reading a sector too late, and i'll need ...
	// ... to add also this extra sector when computing the remaining sectors to read
	if (cur_track_fsys &&				// if file system tracking enabled ...
		((loc >= file_start_loc && loc <= file_end_loc) ||
		 iso9660_get_sector(loc, &file_start_loc, &file_end_loc)) &&
		(nsec = file_end_loc - loc + 1 + 1) < max_nsec)	// (the second '+1' is for the PSX extra sector readed)
		max_nsec = nsec;				// ... stop reading at the ending of the file
	return max_nsec;
}

static BOOL get_readed_sector_from_cache(int loc, BYTE **sector_buf)
{
	int		i, j;
	CACHE_BUFFER	*cb;
	CACHE_LOC	*cl;
	int		next_loc;
	BYTE	*dummy_sector_buf;

	for (cb = cache_buffers_list.top; cb != NULL; cb = cb->next)
		for (cl = cb->data_loc, i = j = 0; i < cb->num_loc_slots; i++, j+=cl->num_sectors, cl++)
			if ((DWORD)(loc - cl->start_loc) < (DWORD)cl->num_sectors) {
		// move buffer on cache list top
		if (cb->prev != NULL) {
			GET_CACHE_BUFFER(cb, FALSE);
			PUT_CACHE_BUFFER(cb, TRUE);
		}
		// return the sector found in the cached data
		*sector_buf = cb->data[j + (loc - cl->start_loc)];
		if (cur_cache_level != CacheLevel_ReadAsync ||	// async reads disabled
			loc == async_last_req_loc)	// duplicate sector reads doesn't be computed as async cache hit
			return TRUE;
		// ensure all the data sectors are readed in sequence, ...
		// ... otherwise stop & restart the async cache read prefetching
		if (loc != (async_last_req_loc + 1)) {	// readed sector not in sequence, reset async trigger info
			async_cache_hit_count = async_cache_hit_trigger + 1;
			async_cache_prefetch_size = 0;
			async_first_read_triggered = FALSE;
		}
		async_last_req_loc = loc;		// update async cache location
		// check async hit trigger to increase prefetch size
		if (async_cache_hit_count && !--async_cache_hit_count &&
			async_cache_prefetch_size < (cur_cache_size / 4) * MAX_SECTORS_PER_READ) {
			async_cache_hit_count = async_cache_hit_trigger;
			async_cache_prefetch_size += MAX_SECTORS_PER_READ;
		}
		// ensure prefetch is triggered and no async read pending
		if (async_cache_prefetch_size == 0 || async_read_pending)
			return TRUE;				// ret if prefetch not yet triggered, or async read already pending
		// compute next sector loc to be read: get from cache info if async reading ...
		// ... not yet triggered, or use last readed sector info (from last async read request)
		next_loc = (!async_first_read_triggered) ?
			cl->start_loc + cl->num_sectors :
			cache_read_sector_loc + cache_read_num_sectors;
		// check if next data block needs to be prefetched
		if ((next_loc - loc) > async_cache_prefetch_size)
			return TRUE;
		if (cur_track_fsys &&			// if file system tracking enabled ...
			loc >= file_start_loc && loc <= file_end_loc &&
			next_loc > file_end_loc)	// ... stop async read when end of file is reached
			return TRUE;
		// start async read request
#ifdef	DEBUG_CACHE_READ
		PRINTF((" <A%d>", next_loc));
#endif//DEBUG_CACHE_READ
		async_read_pending = TRUE;
		async_read_finished = FALSE;
		async_first_read_triggered = TRUE;
		start_async_read_sector_to_cache(next_loc, &dummy_sector_buf);
		return TRUE;
	}
	// sector not in cache: reset async cache hit counter & location
	if (!async_read_pending) {
		async_cache_hit_count = async_cache_hit_trigger;
		async_cache_prefetch_size = 0;
		async_last_req_loc = loc;
	}
	return FALSE;
}

static void init_next_read_sector_cache_buffer(int loc, BYTE **sector_buf)
{
	CACHE_BUFFER	*cb = cache_buffers_list.top;

	// find next free cache buffer, attempting to reuse the most recent cache buffer if not full
	// Warning: 'num_data_slots == 0' means than the buffer has been 'flushed' ...
	// ... and now is free, but it cannot be reused since FPSE10 doesn't like ...
	// ... than 2 consecutive reads should return the same sector buffer.
	if (last_cache_buffer == NULL ||
		(cb = cache_buffers_list.top)->num_data_slots == 0 || cb->num_data_slots >= MAX_SECTORS_PER_READ) {
		// 'last_cache_buffer' was invalid or empty/full, use a new buffer
		cb = GET_CACHE_BUFFER(NULL, FALSE);	// grab a buffer from list bottom ...
		PUT_CACHE_BUFFER(cb, TRUE);		// ... and put it back to list top
		cb->num_loc_slots = cb->num_data_slots = 0;	// clear cache buffer entry
		last_cache_buffer = cb;			// remember last cache buffer used
	}
	cache_read_num_sectors = compute_num_sectors_to_read(loc, cb);
	cache_read_sector_loc = loc;
	*sector_buf = cache_read_sector_buffer = cb->data[cb->num_data_slots];
}

static BOOL start_next_read_sector_to_cache_buffer(void)
{
	if (!if_layer.StartReadSector(cache_read_sector_buffer, cache_read_sector_loc, cache_read_num_sectors)) {
		PRINTF((" read-%s failed!\n", (cache_read_num_sectors == 1) ? "single" : "multi"));
		if (cache_read_num_sectors == 1)
			return FALSE;				// DO NOT cache failed read operations
		// save failed read location to prevent further multi-sector reads on the same zone
		last_failed_multi_read_start = cache_read_sector_loc;
		last_failed_multi_read_end = cache_read_sector_loc + cache_read_num_sectors - 1;
		// if multi-sector read failed, try with a single-sector read now
		if (!if_layer.StartReadSector(cache_read_sector_buffer, cache_read_sector_loc, cache_read_num_sectors = 1))
			return FALSE;				// DO NOT cache failed read operations
	}
	return TRUE;
}

static BOOL start_read_sector_to_cache(int loc, BYTE **sector_buf)
{
	init_next_read_sector_cache_buffer(loc, sector_buf);
	return start_next_read_sector_to_cache_buffer();
}

static BOOL finish_read_sector_to_cache(void)
{
	CACHE_BUFFER	*cb;
	CACHE_LOC	*cl;

	if (pending_read_running && !if_layer.WaitEndReadSector()) {
		PRINTF((" read-%s failed!\n", (cache_read_num_sectors == 1) ? "single" : "multi"));
		if (cache_read_num_sectors == 1)
			return FALSE;				// DO NOT cache failed read operations
		// save failed read location to prevent further multi-sector reads on the same zone
		last_failed_multi_read_start = cache_read_sector_loc;
		last_failed_multi_read_end = cache_read_sector_loc + cache_read_num_sectors - 1;
		// if multi-sector read failed, try with a single-sector read (with wait) now
		if (!if_layer.StartReadSector(cache_read_sector_buffer, cache_read_sector_loc, cache_read_num_sectors = 1) ||
			(pending_read_running && !if_layer.WaitEndReadSector()))
		return FALSE;					// DO NOT cache failed read operations
	}
	// ensure that the cache was not cleaned up during sector read (see cache_FlushBuffers()).
	if (last_cache_buffer == NULL ||
		cache_read_sector_buffer != (cb = last_cache_buffer)->data[cb->num_data_slots])
		return TRUE;					// if the cache was cleaned up, DO NOT cache readed data !!!
	cl = &cb->data_loc[cb->num_loc_slots++];
	cl->start_loc = cache_read_sector_loc;
	cl->num_sectors = cache_read_num_sectors;
	cb->num_data_slots += cache_read_num_sectors;
	return TRUE;
}

static BOOL start_async_read_sector_to_cache(int loc, BYTE **sector_buf)
{
	if (async_thread_running <= 0)
		return FALSE;
//	if (async_read_pending)
//		finish_async_read_sector_to_cache();
	init_next_read_sector_cache_buffer(loc, sector_buf);
//	ResetEvent(async_wait_event);
	SetEvent(async_start_event);
	return TRUE;
}

static BOOL finish_async_read_sector_to_cache(void)
{
	if (async_thread_running <= 0)
		return FALSE;
	WaitForSingleObject(async_wait_event, INFINITE);
	return async_read_esito;
}

static BOOL start_async_read_thread(void)
{
	async_start_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	async_wait_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	async_thread_running = 1;			// RUNNING state
	if (_beginthread(async_read_thread_main, 0, NULL) == (DWORD)-1) {
		async_thread_running = 0;		// STOPPED state
		stop_async_read_thread();
		return FALSE;					// (failed to start worker thread)
	}
	return TRUE;
}

static void stop_async_read_thread(void)
{
	if (async_thread_running > 0) {
		async_thread_running = -1;		// STOP request
		SetEvent(async_start_event);
		while (async_thread_running != 0)
			Sleep(50);					// wait until thread stopped
	}
	if (async_start_event != NULL)
		CloseHandle(async_start_event);
	if (async_wait_event != NULL)
		CloseHandle(async_wait_event);
	async_start_event = async_wait_event = NULL;
}

static void async_read_thread_main(void *)
{
//	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	for (;;) {
		WaitForSingleObject(async_start_event, INFINITE);
		if (async_thread_running < 0)
			break;						// exit loop if STOP request
		async_read_esito = TRUE;
		if (!start_next_read_sector_to_cache_buffer())
			async_read_esito = FALSE;
		if (!finish_read_sector_to_cache())
			async_read_esito = FALSE;
		async_read_finished = TRUE;
		SetEvent(async_wait_event);
	}
	async_thread_running = 0;
}
