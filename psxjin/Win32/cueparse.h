#ifndef _CUEPARSE_H_
#define _CUEPARSE_H_

#include <string>
#include <map>

class CueTimestamp
{
public:
	CueTimestamp();
  union {
    struct {
      int mm,ss,ff;
    };
    int parts[3];
  };
};

class CueTrack
{
public:
  CueTrack();
  std::string filename, filetype;
  std::string tracktype;
  std::map<int,CueTimestamp> indexes;
};





class CueData
{
public:
	CueData();	
  std::map<int,CueTrack> tracks;
  int NumTracks();
  int MinTrack();    
  int MaxTrack();  
  bool parse_cue(const char* buf);
  int cueparser(char* Filename);
  void CopyToConfig();
};



#endif //_CUEPARSE_H_
