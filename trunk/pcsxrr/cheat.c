#include "PsxCommon.h"
#include "cheat.h"
#include "movie.h"

#ifdef WIN32
#include "Win32.h"
#endif

void PCSXRemoveCheat (uint32 which1)
{
	if (Cheat.c [which1].saved)
	{
		uint32 address = Cheat.c[which1].address;

//		int block = (address >> MEMMAP_SHIFT) & MEMMAP_MASK;
//		uint8 *ptr = Memory.Map [block];

//		if (ptr >= (uint8 *) CMemory::MAP_LAST)
//			*(ptr + (address & 0xffff)) = Cheat.c [which1].saved_byte;
//		else
//			S9xSetByte (Cheat.c [which1].saved_byte, address);

		psxMemWrite8(address,Cheat.c[which1].saved_byte);
	}
}

void PCSXDeleteCheat (uint32 which1)
{
	if (which1 < Cheat.num_cheats)
	{
		if (Cheat.c[which1].enabled)
			PCSXRemoveCheat (which1);
		
		memmove (&Cheat.c[which1], &Cheat.c[which1 + 1],
		         sizeof (Cheat.c [0]) * (Cheat.num_cheats - which1 - 1));
		Cheat.num_cheats--; //MK: This used to set it to 0??
	}
}

void PCSXAddCheat (BOOL enable, BOOL save_current_value, 
                  uint32 address, uint8 byte)
{
	if (Cheat.num_cheats < sizeof (Cheat.c) / sizeof (Cheat.c[0]))
	{
		Cheat.c [Cheat.num_cheats].address = address;
		Cheat.c [Cheat.num_cheats].byte = byte;
		Cheat.c [Cheat.num_cheats].enabled = enable;
		if (save_current_value)
		{
			Cheat.c [Cheat.num_cheats].saved_byte = psxMs8(address);
			Cheat.c [Cheat.num_cheats].saved = TRUE;
		}
		Cheat.num_cheats++;
	}
}

void PCSXDisableCheat (uint32 which1)
{
	if (which1 < Cheat.num_cheats && Cheat.c[which1].enabled)
	{
		PCSXRemoveCheat(which1);
		Cheat.c[which1].enabled = FALSE;
	}
}

void PCSXApplyCheat(uint32 which1)
{
	uint32 address = Cheat.c[which1].address;

	if (!Cheat.c[which1].saved)
		Cheat.c[which1].saved_byte = psxMs8(address);

//	int block = (address >> MEMMAP_SHIFT) & MEMMAP_MASK;
//	uint8 *ptr = Memory.Map [block];
//
//	if (ptr >= (uint8 *) CMemory::MAP_LAST)
//		*(ptr + (address & 0xffff)) = Cheat.c [which1].byte;
//	else
//		S9xSetByte (Cheat.c [which1].byte, address);

	psxMemWrite8(address,Cheat.c[which1].byte);
	Cheat.c [which1].saved = TRUE;
}

void PCSXEnableCheat (uint32 which1)
{
	if (which1 < Cheat.num_cheats && !Cheat.c[which1].enabled)
	{
		Cheat.c[which1].enabled = TRUE;
		PCSXApplyCheat(which1);
	}
}

void PCSXApplyCheats()
{
	uint32 i;
	for (i = 0; i < Cheat.num_cheats; i++)
		if (Cheat.c[i].enabled)
			PCSXApplyCheat(i);
}

void PCSXRemoveCheats()
{
	uint32 i;
	for (i = 0; i < Cheat.num_cheats; i++)
	if (Cheat.c[i].enabled)
		PCSXRemoveCheat(i);
}

BOOL PCSXLoadCheatFile (const char *filename)
{
	Cheat.num_cheats = 0;

	FILE *fs = fopen (filename, "rb");
	uint8 data [28];

	if (!fs)
		return (FALSE);

	while (fread ((void *) data, 1, 28, fs) == 28)
	{
		Cheat.c [Cheat.num_cheats].enabled = (data [0] & 4) == 0;
		Cheat.c [Cheat.num_cheats].byte = data [1];
		Cheat.c [Cheat.num_cheats].address = data [2] | (data [3] << 8) |  (data [4] << 16);
		Cheat.c [Cheat.num_cheats].saved_byte = data [5];
		Cheat.c [Cheat.num_cheats].saved = (data [0] & 8) != 0;
		memmove (Cheat.c [Cheat.num_cheats].name, &data [8], 20);
		Cheat.c [Cheat.num_cheats++].name [20] = 0;
	}
	fclose (fs);

	return (TRUE);
}

