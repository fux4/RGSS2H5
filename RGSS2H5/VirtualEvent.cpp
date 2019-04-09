#include "stdafx.h"
#include "VirtualEvent.h"
#include "Tool.h"
#include <regex>

VirtualEvent::VirtualEvent(RV *commands,int listSize,Json::Value &thisPageEvents, RGSSLoader &rgss,bool cheapEvent,int startIndex, Json::Value& jsonConfig, Json::Value& log,RV thisEvent):thisPageEvents(thisPageEvents),rgss(rgss), jsonConfig(jsonConfig), log(log){
	this->commands = commands;
	this->listSize = listSize;
	this->thisPageEvents = thisPageEvents;
	this->jsonConfig = jsonConfig;
	this->log = log;
	this->rgss = rgss;
	this->cheapEvent = cheapEvent;
	this->thisEvent = thisEvent;
	index = startIndex;
	state = 0;
}

Json::Value VirtualEvent::executeCommand() {
	Json::Value thisCommand;
	int thisCommandCode = FIX2NUM(rgss.rb_iv_get(commands[index], "@code"));
	RV *thisParams = RR::RARRAY_PTR(rgss.rb_iv_get(commands[index], "@parameters"));
	int thisIndent = FIX2NUM(rgss.rb_iv_get(commands[index], "@indent"));
	switch (thisCommandCode)
	{
	case 0:
	case 401:
	case 403:
	case 404:
	case 412:
		thisCommand = Json::nullValue;
		index++;
		break;
	case 101:
	{
		string txt = "";
		Tool::convertString(tempStr, rgss.rb_string_value_ptr(&thisParams[0]));
		txt = txt + tempStr + "\n";
		while (FIX2NUM(rgss.rb_iv_get(commands[index + 1], "@code")) == 401) {
			index++;
			Tool::convertString(tempStr, rgss.rb_string_value_ptr(&RR::RARRAY_PTR(rgss.rb_iv_get(commands[index], "@parameters"))[0]));
			txt = txt + tempStr + "\n";
		}
		static regex r("\\\\");
		txt = regex_replace(txt, r, "/");
		index++;
		thisCommand = txt;
		break;
	}
	case 102:
	{
		thisCommand["type"] = "choices";
		thisCommand["text"] = "";
		thisCommand["choices"].resize(0);
		RV *rchoices = RR::RARRAY_PTR(thisParams[0]);
		int rchoiceSize = RR::RARRAY_LEN(thisParams[0]);
		index++;
		for (int cid = 0; cid < rchoiceSize; cid++) {
			Json::Value choiceContent;
			Tool::convertString(tempStr, rgss.rb_string_value_ptr(&rchoices[cid]));
			choiceContent["text"] = tempStr;
			choiceContent["action"].resize(0);
			getBranchCommand(choiceContent["action"]);
			thisCommand["choices"].append(choiceContent);
		}
		if (FIX2NUM(thisParams[1]) == 4 && FIX2NUM(rgss.rb_iv_get(commands[index], "@code")) == 403) {
			Json::Value choiceContent;
			choiceContent["text"] = "取消";
			choiceContent["action"].resize(0);
			getBranchCommand(choiceContent["action"]);
			thisCommand["choices"].append(choiceContent);
		}
		break;
	}
	case 103:
	{
		Json::Value firstCmd;
		firstCmd["type"] = "input";
		firstCmd["text"] = "请输入数值";
		Json::Value nextCmd;
		nextCmd["type"] = "setValue";
		sprintf_s(tempStr, "flag:exVar%d", FIX2NUM(thisParams[0]));
		nextCmd["name"] = tempStr;
		nextCmd["value"] = "flag:input";
		thisCommand.append(firstCmd);
		thisCommand.append(nextCmd);
		index++;
		break;
	}
	case 104:
	{
		static const char *index2pos[] = { "up","center","down" };
		thisCommand["type"] = "setText";
		thisCommand["position"] = index2pos[FIX2NUM(thisParams[0])];
		Json::Value colorValue;
		colorValue.append(0);
		colorValue.append(0);
		colorValue.append(0);
		colorValue.append(FIX2NUM(thisParams[1]) == 0 ? 0.85 : 0);
		thisCommand["background"] = colorValue;
		index++;
		break;
	}
	case 105:
	{
		sprintf_s(tempStr, "等待按键输入，结果传给变量%d", FIX2NUM(thisParams[0]));
		thisCommand["type"] = "comment";
		thisCommand["text"] = tempStr;
		index++;
		break;
	}
	case 106:
	{
		thisCommand["type"] = "sleep";
		thisCommand["time"] = FIX2NUM(thisParams[0]) * 32;
		index++;
		break;
	}
	case 108:
	{
		string txt = "";
		Tool::convertString(tempStr, rgss.rb_string_value_ptr(&thisParams[0]));
		txt = txt + tempStr + "\n";
		while (FIX2NUM(rgss.rb_iv_get(commands[index + 1], "@code")) == 408) {
			index++;
			Tool::convertString(tempStr, rgss.rb_string_value_ptr(&RR::RARRAY_PTR(rgss.rb_iv_get(commands[index], "@parameters"))[0]));
			txt = txt + tempStr + "\n";
		}
		thisCommand["type"] = "comment";
		thisCommand["text"] = txt;
		index++;
		break;
	}
	case 111:
	{
		thisCommand["type"] = "if";
		switch (FIX2NUM(thisParams[0]))
		{
		case 0:
		{
			sprintf_s(tempStr, "flag:exSwitch%d == %s", FIX2NUM(thisParams[1]), FIX2NUM(thisParams[2]) == 0 ? "true" : "false");
			thisCommand["condition"] = tempStr;
			break;
		}
		case 1:
		{
			static const char *index2symbol[] = { "==",">=","<=",">","<","!=" };
			if (FIX2NUM(thisParams[2]) == 0) {
				sprintf_s(tempStr, "flag:exVar%d %s %d", FIX2NUM(thisParams[1]), index2symbol[FIX2NUM(thisParams[4])], FIX2NUM(thisParams[3]));
			}
			else {
				sprintf_s(tempStr, "flag:exVar%d %s flag:exVar%d", FIX2NUM(thisParams[1]), index2symbol[FIX2NUM(thisParams[4])], FIX2NUM(thisParams[3]));
			}
			thisCommand["condition"] = tempStr;
			break;
		}
		case 2:
		{
			sprintf_s(tempStr, "switch:%s == %s", rgss.rb_string_value_ptr(&thisParams[1]), FIX2NUM(thisParams[2]) == 0 ? "true" : "false");
			thisCommand["condition"] = tempStr;
			break;
		}
		case 3:
		{
			thisCommand["condition"] = "(!)计时器结果判断";
			state = 1;
			break;
		}
		case 4:
		{
			thisCommand["condition"] = "(!)角色属性判断";
			state = 1;
			break;
		}
		case 5:
		{
			thisCommand["condition"] = "(!)敌人属性判断";
			state = 1;
			break;
		}
		case 6:
		{
			thisCommand["condition"] = "(!)地图NPC属性判断";
			state = 1;
			break;
		}
		case 7:
		{
			sprintf_s(tempStr, "status:money %s %d", FIX2NUM(thisParams[2]) == 0 ? ">=" : "<=", FIX2NUM(thisParams[1]));
			thisCommand["condition"] = tempStr;
			break;
		}
		case 8:
		{
			sprintf_s(tempStr, "(!)%d号物品是否存在", FIX2NUM(thisParams[1]));
			thisCommand["condition"] = tempStr;
			state = 1;
			break;
		}
		case 9:
		{
			sprintf_s(tempStr, "(!)%d号武器是否存在", FIX2NUM(thisParams[1]));
			thisCommand["condition"] = tempStr;
			state = 1;
			break;
		}
		case 10:
		{
			sprintf_s(tempStr, "(!)%d号防具是否存在", FIX2NUM(thisParams[1]));
			thisCommand["condition"] = tempStr;
			state = 1;
			break;
		}
		case 11:
		{
			thisCommand["condition"] = "(!)按键是否被按下";
			state = 1;
			break;
		}
		case 12:
		{
			Tool::convertString(tempStr2, rgss.rb_string_value_ptr(&thisParams[1]));
			sprintf_s(tempStr, "(!)ruby脚本结果为真:%s", tempStr2);
			break;
		}
		default:
			thisCommand["condition"] = "(!)未知条件";
			state = 1;
			break;
		}
		thisCommand["true"].resize(0);
		thisCommand["false"].resize(0);
		getBranchCommand(thisCommand["true"]);
		if (FIX2NUM(rgss.rb_iv_get(commands[index], "@code")) == 411) {
			getBranchCommand(thisCommand["false"]);
		}
		break;
	}
	case 112:
	{
		thisCommand["type"] = "while";
		thisCommand["condition"] = "true";
		thisCommand["data"].resize(0);
		getBranchCommand(thisCommand["data"]);
		index++;
		break;
	}
	case 113:
	{
		thisCommand["type"] = "break";
		index++;
		break;
	}
	case 115:
	{
		thisCommand["type"] = "exit";
		index++;
		break;
	}
	case 116:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)暂时消除事件";
		index++;
		state = 1;
		break;
	}
	case 117:
	{
		static char name[50];
		bool isSpecialCommonEvent = false;
		int cmEventId = FIX2NUM(thisParams[0]);
		if (cmEventId == jsonConfig["commonEventList"]["battle"].asInt()) {
			RV rname = rgss.rb_iv_get(thisEvent, "@name");
			static regex enemyName("怪物(\\d+)");
			static smatch regexResult;
			Tool::convertString(name, rgss.rb_string_value_ptr(&rname));
			string eventName = name;
			if (regex_match(eventName, regexResult, enemyName)) {
				int enemyId = atoi(regexResult[1].str().c_str());
				sprintf_s(name, "exEnemy%d", enemyId);
				myLog["enemies"].append(enemyId);
				Json::Value firstCmd;
				firstCmd["type"] = "battle";
				firstCmd["id"] = name;
				Json::Value nextCmd;
				nextCmd["type"] = "hide";
				nextCmd["time"] = 0;
				thisCommand.append(firstCmd);
				thisCommand.append(nextCmd);
				isSpecialCommonEvent = true;
			}
		}
		else if (cmEventId == jsonConfig["commonEventList"]["openDoor"].asInt()) {
			RV rname = rgss.rb_iv_get(thisEvent, "@name");
			Tool::convertString(name, rgss.rb_string_value_ptr(&rname));
			if (!jsonConfig["doorName2Id"][name].isNull()) {
				const char *doorName = jsonConfig["doorName2Id"][name].asCString();
				sprintf_s(tempStr, "(!)将本事件视为%s门", doorName);
				thisCommand["type"] = "comment";
				thisCommand["text"] = tempStr;
				state = 1;
				isSpecialCommonEvent = true;
			}
		}
		else if (cmEventId == jsonConfig["commonEventList"]["eventEnd"].asInt()) {
			thisCommand["type"] = "hide";
			thisCommand["time"] = 0;
			isSpecialCommonEvent = true;
		}
		if (!isSpecialCommonEvent) {
			sprintf_s(tempStr, "(!)执行%d号公共事件", cmEventId);
			thisCommand["type"] = "comment";
			thisCommand["text"] = tempStr;
			state = 1;
		}
		index++;
		break;
	}
	case 118:
	{
		Tool::convertString(tempStr2, rgss.rb_string_value_ptr(&thisParams[0]));
		sprintf_s(tempStr, "(!)标签:%s", tempStr2);
		thisCommand["type"] = "comment";
		thisCommand["text"] = tempStr;
		index++;
		state = 1;
		break;
	}
	case 119:
	{
		Tool::convertString(tempStr2, rgss.rb_string_value_ptr(&thisParams[0]));
		sprintf_s(tempStr, "(!)跳转至标签:%s", tempStr2);
		thisCommand["type"] = "comment";
		thisCommand["text"] = tempStr;
		index++;
		state = 1;
		break;
	}
	case 121:
	{
		int startSwId = FIX2NUM(thisParams[0]);
		int endSwId = FIX2NUM(thisParams[1]);
		bool judgeValue = FIX2NUM(thisParams[2]) == 0;
		if (startSwId == endSwId) {
			thisCommand["type"] = "setValue";
			sprintf_s(tempStr, "flag:exSwitch%d", startSwId);
			thisCommand["name"] = tempStr;
			thisCommand["value"] = judgeValue ? "true" : "false";
		}
		else {
			sprintf_s(tempStr, "for(var i=%d;i<=%d;i++){\n  core.setFlag('exSwitch'+i,%s);\n}", startSwId, endSwId, judgeValue ? "true" : "false");
			thisCommand["type"] = "function";
			thisCommand["function"] = tempStr;
		}
		index++;
		break;
	}
	case 122:
	{
		bool isSpecialEvent = false;
		int secondCommandCode = FIX2NUM(rgss.rb_iv_get(commands[index + 1], "@code"));
		if (secondCommandCode == 117) {
			RV *params = RR::RARRAY_PTR(rgss.rb_iv_get(commands[index], "@parameters"));
			int voType = FIX2NUM(params[3]);
			int constValue = FIX2NUM(params[4]);
			int opType = FIX2NUM(params[2]);
			int vstart = FIX2NUM(params[0]);
			int vend = FIX2NUM(params[1]);
			if (vstart == vend && voType == 0 && opType == 0) {
				if (vstart == jsonConfig["variableList"]["doorId"].asInt()) {
					if (FIX2NUM(RR::RARRAY_PTR(rgss.rb_iv_get(commands[index + 1], "@parameters"))[0]) == jsonConfig["commonEventList"]["openDoor"].asInt()) {
						sprintf_s(tempStr, "%d", constValue);
						if (!jsonConfig["doorIndex2Id"][tempStr].isNull()) {
							const char *doorName = jsonConfig["doorIndex2Id"][tempStr].asCString();
							sprintf_s(tempStr, "(!)将本事件视为%s门", doorName);
							thisCommand["type"] = "comment";
							thisCommand["text"] = tempStr;
							state = 1;
							index++;
							isSpecialEvent = true;
						}
						else {
							sprintf_s(tempStr, "(!)将本事件视为RM中的%d号门", constValue);
							thisCommand["type"] = "comment";
							thisCommand["text"] = tempStr;
							state = 1;
							index++;
							isSpecialEvent = true;
						}
					}
				}
				else if (vstart == jsonConfig["variableList"]["powerUpType"].asInt()) {
					if (FIX2NUM(RR::RARRAY_PTR(rgss.rb_iv_get(commands[index + 1], "@parameters"))[0]) == jsonConfig["commonEventList"]["powerUp"].asInt()) {
						sprintf_s(tempStr, "%d", constValue);
						if (!jsonConfig["powerIndex2Id"][tempStr].isNull()) {
							const char *powerName = jsonConfig["powerIndex2Id"][tempStr].asCString();
							myLog["powerup"].append(powerName);
							thisCommand["type"] = "setValue";
							sprintf_s(tempStr, "item:%s", powerName);
							thisCommand["name"] = tempStr;
							sprintf_s(tempStr, "item:%s + 1", powerName);
							thisCommand["value"] = tempStr;
							index++;
							isSpecialEvent = true;
						}
						else {
							sprintf_s(tempStr, "(!)将本事件视为RM中的%d号宝石", constValue);
							thisCommand["type"] = "comment";
							thisCommand["text"] = tempStr;
							state = 1;
							index++;
							isSpecialEvent = true;
						}
					}
				}
				else if (vstart == jsonConfig["variableList"]["potionId"].asInt()) {
					if (FIX2NUM(RR::RARRAY_PTR(rgss.rb_iv_get(commands[index + 1], "@parameters"))[0]) == jsonConfig["commonEventList"]["gainHp"].asInt()) {
						sprintf_s(tempStr, "%d", constValue);
						if (!jsonConfig["potionIndex2Id"][tempStr].isNull()) {
							const char *powerName = jsonConfig["potionIndex2Id"][tempStr].asCString();
							myLog["potions"].append(powerName);
							thisCommand["type"] = "setValue";
							sprintf_s(tempStr, "item:%s", powerName);
							thisCommand["name"] = tempStr;
							sprintf_s(tempStr, "item:%s + 1", powerName);
							thisCommand["value"] = tempStr;
							index++;
							isSpecialEvent = true;
						}
						else {
							sprintf_s(tempStr, "(!)将本事件视为RM中的%d号血瓶", constValue);
							thisCommand["type"] = "comment";
							thisCommand["text"] = tempStr;
							state = 1;
							index++;
							isSpecialEvent = true;
						}
					}
				}
			}
		}
		else if (secondCommandCode == 122) {
			int thirdCommandCode = FIX2NUM(rgss.rb_iv_get(commands[index + 2], "@code"));
			if (thirdCommandCode == 117) {
				RV *params1 = RR::RARRAY_PTR(rgss.rb_iv_get(commands[index], "@parameters"));
				int voType1 = FIX2NUM(params1[3]);
				int constValue1 = FIX2NUM(params1[4]);
				int opType1 = FIX2NUM(params1[2]);
				int vstart1 = FIX2NUM(params1[0]);
				int vend1 = FIX2NUM(params1[1]);
				RV *params2 = RR::RARRAY_PTR(rgss.rb_iv_get(commands[index + 1], "@parameters"));
				int voType2 = FIX2NUM(params2[3]);
				int constValue2 = FIX2NUM(params2[4]);
				int opType2 = FIX2NUM(params2[2]);
				int vstart2 = FIX2NUM(params2[0]);
				int vend2 = FIX2NUM(params2[1]);
				if (vstart1 == vend1 && voType1 == 0 && opType1 == 0 && vstart2 == vend2 && voType2 == 0 && opType2 == 0) {
					if (vstart1 == jsonConfig["variableList"]["itemId"].asInt() && vstart2 == jsonConfig["variableList"]["itemType"].asInt()) {
						sprintf_s(tempStr, "%d", constValue1);
						sprintf_s(tempStr2, "%d", constValue2);
						if (!jsonConfig["itemIndex2Id"][tempStr2][tempStr].isNull()) {
							const char *itemName = jsonConfig["itemIndex2Id"][tempStr2][tempStr].asCString();
							myLog["items"].append(itemName);
							thisCommand["type"] = "setValue";
							sprintf_s(tempStr, "item:%s", itemName);
							thisCommand["name"] = tempStr;
							sprintf_s(tempStr, "item:%s + 1", itemName);
							thisCommand["value"] = tempStr;
							index += 2;
							isSpecialEvent = true;
						}
						else {
							sprintf_s(tempStr, "(!)将本事件视为RM中的%d类%d号物品", constValue2, constValue1);
							thisCommand["type"] = "comment";
							thisCommand["text"] = tempStr;
							state = 1;
							index += 2;
							isSpecialEvent = true;
						}
					}
				}
			}
		}
		if (!isSpecialEvent) {
			switch (FIX2NUM(thisParams[3])) {
			case 0:
				sprintf_s(tempStr2, "%d", FIX2NUM(thisParams[4]));
				break;
			case 1:
				sprintf_s(tempStr2, "core.getFlag('exVar%d',0)", FIX2NUM(thisParams[4]));
				break;
			case 2:
				sprintf_s(tempStr2, "%d + Math.floor(core.rand(%d))", FIX2NUM(thisParams[4]), FIX2NUM(thisParams[5]) - FIX2NUM(thisParams[4]) + 1);
				break;
			case 3:
				sprintf_s(tempStr2, "(!)%d号物品数量", FIX2NUM(thisParams[4]));
				state = 1;
				break;
			case 4:
				sprintf_s(tempStr2, "(!)%d号角色的某属性", FIX2NUM(thisParams[4]));
				state = 1;
				break;
			case 5:
				sprintf_s(tempStr2, "(!)%d号敌人的某属性", FIX2NUM(thisParams[4]));
				state = 1;
				break;
			case 6:
				sprintf_s(tempStr2, "(!)%d号事件的某属性", FIX2NUM(thisParams[4]));
				state = 1;
				break;
			case 7:
			{
				switch (FIX2NUM(thisParams[4])) {
				case 0:
					sprintf_s(tempStr2, "(!)当前地图ID");
					state = 1;
					break;
				case 1:
					sprintf_s(tempStr2, "(!)队伍人数");
					state = 1;
					break;
				case 2:
					sprintf_s(tempStr2, "core.getStatus('money')");
					break;
				case 3:
					sprintf_s(tempStr2, "core.getStatus('steps')");
					break;
				case 4:
					sprintf_s(tempStr2, "(!)游戏运行到现在的帧数");
					state = 1;
					break;
				case 5:
					sprintf_s(tempStr2, "(!)计时器剩余数");
					state = 1;
					break;
				case 6:
					sprintf_s(tempStr2, "(!)存档次数");
					state = 1;
					break;
				default:
					sprintf_s(tempStr2, "(!)未知值");
					state = 1;
					break;
				}
			}
			}
			static const char *index2symbol[] = { "=","+","-","*","/","%" };
			int startSwId = FIX2NUM(thisParams[0]);
			int endSwId = FIX2NUM(thisParams[1]);
			if (startSwId == endSwId) {
				thisCommand["type"] = "setValue";
				sprintf_s(tempStr, "flag:exVar%d", startSwId);
				thisCommand["name"] = tempStr;
				if (FIX2NUM(thisParams[2]) > 0) {
					sprintf_s(tempStr, "flag:exVar%d %s %s", startSwId, index2symbol[FIX2NUM(thisParams[2])], tempStr2);
					thisCommand["value"] = tempStr;
				}
				else {
					thisCommand["value"] = tempStr2;
				}
			}
			else {
				if (FIX2NUM(thisParams[2]) > 0) {
					sprintf_s(tempStr, "for(var i=%d;i<=%d;i++){\n  core.setFlag('exVar'+i,core.getFlag('exVar'+i,0) %s %s);\n}", startSwId, endSwId, index2symbol[FIX2NUM(thisParams[2])], tempStr2);
				}
				else {
					sprintf_s(tempStr, "for(var i=%d;i<=%d;i++){\n  core.setFlag('exVar'+i,%s);\n}", startSwId, endSwId, tempStr2);
				}
				thisCommand["type"] = "function";
				thisCommand["function"] = tempStr;
			}
		}
		index++;
		break;
	}
	case 123:
	{
		if (cheapEvent) {
			thisCommand["type"] = "hide";
			thisCommand["time"] = 0;
		}
		else {
			thisCommand["type"] = "setValue";
			char *rswName = rgss.rb_string_value_ptr(&thisParams[0]);
			sprintf_s(tempStr, "switch:%s", rswName);
			thisCommand["name"] = tempStr;
			thisCommand["value"] = (FIX2NUM(thisParams[1]) == 0) ? "true" : "false";
		}
		index++;
		break;
	}
	case 124:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)计时器操作";
		index++;
		state = 1;
		break;
	}
	case 125:
	{
		operate_value(FIX2NUM(thisParams[0]), FIX2NUM(thisParams[1]), FIX2NUM(thisParams[2]));
		thisCommand["type"] = "setValue";
		thisCommand["name"] = "status:money";
		sprintf_s(tempStr, "status:money + %s", tempStr2);
		thisCommand["value"] = tempStr;
		index++;
		break;
	}
	case 126:
	{
		operate_value(FIX2NUM(thisParams[1]), FIX2NUM(thisParams[2]), FIX2NUM(thisParams[3]));
		sprintf_s(tempStr, "(!)获得%s件%d号物品", tempStr2, FIX2NUM(thisParams[0]));
		thisCommand["type"] = "comment";
		thisCommand["text"] = tempStr;
		index++;
		state = 1;
		break;
	}
	case 127:
	{
		operate_value(FIX2NUM(thisParams[1]), FIX2NUM(thisParams[2]), FIX2NUM(thisParams[3]));
		sprintf_s(tempStr, "(!)获得%s件%d号武器", tempStr2, FIX2NUM(thisParams[0]));
		thisCommand["type"] = "comment";
		thisCommand["text"] = tempStr;
		index++;
		state = 1;
		break;
	}
	case 128:
	{
		operate_value(FIX2NUM(thisParams[1]), FIX2NUM(thisParams[2]), FIX2NUM(thisParams[3]));
		sprintf_s(tempStr, "(!)获得%s件%d号防具", tempStr2, FIX2NUM(thisParams[0]));
		thisCommand["type"] = "comment";
		thisCommand["text"] = tempStr;
		index++;
		state = 1;
		break;
	}
	case 129:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)角色替换";
		index++;
		state = 1;
		break;
	}
	case 131:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改窗口外观";
		index++;
		state = 1;
		break;
	}
	case 132:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改战斗BGM";
		index++;
		state = 1;
		break;
	}
	case 133:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改战斗结束ME";
		index++;
		state = 1;
		break;
	}
	case 134:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改禁止存档";
		index++;
		state = 1;
		break;
	}
	case 135:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改禁止菜单";
		index++;
		state = 1;
		break;
	}
	case 136:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改禁止遇敌";
		index++;
		state = 1;
		break;
	}
	case 201:
	{
		static const char *index2dir[] = { "down","left","right","up" };
		if (FIX2NUM(thisParams[0]) == 0) {
			sprintf_s(tempStr, "fux%d", FIX2NUM(thisParams[1]));
			thisCommand["type"] = "changeFloor";
			thisCommand["floorId"] = tempStr;
			thisCommand["time"] = 500;
			thisCommand["loc"].resize(0);
			thisCommand["loc"].append(FIX2NUM(thisParams[2]));
			thisCommand["loc"].append(FIX2NUM(thisParams[3]));
			if (FIX2NUM(thisParams[4]) > 0) {
				thisCommand["direction"] = index2dir[FIX2NUM(thisParams[4]) / 2 - 1];
			}
		}
		else {
			thisCommand["type"] = "comment";
			thisCommand["text"] = "(!)无法转换变量地图传送";
			state = 1;
		}
		index++;
		break;
	}
	case 202:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)设置事件位置";
		index++;
		state = 1;
		break;
	}
	case 203:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)地图滚动";
		index++;
		state = 1;
		break;
	}
	case 204:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改地图设置";
		index++;
		state = 1;
		break;
	}
	case 205:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改雾色调";
		index++;
		state = 1;
		break;
	}
	case 206:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改雾不透明度";
		index++;
		state = 1;
		break;
	}
	case 207:
	{
		sprintf_s(tempStr, "(!)显示%d号动画", FIX2NUM(thisParams[1]));
		thisCommand["type"] = "comment";
		thisCommand["text"] = tempStr;
		index++;
		state = 1;
		break;
	}
	case 208:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改主角透明状态";
		index++;
		state = 1;
		break;
	}
	case 209:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)设置移动路线";
		index++;
		state = 1;
		break;
	}
	case 210:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)等待移动结束";
		index++;
		state = 1;
		break;
	}
	case 221:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)开始过渡";
		index++;
		state = 1;
		break;
	}
	case 222:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)执行过渡";
		index++;
		state = 1;
		break;
	}
	case 223:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改画面色调";
		index++;
		state = 1;
		break;
	}
	case 224:
	{
		RR::RGSSColor *thisColor;
		Data_Get_Struct(thisParams[0], RR::RGSSColor, thisColor);
		thisCommand["type"] = "setFg";
		thisCommand["color"].append((int)thisColor->red);
		thisCommand["color"].append((int)thisColor->green);
		thisCommand["color"].append((int)thisColor->blue);
		thisCommand["color"].append(thisColor->alpha / 255.0);
		thisCommand["time"] = FIX2NUM(thisParams[1]) * 32;
		index++;
		break;
	}
	case 225:
	{
		thisCommand["type"] = "vibrate";
		thisCommand["time"] = FIX2NUM(thisParams[2]) * 32;
		index++;
		break;
	}
	case 231:
	{
		Json::Value firstCmd;
		firstCmd["type"] = "comment";
		firstCmd["text"] = "(!)显示图片或许不准确";
		Json::Value nextCmd;
		nextCmd["type"] = "showImage";
		nextCmd["code"] = FIX2NUM(thisParams[0]);
		Tool::convertString(tempStr, rgss.rb_string_value_ptr(&thisParams[1]));
		nextCmd["image"] = tempStr;
		if (FIX2NUM(thisParams[3]) == 0) {
			nextCmd["loc"].append(FIX2NUM(thisParams[4]));
			nextCmd["loc"].append(FIX2NUM(thisParams[5]));
		}
		else {
			sprintf_s(tempStr, "flag:exVar%d", FIX2NUM(thisParams[4]));
			nextCmd["loc"].append(tempStr);
			sprintf_s(tempStr, "flag:exVar%d", FIX2NUM(thisParams[5]));
			nextCmd["loc"].append(tempStr);
		}
		nextCmd["dw"] = FIX2NUM(thisParams[6]);
		nextCmd["dh"] = FIX2NUM(thisParams[7]);
		nextCmd["opacity"] = (double)FIX2NUM(thisParams[8]) / 255.0;
		nextCmd["time"] = 0;
		thisCommand.append(firstCmd);
		thisCommand.append(nextCmd);
		state = 1;
		index++;
		break;
	}
	case 232:
	{
		Json::Value firstCmd;
		firstCmd["type"] = "comment";
		firstCmd["text"] = "(!)移动图片或许不准确";
		Json::Value nextCmd;
		nextCmd["type"] = "moveImage";
		nextCmd["code"] = FIX2NUM(thisParams[0]);
		if (FIX2NUM(thisParams[3]) == 0) {
			nextCmd["to"].append(FIX2NUM(thisParams[4]));
			nextCmd["to"].append(FIX2NUM(thisParams[5]));
		}
		else {
			sprintf_s(tempStr, "flag:exVar%d", FIX2NUM(thisParams[4]));
			nextCmd["to"].append(tempStr);
			sprintf_s(tempStr, "flag:exVar%d", FIX2NUM(thisParams[5]));
			nextCmd["to"].append(tempStr);
		}
		nextCmd["time"] = FIX2NUM(thisParams[1]) * 32;
		thisCommand.append(firstCmd);
		thisCommand.append(nextCmd);
		state = 1;
		index++;
		break;
	}
	case 233:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)旋转图片";
		index++;
		state = 1;
		break;
	}
	case 234:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改图片色调";
		index++;
		state = 1;
		break;
	}
	case 235:
	{
		thisCommand["type"] = "hideImage";
		thisCommand["code"] = FIX2NUM(thisParams[0]);
		thisCommand["time"] = 0;
		index++;
		break;
	}
	case 236:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)设置天气";
		index++;
		state = 1;
		break;
	}
	case 241:
	{
		static char srcDir[MAX_PATH];
		static char dstDir[MAX_PATH];
		static char h5Name[MAX_PATH];
		thisCommand["type"] = "playBgm";
		RV sname = rgss.rb_iv_get(thisParams[0], "@name");
		Tool::convertString(tempStr, rgss.rb_string_value_ptr(&sname));
		if (log["bgms"][tempStr].isNull()) {
			sprintf_s(dstDir, "Audio\\BGM\\%s", tempStr);
			rgss.rgss_get_filefullname(dstDir, srcDir);
			char *extendName;
			Tool::GetFileExtendName(&extendName, srcDir);
			sprintf_s(h5Name, "exBGM%d.%s", log["bgmCount"].asInt(), extendName);
			log["bgms"][tempStr].append(h5Name);
			log["bgms"][tempStr].append(srcDir);
			log["bgmCount"] = log["bgmCount"].asInt() + 1;
		}
		else {
			sprintf_s(h5Name, "%s", log["bgms"][tempStr][0].asCString());
		}
		thisCommand["name"] = h5Name;
		myLog["bgms"].append(tempStr);
		index++;
		break;
	}
	case 242:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)BGM淡入淡出";
		index++;
		state = 1;
		break;
	}
	case 245:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)演奏BGS";
		index++;
		state = 1;
		break;
	}
	case 246:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)BGS淡入淡出";
		index++;
		state = 1;
		break;
	}
	case 247:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)记忆 BGM / BGS";
		index++;
		state = 1;
		break;
	}
	case 248:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)还原 BGM / BGS";
		index++;
		state = 1;
		break;
	}
	case 249:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)演奏ME";
		index++;
		state = 1;
		break;
	}
	case 250:
	{
		static char srcDir[MAX_PATH];
		static char dstDir[MAX_PATH];
		static char h5Name[MAX_PATH];
		thisCommand["type"] = "playSound";
		RV sname = rgss.rb_iv_get(thisParams[0], "@name");
		Tool::convertString(tempStr, rgss.rb_string_value_ptr(&sname));
		if (log["se"][tempStr].isNull()) {
			sprintf_s(dstDir, "Audio\\SE\\%s", tempStr);
			rgss.rgss_get_filefullname(dstDir, srcDir);
			char *extendName;
			Tool::GetFileExtendName(&extendName, srcDir);
			sprintf_s(h5Name, "exSE%d.%s", log["bgmCount"].asInt(), extendName);
			log["se"][tempStr].append(h5Name);
			log["se"][tempStr].append(srcDir);
			log["bgmCount"] = log["bgmCount"].asInt() + 1;
		}
		else {
			sprintf_s(h5Name, "%s", log["se"][tempStr][0].asCString());
		}
		thisCommand["name"] = h5Name;
		myLog["se"].append(tempStr);
		index++;
		break;
	}
	case 251:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)停止SE";
		index++;
		state = 1;
		break;
	}
	case 301:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)RPG战斗处理";
		index++;
		state = 1;
		break;
	}
	case 601:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)战斗胜利的情况";
		index++;
		state = 1;
		break;
	}
	case 602:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)战斗逃跑的情况";
		index++;
		state = 1;
		break;
	}
	case 603:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)战斗失败的情况";
		index++;
		state = 1;
		break;
	}
	case 302:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)商店处理";
		index++;
		state = 1;
		break;
	}
	case 303:
	{
		Json::Value firstCmd;
		firstCmd["type"] = "input2";
		firstCmd["text"] = "请输入你的名字";
		Json::Value nextCmd;
		nextCmd["type"] = "setValue";
		nextCmd["name"] = "status:name";
		nextCmd["value"] = "flag:input";
		thisCommand.append(firstCmd);
		thisCommand.append(nextCmd);
		index++;
		break;
	}
	case 311:
	{
		operate_value(FIX2NUM(thisParams[1]), FIX2NUM(thisParams[2]), FIX2NUM(thisParams[3]));
		sprintf_s(tempStr, "(!)增减某人%s点HP", tempStr2);
		thisCommand["type"] = "comment";
		thisCommand["text"] = tempStr;
		index++;
		state = 1;
		break;
	}
	case 312:
	{
		operate_value(FIX2NUM(thisParams[1]), FIX2NUM(thisParams[2]), FIX2NUM(thisParams[3]));
		sprintf_s(tempStr, "(!)增减某人%s点SP", tempStr2);
		thisCommand["type"] = "comment";
		thisCommand["text"] = tempStr;
		index++;
		state = 1;
		break;
	}
	case 313:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改某人状态";
		index++;
		state = 1;
		break;
	}
	case 314:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)全恢复";
		index++;
		state = 1;
		break;
	}
	case 315:
	{
		operate_value(FIX2NUM(thisParams[1]), FIX2NUM(thisParams[2]), FIX2NUM(thisParams[3]));
		sprintf_s(tempStr, "(!)增减某人%s点经验", tempStr2);
		thisCommand["type"] = "comment";
		thisCommand["text"] = tempStr;
		index++;
		state = 1;
		break;
	}
	case 316:
	{
		operate_value(FIX2NUM(thisParams[1]), FIX2NUM(thisParams[2]), FIX2NUM(thisParams[3]));
		sprintf_s(tempStr, "(!)增减某人%s个级别", tempStr2);
		thisCommand["type"] = "comment";
		thisCommand["text"] = tempStr;
		index++;
		state = 1;
		break;
	}
	case 317:
	{
		static const char *index2errinfo[] = { "最大生命值","最大法力值","力量","灵巧","速度","魔力" };
		operate_value(FIX2NUM(thisParams[2]), FIX2NUM(thisParams[3]), FIX2NUM(thisParams[4]));
		sprintf_s(tempStr, "(!)增减某人%s点%s", tempStr2, index2errinfo[FIX2NUM(thisParams[1])]);
		thisCommand["type"] = "comment";
		thisCommand["text"] = tempStr;
		index++;
		state = 1;
		break;
	}
	case 318:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)增减特技";
		index++;
		state = 1;
		break;
	}
	case 319:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更变装备";
		index++;
		state = 1;
		break;
	}
	case 320:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改角色名字";
		index++;
		state = 1;
		break;
	}
	case 321:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改角色职业";
		index++;
		state = 1;
		break;
	}
	case 322:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改角色图形";
		index++;
		state = 1;
		break;
	}
	case 331:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)增减敌人HP";
		index++;
		state = 1;
		break;
	}
	case 332:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)增减敌人SP";
		index++;
		state = 1;
		break;
	}
	case 333:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)更改敌人状态";
		index++;
		state = 1;
		break;
	}
	case 334:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)敌人全恢复";
		index++;
		state = 1;
		break;
	}
	case 335:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)敌人出现";
		index++;
		state = 1;
		break;
	}
	case 336:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)敌人变身";
		index++;
		state = 1;
		break;
	}
	case 337:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)给战斗者显示动画";
		index++;
		state = 1;
		break;
	}
	case 338:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)伤害处理";
		index++;
		state = 1;
		break;
	}
	case 339:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)强制行动";
		index++;
		state = 1;
		break;
	}
	case 340:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)战斗中断";
		index++;
		state = 1;
		break;
	}
	case 351:
	{
		thisCommand["type"] = "callBook";
		index++;
		break;
	}
	case 352:
	{
		thisCommand["type"] = "callSave";
		index++;
		break;
	}
	case 353:
	{
		thisCommand["type"] = "lose";
		thisCommand["reason"] = "(!)无法得知原因";
		index++;
		break;
	}
	case 354:
	{
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)返回标题界面";
		index++;
		state = 1;
		break;
	}
	case 355:
	{
		string txt = "";
		Tool::convertString(tempStr, rgss.rb_string_value_ptr(&thisParams[0]));
		txt = txt + tempStr + "\n";
		while (FIX2NUM(rgss.rb_iv_get(commands[index + 1], "@code")) == 655) {
			index++;
			Tool::convertString(tempStr, rgss.rb_string_value_ptr(&RR::RARRAY_PTR(rgss.rb_iv_get(commands[index], "@parameters"))[0]));
			txt = txt + tempStr + "\n";
		}
		thisCommand["type"] = "comment";
		thisCommand["text"] = "(!)" + txt;
		index++;
		state = 1;
		break;
	}
	default:
		sprintf_s(tempStr, "(!)无法识别的指令:%d", thisCommandCode);
		thisCommand["type"] = "comment";
		thisCommand["text"] = tempStr;
		index++;
		state = 1;
		break;
	}
	return thisCommand;
}

