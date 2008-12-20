struct SCheat
{
	uint32  address;
	uint8   byte;
	uint8   saved_byte;
	uint8   enabled;
	uint8   saved;
	char    name [22];
};

#define MAX_CHEATS 75

struct SCheatData
{
	struct SCheat  c [MAX_CHEATS];
	uint32         num_cheats;
	u8             CRAM [0x200000];
	u8             *RAM;
	s32            ALL_BITS [(0x200000 >> 5)];
	s8             CWatchRAM [0x200000];
};

void PCSXApplyCheats();
void PCSXRemoveCheats();
void PCSXAddCheat(BOOL enable, BOOL save_current_value, uint32 address, uint8 byte);
void ScanAddress(const char* str, uint32 *value);
BOOL CHT_SaveCheatFileEmbed(const char *filename);
BOOL CHT_LoadCheatFileEmbed(const char *filename);
void CHT_ClearCheatFileEmbed();