BOOL PCSXSaveCheatFile (const char *filename)
{
	if (Cheat.num_cheats == 0)
	{
		(void) remove (filename);
		return (TRUE);
	}

	FILE *fs = fopen (filename, "wb");
	uint8 data [28];

	if (!fs)
	return (FALSE);

	uint32 i;
	for (i = 0; i < Cheat.num_cheats; i++)
	{
		memset (data, 0, 28);
		if (i == 0)
		{
			data [6] = 254;
			data [7] = 252;
		}
		if (!Cheat.c [i].enabled)
			data [0] |= 4;
	
		if (Cheat.c [i].saved)
			data [0] |= 8;
	
		data [1] = Cheat.c [i].byte;
		data [2] = (uint8) Cheat.c [i].address;
		data [3] = (uint8) (Cheat.c [i].address >> 8);
		data [4] = (uint8) (Cheat.c [i].address >> 16);
		data [5] = Cheat.c [i].saved_byte;
	
		memmove (&data [8], Cheat.c [i].name, 19);
		if (fwrite (data, 28, 1, fs) != 1)
		{
			fclose (fs);
			return (FALSE);
		}
	}
	return (fclose (fs) == 0);
}

void ScanAddress(const char* str, uint32 *value)
{
	if(tolower(*str) == 's') {
		sscanf(str+1, "%x", value);
		value += 0x7E0000 + 0x20000;
	}
	else if(tolower(*str) == 'i') {
		sscanf(str+1, "%x", value);
		value += 0x7E0000 + 0x30000;
	}
	else {
		int plus = (*str == '0' && tolower(str[1]) == 'x') ? 2 : 0;
		sscanf(str+plus, "%x", value);
	}
}

BOOL CHT_SaveCheatFileEmbed(const char *filename)
{
	if (Cheat.num_cheats == 0)
		return (FALSE);

	gzFile fs = gzopen(filename, "ab");
	uint8 data [28];

	if (!fs)
		return (FALSE);

	uint32 i;
	for (i = 0; i < Cheat.num_cheats; i++)
	{
		memset (data, 0, 28);
		if (i == 0)
		{
			data [6] = 254;
			data [7] = 252;
		}
		if (!Cheat.c [i].enabled)
			data [0] |= 4;

		if (Cheat.c [i].saved)
			data [0] |= 8;

		data [1] = Cheat.c [i].byte;
		data [2] = (uint8) Cheat.c [i].address;
		data [3] = (uint8) (Cheat.c [i].address >> 8);
		data [4] = (uint8) (Cheat.c [i].address >> 16);
		data [5] = Cheat.c [i].saved_byte;

		memmove (&data [8], Cheat.c [i].name, 19);

		if (gzwrite(fs, data, 28) <= 0)
		{
			gzclose (fs);
			return (FALSE);
		}
	}
	return (gzclose(fs) == 0);
}

BOOL CHT_LoadCheatFileEmbed(const char *filename)
{
	gzFile fs;
	FILE* fp;
	FILE* fp2;
	char embCheatTmp[Movie.inputOffset-Movie.cheatListOffset];
	fp = fopen(filename,"rb");
	fp2 = fopen("embcheat.tmp","wb");
	fseek(fp, Movie.cheatListOffset, SEEK_SET);
	fread(embCheatTmp, 1, sizeof(embCheatTmp), fp);
	fwrite(embCheatTmp, 1, sizeof(embCheatTmp), fp2);
	fclose(fp);
	fclose(fp2);

	Cheat.num_cheats = 0;

	fs = gzopen("embcheat.tmp", "rb");
	uint8 data [28];

	if (!fs)
		return (FALSE);

	while (gzread(fs, (void *) data, 28) == 28)
	{
		Cheat.c [Cheat.num_cheats].enabled = (data [0] & 4) == 0;
		Cheat.c [Cheat.num_cheats].byte = data [1];
		Cheat.c [Cheat.num_cheats].address = data [2] | (data [3] << 8) |  (data [4] << 16);
		Cheat.c [Cheat.num_cheats].saved_byte = data [5];
		Cheat.c [Cheat.num_cheats].saved = (data [0] & 8) != 0;
		memmove (Cheat.c [Cheat.num_cheats].name, &data [8], 20);
		Cheat.c [Cheat.num_cheats++].name [20] = 0;
	}
	gzclose(fs);
	remove("embcheat.tmp");

	return (TRUE);
}

void CHT_ClearCheatFileEmbed() {
	uint32 i;
	for (i = 0; i < Cheat.num_cheats; i++)
		PCSXDeleteCheat(i);
	Cheat.num_cheats=0;
}
