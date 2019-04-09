#pragma once
#include "VirtualEvent.h"

class Convertor {
private:
	RGSSLoader rgss;

	static char scriptGetAllMapList[];
	static char scriptGetMapName[];
	static char scriptGetMapData[];
	static char scriptGetMapOri[];
	static char scriptGetMapEvents[];
	static char scriptGetAllEnemyData[];
	static char scriptGetEnemyData[];
	static char scriptGetStartPos[];
	static const char *layer2name[3];
	static const char *dir2name[4];

	static unsigned int versionCode;

	wchar_t *rmDir;
	char *h5Dir;

	char globalDataDir[MAX_PATH];
	char *globalDataStr;
	char *jsonDataStr;
	char globalMapDir[MAX_PATH];
	char *globalMapStr;
	char *jsonMapStr;
	char globalIconDir[MAX_PATH];
	char *globalIconStr;
	char *jsonIconStr;
	char globalEnemyDir[MAX_PATH];
	char *globalEnemyStr;
	char *jsonEnemyStr;

	unsigned int TilesetIndexCount;
	int currentBlockIndex;

	char scripts[1024];
	char outputName[MAX_PATH];
	char *workDir;

	Json::Value jsonData;
	Json::Value jsonMap;
	Json::Value jsonIcon;
	Json::Value jsonEnemy;
	Json::Value jsonConfig;
	Json::CharReaderBuilder charbuilder;
	Json::StreamWriterBuilder builder;
	
	Json::Value tilesetRem;
	Json::Value tilesetOrder;
	Json::Value enemiesRem;

	Json::Value log;

	string errorLog;

	unsigned int getTilesetIndex(char*);
	bool getAppOptions(char**,int);
	bool getAppConfig(char*);
	void moveImage(char*, char*);
	void moveSound(char*, char*);
	void bltImage(char*, char*, int, int, int, int, int, int, int, int);
	void clearImage(char*, int, int, int, int);
	void registerBlock(const char*, char*, int);
	void createConfig(char*);
	bool runVirtualEvent(RV*, int, Json::Value&,int,RV,bool,RV);
	int pullEnemyImage(int, RV);
	int pullPowerUpImage(const char*, RV);
	int pullPotionImage(const char*, RV);
	int pullItemImage(const char*, RV);
	int pullDoorImage(const char*, RV);
	void pullBGM(const char*);
	void pullSE(const char*);

	struct RGSSMapInfo
	{
		RR::RGSSTable *mapTable;
		RR::RGSSTable *mapPass;
		RV *autotileList;
		char mapName[50];
		string mapTitle;
		char *tilesetName;
		Json::Value mapjson;
		int mapId;
		bool canCutMap;
	};

	struct AppOptions
	{
		bool cutMap;
		int cutWidth;
		int cutX;
		int cutY;
		int forceStart;
		int forceEnd;
		bool convertAudio;
		bool onlyConfig;
	};

	RGSSMapInfo mapInfo;

public:

	int convertMapData();
	bool initAll(char**,int);
	void resetAll();
	bool loadH5Info();
	void release();
	void preprocessData();
	void processAllEnemies();
	void processAllMap();
	void getRGSSMapInfo(int);
	void setupJsonMapInfo();
	void setupJsonMapTileData();
	void writeJsonMapFile();
	void addMapToGlobalData();
	void processGlobalData();
	void writeJsonGlobalDataFile();
	void getRGSSEvents();
	void pullEnemyData(char*,int);
	void writeLogs();
	void writeFile(string*,char*);

	AppOptions options;
};