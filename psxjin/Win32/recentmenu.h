//Recent Menu Class
//Does the work of creating recent menus for any list of files

#include <string>
#include <vector>
#include <sstream>
#include "windows.h"

using namespace std;

class RecentMenu
{
public:
	static const unsigned int MAX_RECENT_ITEMS = 8;		//Max number of items that can be in the recent menu (this class will use MAX + 2 resource numbers, keep that in mind when reserving!)
	
	RecentMenu(int baseID, HWND GUI_hWnd, int menuItem); //Constructor - Needs a base ID to start building the menu item list
	RecentMenu();

	void UpdateRecentItems(std::string filename);	
	void UpdateRecentItems(const char* filename);	//Overload

	void RemoveRecentItem(std::string filename);
	void RemoveRecentItem(const char* filename);

	void GetRecentItemsFromIni(std::string iniFilename, string section, string type);	//Retrieves items from an Ini file and populates the recent Menu
	void SaveRecentItemsToIni(std::string iniFilename, string section, string type);	//Saves items to the Ini file
	//iniFilename - Name of the .ini file
	//section - Section of the ini file to save [General] for instance, must include the brackets!
	//type - type of file, for instance ROM, the result will be Recent ROM 1

	void ClearRecentItems();
	
	string GetRecentItem(unsigned int listNum);	//Retrieves the filename by list number (use for opening when user selects menu item)
	//TODO: GetRecentItems - returns a vector of strings, for emulators that don't save to Ini

private:
	int BaseID;					//The user will decide an ID for which to build the menu items from (one that doesn't class with other resoruce file items!)
	int ClearID;				//Menu item for the clear list (will be one baseID + MAX
	int menuItem;				//Which menu item to build the recent submenu off	
	HWND GhWnd;					//Handle to the Windows GUI

	vector<string> RecentItems;	//The list of recent filenames
	HMENU recentmenu;			//Handle to the recent ROMs submenu

	void UpdateRecent(string newItem);	//Does the actual work for UpdateRecentItemsMenu
	void RemoveRecent(string filename); //Does the actual work for RemoveRecentItem
	void UpdateRecentItemsMenu();		//Populate the menu with the recent Items
};