void VirtualEvent::operate_value(int operation, int operand_type, int operand) {
	if (operand_type == 0) {
		sprintf_s(tempStr2, "%s%d", operation == 1 ? "-" : "", operand);
	}
	else {
		sprintf_s(tempStr2, "%sflag:exVar%d", operation == 1 ? "-" : "", operand);
	}
}

void VirtualEvent::getBranchCommand(Json::Value& cmdList) {
	int thisIndent = FIX2NUM(rgss.rb_iv_get(commands[index], "@indent"));
	index++;
	while (FIX2NUM(rgss.rb_iv_get(commands[index], "@indent")) != thisIndent) {
		Json::Value thisCommand;
		thisCommand = executeCommand();
		if (!thisCommand.isNull()) {
			if (thisCommand.isArray()) {
				int len = thisCommand.size();
				for (int i = 0; i < len; i++) {
					cmdList.append(thisCommand[i]);
				}
			}
			else {
				cmdList.append(thisCommand);
			}
		}
	}
}

void VirtualEvent::run() {
	while (index < listSize) {
		Json::Value thisCommand;
		thisCommand = executeCommand();
		if (!thisCommand.isNull()) {
			if (thisCommand.isArray()) {
				int len = thisCommand.size();
				for (int i = 0; i < len; i++) {
					thisPageEvents.append(thisCommand[i]);
				}
			}
			else {
				thisPageEvents.append(thisCommand);
			}
		}
	}
}