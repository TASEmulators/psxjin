#include "PsxCommon.h"
#include "emufile.h"

#include <vector>

/* little endian (ds' endianess) to local endianess convert macros */
#ifdef LOCAL_BE	/* local arch is big endian */
# define LE_TO_LOCAL_16(x) ((((x)&0xff)<<8)|(((x)>>8)&0xff))
# define LE_TO_LOCAL_32(x) ((((x)&0xff)<<24)|(((x)&0xff00)<<8)|(((x)>>8)&0xff00)|(((x)>>24)&0xff))
# define LE_TO_LOCAL_64(x) ((((x)&0xff)<<56)|(((x)&0xff00)<<40)|(((x)&0xff0000)<<24)|(((x)&0xff000000)<<8)|(((x)>>8)&0xff000000)|(((x)>>24)&0xff00)|(((x)>>40)&0xff00)|(((x)>>56)&0xff))
# define LOCAL_TO_LE_16(x) ((((x)&0xff)<<8)|(((x)>>8)&0xff))
# define LOCAL_TO_LE_32(x) ((((x)&0xff)<<24)|(((x)&0xff00)<<8)|(((x)>>8)&0xff00)|(((x)>>24)&0xff))
# define LOCAL_TO_LE_64(x) ((((x)&0xff)<<56)|(((x)&0xff00)<<40)|(((x)&0xff0000)<<24)|(((x)&0xff000000)<<8)|(((x)>>8)&0xff000000)|(((x)>>24)&0xff00)|(((x)>>40)&0xff00)|(((x)>>56)&0xff))
#else		/* local arch is little endian */
# define LE_TO_LOCAL_16(x) (x)
# define LE_TO_LOCAL_32(x) (x)
# define LE_TO_LOCAL_64(x) (x)
# define LOCAL_TO_LE_16(x) (x)
# define LOCAL_TO_LE_32(x) (x)
# define LOCAL_TO_LE_64(x) (x)
#endif

bool EMUFILE::readAllBytes(std::vector<u8>* dstbuf, const std::string& fname)
{
	EMUFILE_FILE file(fname.c_str(),"rb");
	if(file.fail()) return false;
	int size = file.size();
	dstbuf->resize(size);
	file.fread(&dstbuf->at(0),size);
	return true;
}

void EMUFILE::write64le(u64* val)
{
	write64le(*val);
}

void EMUFILE::write64le(u64 val)
{
#ifdef LOCAL_BE
	u8 s[8];
	s[0]=(u8)b;
	s[1]=(u8)(b>>8);
	s[2]=(u8)(b>>16);
	s[3]=(u8)(b>>24);
	s[4]=(u8)(b>>32);
	s[5]=(u8)(b>>40);
	s[6]=(u8)(b>>48);
	s[7]=(u8)(b>>56);
	fwrite((char*)&s,8);
	return 8;
#else
	fwrite(&val,8);
#endif
}


size_t EMUFILE::read64le(u64 *Bufo)
{
	u64 buf;
	if(fread((char*)&buf,8) != 8)
		return 0;
#ifndef LOCAL_BE
	*Bufo=buf;
#else
	*Bufo = LE_TO_LOCAL_64(buf);
#endif
	return 1;
}

u64 EMUFILE::read64le()
{
	u64 temp;
	read64le(&temp);
	return temp;
}

void EMUFILE::write32le(u32* val)
{
	write32le(*val);
}

void EMUFILE::write32le(u32 val)
{
#ifdef LOCAL_BE
	u8 s[4];
	s[0]=(u8)val;
	s[1]=(u8)(val>>8);
	s[2]=(u8)(val>>16);
	s[3]=(u8)(val>>24);
	fwrite(s,4);
#else
	fwrite(&val,4);
#endif
}

size_t EMUFILE::read32le(s32* Bufo) { return read32le((u32*)Bufo); }

size_t EMUFILE::read32le(u32* Bufo)
{
	u32 buf;
	if(fread(&buf,4)<4)
		return 0;
#ifndef LOCAL_BE
	*(u32*)Bufo=buf;
#else
	*(u32*)Bufo=((buf&0xFF)<<24)|((buf&0xFF00)<<8)|((buf&0xFF0000)>>8)|((buf&0xFF000000)>>24);
#endif
	return 1;
}

u32 EMUFILE::read32le()
{
	u32 ret;
	read32le(&ret);
	return ret;
}

void EMUFILE::write16le(u16* val)
{
	write16le(*val);
}

void EMUFILE::write16le(u16 val)
{
#ifdef LOCAL_BE
	u8 s[2];
	s[0]=(u8)val;
	s[1]=(u8)(val>>8);
	fwrite(s,2);
#else
	fwrite(&val,2);
#endif
}

size_t EMUFILE::read16le(s16* Bufo) { return read16le((u16*)Bufo); }

size_t EMUFILE::read16le(u16* Bufo)
{
	u32 buf;
	if(fread(&buf,2)<2)
		return 0;
#ifndef LOCAL_BE
	*(u16*)Bufo=buf;
#else
	*Bufo = LE_TO_LOCAL_16(buf);
#endif
	return 1;
}

u16 EMUFILE::read16le()
{
	u16 ret;
	read16le(&ret);
	return ret;
}

void EMUFILE::write8le(u8* val)
{
	write8le(*val);
}


void EMUFILE::write8le(u8 val)
{
	fwrite(&val,1);
}

size_t EMUFILE::read8le(u8* val)
{
	return fread(val,1);
}

u8 EMUFILE::read8le()
{
	u8 temp;
	fread(&temp,1);
	return temp;
}

void EMUFILE::writedouble(double* val)
{
	write64le(double_to_u64(*val));
}
void EMUFILE::writedouble(double val)
{
	write64le(double_to_u64(val));
}

double EMUFILE::readdouble()
{
	double temp;
	readdouble(&temp);
	return temp;
}

size_t EMUFILE::readdouble(double* val)
{
	u64 temp;
	size_t ret = read64le(&temp);
	*val = u64_to_double(temp);
	return ret;
}