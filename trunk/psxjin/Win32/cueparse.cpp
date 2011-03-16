// cueparse.cpp : Defines the entry point for the console application.
//

#define _CRT_SECURE_NO_WARNINGS
//#include "stdafx.h"
#include <stdio.h>
#include <string>
#include <string.h>
#include <vector>
#include <ctype.h>
#include <algorithm>
#include <map>
#include "cueparse.h"
#include "PsxCommon.h"

/// \brief convert input string into vector of string tokens
///
/// \note consecutive delimiters will be treated as single delimiter
/// \note delimiters are _not_ included in return data
///
/// \param input string to be parsed
/// \param delims list of delimiters.

std::vector<std::string> tokenize_str(const std::string & str,
                                      const std::string & delims=", \t")
{
  using namespace std;
  // Skip delims at beginning, find start of first token
  string::size_type lastPos = str.find_first_not_of(delims, 0);
  // Find next delimiter @ end of token
  string::size_type pos     = str.find_first_of(delims, lastPos);

  // output vector
  vector<string> tokens;

  while (string::npos != pos || string::npos != lastPos)
    {
      // Found a token, add it to the vector.
      tokens.push_back(str.substr(lastPos, pos - lastPos));
      // Skip delims.  Note the "not_of". this is beginning of token
      lastPos = str.find_first_not_of(delims, pos);
      // Find next delimiter at end of token.
      pos     = str.find_first_of(delims, lastPos);
    }

  return tokens;
}


// replace all instances of victim with replacement
std::string mass_replace(const std::string &source, const std::string &victim, const std::string &replacement)
{
	std::string answer = source;
	std::string::size_type j = 0;
	while ((j = answer.find(victim, j)) != std::string::npos )
	answer.replace(j, victim.length(), replacement);
	return answer;
}

std::string stringToUpper(const std::string& s);

std::string stringToUpper(const std::string& s) {
	std::string ret = s;
	for(size_t i = 0; i<s.size(); i++)
		ret[i] = toupper(ret[i]);
	return ret;
}

#define STRIP_SP	0x01 // space
#define STRIP_TAB	0x02 // tab
#define STRIP_CR	0x04 // carriage return
#define STRIP_LF	0x08 // line feed
#define STRIP_WHITESPACE (STRIP_SP|STRIP_TAB)

///White space-trimming routine
///Removes whitespace from left side of string, depending on the flags set (See STRIP_x definitions in xstring.h)
///Returns number of characters removed
std::string str_ltrim(const std::string &stdstr, int flags = STRIP_WHITESPACE) {
  char *str = strdup(stdstr.c_str());
	int i=0;

	while (str[0]) {
		if ((str[0] != ' ') && (str[0] != '\t') && (str[0] != '\r') && (str[0] != '\n')) break;

		if ((flags & STRIP_SP) && (str[0] == ' ')) {
			i++;
			strcpy(str,str+1);
		}
		if ((flags & STRIP_TAB) && (str[0] == '\t')) {
			i++;
			strcpy(str,str+1);
		}
		if ((flags & STRIP_CR) && (str[0] == '\r')) {
			i++;
			strcpy(str,str+1);
		}
		if ((flags & STRIP_LF) && (str[0] == '\n')) {
			i++;
			strcpy(str,str+1);
		}
	}
	
  std::string ret = str;
  free(str);
  return ret;
}





bool CueData::parse_cue(const char* buf)
{
  std::string data = mass_replace(buf,"\r\n","\n");
  std::vector<std::string> lines = tokenize_str(data,"\n");

  int thistrack=-1;
  CueTrack currTrack;
  int state = 0;
  for(int i=0;i<(int)lines.size();i++)
  {
    std::string line = lines[i];
    line = str_ltrim(line);
reparse:
    switch(state)
    {
    case 0:
      if(stringToUpper(line).substr(0,4)!="FILE") break;
      //We actually need the file names - especially the first one, so we can load the CD from that.
      state = 1;
      break;
    case 1:
      {
        if(stringToUpper(line).substr(0,5)!="TRACK") break;
        std::vector<std::string> parts = tokenize_str(line.substr(5)," ");
        thistrack = atoi(parts[0].c_str());
        currTrack.tracktype = parts[1];
        state = 2;
        break;
      }
    case 2:
      {
        if(stringToUpper(line).substr(0,5)=="INDEX")
        {
          std::vector<std::string> parts = tokenize_str(line.substr(5)," ");
          int thisindex = atoi(parts[0].c_str());
          CueTimestamp temp;
          sscanf(parts[1].c_str(),"%d:%d:%d",&temp.parts[0],&temp.parts[1],&temp.parts[2]);
          currTrack.indexes[thisindex] = temp;
        }
        if(stringToUpper(line).substr(0,4)=="FILE")
        {
          tracks[thistrack] = currTrack;
          currTrack = CueTrack();
          thistrack = -1;
          state = 0;
          goto reparse;
        }
        break;
      }
    }
  }

  if(thistrack != -1)
     tracks[thistrack] = currTrack;

  return true;
}

int CueData::NumTracks() 
{ 
	return tracks.size(); 
}
int CueData::MinTrack()
  {
    struct COMPARE { bool operator()(std::pair<int,CueTrack> lhs, std::pair<int,CueTrack> rhs) { return lhs.first < rhs.first; } } compare;
    return std::min_element(tracks.begin(),tracks.end(),compare)->first;
  }
int CueData::MaxTrack()
  {
    struct COMPARE { bool operator()(std::pair<int,CueTrack> lhs, std::pair<int,CueTrack> rhs) { return lhs.first < rhs.first; } } compare;
    return std::max_element(tracks.begin(),tracks.end(),compare)->first;
  }

int CueData::cueparser(char* Filename)
{  
  //Need to pull file location off this file, so it can be added to each file in the cue.
  FILE* inf = fopen(Filename,"rb"); 
  fseek(inf,0,SEEK_END);
  long len = ftell(inf);
  fseek(inf,0,SEEK_SET);
  char* buf = new char[len+1];
  fread(buf,1,len,inf);
  buf[len] = 0;  
  parse_cue(buf);
  printf("numtracks: %d; mintrack: %d; maxtrack: %d\n",NumTracks(),MinTrack(),MaxTrack());

}

void CueData::CopyToConfig()
{
	Config.CueTracks = NumTracks();
	for (int i = 0; i < Config.CueTracks; i++)
	{
		//Add file extension here.
		strcpy(Config.CueList[i].FileName, "\0");
		Config.CueList[i].StartPosMM = 0;
		Config.CueList[i].StartPosSS = 0;
		Config.CueList[i].StartPosFF = 0;
		Config.CueList[i].EndPosMM = 0;		
		Config.CueList[i].EndPosSS = 0;		
		Config.CueList[i].EndPosFF = 0;
	}
}
 CueData::CueData()
{

}

 CueTrack::CueTrack()
{
}

CueTimestamp::CueTimestamp()
{
}