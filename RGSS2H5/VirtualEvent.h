#pragma once
#include "json\json.h"
#include "RGSSLoader.h"
class VirtualEvent {
private:
	RV* commands;
	int listSize;
	int index = 0;
	RV thisEvent;
	bool cheapEvent = true;
	char tempStr[1024];
	char tempStr2[1024];
	Json::Value& thisPageEvents;
	Json::Value& jsonConfig;
	Json::Value& log;
	RGSSLoader& rgss;

	Json::Value executeCommand();
	void getBranchCommand(Json::Value& cmdList);
	void operate_value(int, int, int);
public:
	VirtualEvent(RV*, int, Json::Value&, RGSSLoader&, bool, int, Json::Value&, Json::Value&, RV);
	void run();
	int state;
	Json::Value myLog;
};