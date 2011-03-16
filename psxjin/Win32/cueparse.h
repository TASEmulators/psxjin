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
	~CueData();
  std::map<int,CueTrack> tracks;
  int NumTracks();
  int MinTrack();    
  int MaxTrack();
  bool parse_cue(const char* buf);
  int cueparser(char* Filename);
};

