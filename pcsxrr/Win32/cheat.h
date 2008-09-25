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
    struct SCheat c [MAX_CHEATS];
    uint32	    num_cheats;
    uint8	    CWRAM [0x20000];
    uint8	    CSRAM [0x10000];
    uint8	    CIRAM [0x2000];
    uint8           *RAM;
    uint8           *FillRAM;
    uint8           *SRAM;
    uint32	    WRAM_BITS [0x20000 >> 3];
    uint32	    SRAM_BITS [0x10000 >> 3];
    uint32	    IRAM_BITS [0x2000 >> 3];
};

void CreateCheatEditor();
void PCSXApplyCheats();
void PCSXRemoveCheats();
