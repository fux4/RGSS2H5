#include "stdafx.h"
#include "Convertor.h"
#include "Tool.h"
#include <regex>

bool Convertor::initAll(char **arguments,int argc) {
	if (!getAppOptions(arguments, argc)) return false;
	if (!getAppConfig(arguments[0])) return false;
	return rgss.initAllItems(rmDir) && rgss.initAllFunctions();
}

void Convertor::resetAll() {
	errorLog.clear();
	currentBlockIndex = 300;
	TilesetIndexCount = 10000;
	tilesetRem.clear();
	tilesetOrder.clear();
	enemiesRem.clear();
	jsonEnemy.clear();
	jsonIcon["enemys"].clear();
	jsonIcon["enemy48"].clear();
	//jsonIcon["animates"].clear();
	log.clear();
	log["move"].resize(0);
	log["draw"].resize(0);
	log["moveSound"].resize(0);
	log["bgmCount"] = 100;
	charbuilder.settings_["allowComments"] = false;
	builder.settings_["commentStyle"] = "None";
}

int Convertor::convertMapData() {
	if (!loadH5Info()) return 3;
	resetAll();
	preprocessData();
	processAllEnemies();
	processAllMap();
	processGlobalData();
	writeJsonGlobalDataFile();
	writeLogs();
	release();
	return 0;
}

void Convertor::processAllEnemies() {
	RV data_enemies = rgss.eval(scriptGetAllEnemyData);
	long enemy_size = RR::RARRAY_LEN(rgss.rdata);
	currentBlockIndex = 300 + enemy_size;
}

void Convertor::processAllMap() {
	static regex usefulMap("\\d{4}");
	static smatch regexResult;
	rgss.eval(scriptGetAllMapList);
	RV *mapdata = RR::RARRAY_PTR(rgss.rdata);
	long len = RR::RARRAY_LEN(rgss.rdata);
	int firstMapId = 9999999;
	if (options.forceStart>0) {
		for (long i = 0; i < len; i++) {
			int mapId = FIX2NUM((mapdata)[i]);
			if (mapId == options.forceStart) firstMapId = i;
			if (mapId == 1 || i >= firstMapId) {
				getRGSSMapInfo(mapId);
				setupJsonMapInfo();
				getRGSSEvents();
				writeJsonMapFile();
				addMapToGlobalData();
			}
			if (mapId == options.forceEnd) break;
		}
	}
	else {
		for (long i = 0; i < len; i++) {
			int mapId = FIX2NUM((mapdata)[i]);
			sprintf_s(scripts, scriptGetMapName, mapId);
			rgss.eval(scripts);
			Tool::convertString(outputName, rgss.rb_string_value_ptr(&rgss.rdata));
			string mapTitle = outputName;
			sprintf_s(outputName, "fux%d", mapId);
			if (mapTitle == "1001") firstMapId = i;
			if (mapId == 1 || (regex_match(mapInfo.mapTitle, regexResult, usefulMap) && i >= firstMapId)) {
				log["map"][mapTitle] = outputName;
			}
		}
		for (long i = 0; i < len; i++) {
			int mapId = FIX2NUM((mapdata)[i]);
			getRGSSMapInfo(mapId);
			if (mapId == 1 || (regex_match(mapInfo.mapTitle, regexResult, usefulMap) && i >= firstMapId)) {
				setupJsonMapInfo();
				getRGSSEvents();
				writeJsonMapFile();
				addMapToGlobalData();
			}
		}
	}
}

void Convertor::getRGSSEvents() {
	static regex enemyName("怪物(\\d+)");
	static smatch regexResult;
	static char name[100];
	static char extname[MAX_PATH];
	static char fullname[MAX_PATH];
	rgss.eval(scriptGetMapEvents);
	long events_size = RR::RARRAY_LEN(rgss.rdata);
	RV *events = RR::RARRAY_PTR(rgss.rdata);
	for (long i = 0; i < events_size; i++) {
		bool proccessedEvent = false;
		RV pages = rgss.rb_iv_get(events[i], "@pages");
		long pageCount = RR::RARRAY_LEN(pages);
		RV firstPage = RR::RARRAY_PTR(pages)[0];
		RV rcmdList = rgss.rb_iv_get(firstPage, "@list");
		RV *cmdList = RR::RARRAY_PTR(rcmdList);
		int cmdLength = RR::RARRAY_LEN(rcmdList);
		int ori_x = FIX2NUM(rgss.rb_iv_get(events[i], "@x"));
		int ori_y = FIX2NUM(rgss.rb_iv_get(events[i], "@y"));
		int eventId = FIX2NUM(rgss.rb_iv_get(events[i], "@id"));
		bool vaildPos = true;
		int event_x = ori_x;
		int event_y = ori_y;
		if (mapInfo.canCutMap) {
			event_x -= options.cutX;
			event_y -= options.cutY;
		}
		if (event_x < 0 || event_y < 0 || event_x >= mapInfo.mapjson["width"].asInt() || event_y >= mapInfo.mapjson["height"].asInt()) {
			vaildPos = false;
		}
		if (vaildPos) {
			if (pageCount == 1) {	// single page
				int firstCommandCode = FIX2NUM(rgss.rb_iv_get(cmdList[0], "@code"));
				if (firstCommandCode == 121) {	// variables operation
					int secondCommandCode = FIX2NUM(rgss.rb_iv_get(cmdList[1], "@code"));
					if (secondCommandCode == 117 && cmdLength == 3) {	// commonevent
						RV *params = RR::RARRAY_PTR(rgss.rb_iv_get(cmdList[0], "@parameters"));
						bool constValue = (FIX2NUM(params[2]) == 0);
						int sstart = FIX2NUM(params[0]);
						int send = FIX2NUM(params[1]);
						if (sstart == send) {
							if (sstart == jsonConfig["switchList"]["upFloor"].asInt()) {
								if (FIX2NUM(RR::RARRAY_PTR(rgss.rb_iv_get(cmdList[1], "@parameters"))[0]) == jsonConfig["commonEventList"]["changeFloor"].asInt()) {
									string stairName = constValue ? "downFloor" : "upFloor";
									int stairId = 0;
									Json::Value floorChange;
									floorChange["floorId"] = constValue ? ":before" : ":next";
									floorChange["stair"] = constValue ? "upFloor" : "downFloor";
									floorChange["time"] = 500;
									sprintf_s(outputName, "%d,%d", event_x, event_y);
									mapInfo.mapjson["changeFloor"][outputName] = floorChange;
									if (log["floor"][stairName].isNull()) {
										int tindex = 0;
										int tsize = jsonIcon["terrains"].size();
										if (jsonIcon["terrains"][stairName].isNull()) {
											jsonIcon["terrains"][stairName] = tsize;
											tindex = tsize;
										}
										else {
											tindex = jsonIcon["terrains"][stairName].asInt();
										}
										bool foundStair = false;
										Json::Value::Members mem = jsonMap.getMemberNames();
										for (auto iter = mem.begin(); iter != mem.end(); iter++) {
											if (strcmp(jsonMap[*iter]["id"].asCString(), stairName.c_str()) == 0) {
												stairId = atoi(iter[0].c_str());
												foundStair = true;
												break;
											}
										}
										if (!foundStair) {
											static char bid[10];
											sprintf_s(bid, "%d", currentBlockIndex);
											Json::Value content;
											content["cls"] = "terrains";
											content["id"] = stairName;
											content["noPass"] = "false";
											jsonMap[bid] = content;
											stairId = currentBlockIndex;
											currentBlockIndex++;

										}
										log["door"][stairName] = stairId;
									}
									else {
										stairId = log["door"][stairName].asInt();
									}
									mapInfo.mapjson[layer2name[1]][event_y][event_x] = stairId;
									proccessedEvent = true;
								}
							}
						}
					}
				}
				else if (firstCommandCode == 117) {
					if (cmdLength == 2 && FIX2NUM(rgss.rb_iv_get(firstPage, "@trigger")) == 4) {
						int commonEventId = FIX2NUM(RR::RARRAY_PTR(rgss.rb_iv_get(cmdList[0], "@parameters"))[0]);
						if (commonEventId == jsonConfig["commonEventList"]["ignoreCommonFloor"].asInt() || commonEventId == jsonConfig["commonEventList"]["ignoreBGMCheck"].asInt()) {
							proccessedEvent = true;
						}
					}
				}
			}
			else if (pageCount == 2) {
				int firstCommandCode = FIX2NUM(rgss.rb_iv_get(cmdList[0], "@code"));
				if (firstCommandCode == 117) {	// maybe enemy
					int commonEventId = FIX2NUM(RR::RARRAY_PTR(rgss.rb_iv_get(cmdList[0], "@parameters"))[0]);
					if (commonEventId == jsonConfig["commonEventList"]["battle"].asInt()) {	// battleEvent
						RV rname = rgss.rb_iv_get(events[i], "@name");
						Tool::convertString(name, rgss.rb_string_value_ptr(&rname));
						string eventName = name;
						if (regex_match(eventName, regexResult, enemyName)) {
							int enemyId = atoi(regexResult[1].str().c_str());
							int enemyCode = pullEnemyImage(enemyId, firstPage);
							mapInfo.mapjson[layer2name[1]][event_y][event_x] = enemyCode;
							proccessedEvent = true;
							if (cmdLength > 2) {
								Json::Value remainingCmd;
								if (!runVirtualEvent(cmdList, cmdLength, remainingCmd, 1, events[i], true, firstPage)) {
									sprintf_s(outputName, "%d号地图[%s],%d号事件%s[%d,%d]：无法完全转换的战后事件。\n", mapInfo.mapId, mapInfo.mapTitle.c_str(), eventId, name, ori_x, ori_y);
									errorLog += outputName;
								}
								sprintf_s(outputName, "%d,%d", event_x, event_y);
								mapInfo.mapjson["afterBattle"][outputName] = remainingCmd;
							}
						}
					}
					else if (commonEventId == jsonConfig["commonEventList"]["openDoor"].asInt()) {	// another door
						RV rname = rgss.rb_iv_get(events[i], "@name");
						Tool::convertString(name, rgss.rb_string_value_ptr(&rname));
						if (!jsonConfig["doorName2Id"][name].isNull()) {
							const char *doorName = jsonConfig["doorName2Id"][name].asCString();
							int doorId = pullDoorImage(doorName, firstPage);
							mapInfo.mapjson[layer2name[1]][event_y][event_x] = doorId;
							proccessedEvent = true;
							if (cmdLength > 2) {
								Json::Value remainingCmd;
								if (!runVirtualEvent(cmdList, cmdLength, remainingCmd, 1, events[i], true, firstPage)) {
									sprintf_s(outputName, "%d号地图[%s],%d号事件%s[%d,%d]：无法完全转换的开门后事件。\n", mapInfo.mapId, mapInfo.mapTitle.c_str(), eventId, name, ori_x, ori_y);
									errorLog += outputName;
								}
								sprintf_s(outputName, "%d,%d", event_x, event_y);
								mapInfo.mapjson["afterOpenDoor"][outputName] = remainingCmd;
							}
						}
					}
				}
				else if (firstCommandCode == 122) {	// Variables Event
					int secondCommandCode = FIX2NUM(rgss.rb_iv_get(cmdList[1], "@code"));
					if (secondCommandCode == 117) {
						RV *params = RR::RARRAY_PTR(rgss.rb_iv_get(cmdList[0], "@parameters"));
						int voType = FIX2NUM(params[3]);
						int constValue = FIX2NUM(params[4]);
						int opType = FIX2NUM(params[2]);
						int vstart = FIX2NUM(params[0]);
						int vend = FIX2NUM(params[1]);

						if (vstart == vend && voType == 0 && opType == 0) {
							if (vstart == jsonConfig["variableList"]["doorId"].asInt()) {	// door event
								if (FIX2NUM(RR::RARRAY_PTR(rgss.rb_iv_get(cmdList[1], "@parameters"))[0]) == jsonConfig["commonEventList"]["openDoor"].asInt()) {
									sprintf_s(outputName, "%d", constValue);
									if (!jsonConfig["doorIndex2Id"][outputName].isNull()) {
										const char *doorName = jsonConfig["doorIndex2Id"][outputName].asCString();
										int doorId = pullDoorImage(doorName, firstPage);
										mapInfo.mapjson[layer2name[1]][event_y][event_x] = doorId;
										proccessedEvent = true;
										if (cmdLength > 3) {
											Json::Value remainingCmd;
											if (!runVirtualEvent(cmdList, cmdLength, remainingCmd, 2, events[i], true, firstPage)) {
												sprintf_s(outputName, "%d号地图[%s],%d号事件%s[%d,%d]：无法完全转换的开门后事件。\n", mapInfo.mapId, mapInfo.mapTitle.c_str(), eventId, name, ori_x, ori_y);
												errorLog += outputName;
											}
											sprintf_s(outputName, "%d,%d", event_x, event_y);
											mapInfo.mapjson["afterOpenDoor"][outputName] = remainingCmd;
										}
									}
								}
							}
							else if (vstart == jsonConfig["variableList"]["powerUpType"].asInt()) {		// powerup event
								if (FIX2NUM(RR::RARRAY_PTR(rgss.rb_iv_get(cmdList[1], "@parameters"))[0]) == jsonConfig["commonEventList"]["powerUp"].asInt()) {
									sprintf_s(outputName, "%d", constValue);
									if (!jsonConfig["powerIndex2Id"][outputName].isNull()) {
										const char *powerName = jsonConfig["powerIndex2Id"][outputName].asCString();
										int powerId = pullPowerUpImage(powerName, firstPage);
										mapInfo.mapjson[layer2name[1]][event_y][event_x] = powerId;
										proccessedEvent = true;
										if (cmdLength > 3) {
											Json::Value remainingCmd;
											if (!runVirtualEvent(cmdList, cmdLength, remainingCmd, 2, events[i], true, firstPage)) {
												sprintf_s(outputName, "%d号地图[%s],%d号事件%s[%d,%d]：无法完全转换的获得物品后事件。\n", mapInfo.mapId, mapInfo.mapTitle.c_str(), eventId, name, ori_x, ori_y);
												errorLog += outputName;
											}
											sprintf_s(outputName, "%d,%d", event_x, event_y);
											mapInfo.mapjson["afterGetItem"][outputName] = remainingCmd;
										}
									}
								}
							}
							else if (vstart == jsonConfig["variableList"]["potionId"].asInt()) {
								if (FIX2NUM(RR::RARRAY_PTR(rgss.rb_iv_get(cmdList[1], "@parameters"))[0]) == jsonConfig["commonEventList"]["gainHp"].asInt()) {
									
									sprintf_s(outputName, "%d", constValue);
									if (!jsonConfig["potionIndex2Id"][outputName].isNull()) {
										const char *potionName = jsonConfig["potionIndex2Id"][outputName].asCString();
										int potionId = pullPotionImage(potionName, firstPage);
										mapInfo.mapjson[layer2name[1]][event_y][event_x] = potionId;
										proccessedEvent = true;
										if (cmdLength > 3) {
											Json::Value remainingCmd;
											if (!runVirtualEvent(cmdList, cmdLength, remainingCmd, 2, events[i], true, firstPage)) {
												sprintf_s(outputName, "%d号地图[%s],%d号事件%s[%d,%d]：无法完全转换的获得物品后事件。\n", mapInfo.mapId, mapInfo.mapTitle.c_str(), eventId, name, ori_x, ori_y);
												errorLog += outputName;
											}
											sprintf_s(outputName, "%d,%d", event_x, event_y);
											mapInfo.mapjson["afterGetItem"][outputName] = remainingCmd;
										}
									}
								}
							}
						}
					}
					else if (secondCommandCode == 122) {
						int thirdCommandCode = FIX2NUM(rgss.rb_iv_get(cmdList[2], "@code"));
						if (thirdCommandCode == 117) {	// item
							RV *params1 = RR::RARRAY_PTR(rgss.rb_iv_get(cmdList[0], "@parameters"));
							int voType1 = FIX2NUM(params1[3]);
							int constValue1 = FIX2NUM(params1[4]);
							int opType1 = FIX2NUM(params1[2]);
							int vstart1 = FIX2NUM(params1[0]);
							int vend1 = FIX2NUM(params1[1]);
							RV *params2 = RR::RARRAY_PTR(rgss.rb_iv_get(cmdList[1], "@parameters"));
							int voType2 = FIX2NUM(params2[3]);
							int constValue2 = FIX2NUM(params2[4]);
							int opType2 = FIX2NUM(params2[2]);
							int vstart2 = FIX2NUM(params2[0]);
							int vend2 = FIX2NUM(params2[1]);
							if (vstart1 == vend1 && voType1 == 0 && opType1 == 0 && vstart2 == vend2 && voType2 == 0 && opType2 == 0) {
								if (vstart1 == jsonConfig["variableList"]["itemId"].asInt() && vstart2 == jsonConfig["variableList"]["itemType"].asInt()) {
									sprintf_s(outputName, "%d", constValue1);
									sprintf_s(extname, "%d", constValue2);
									if (!jsonConfig["itemIndex2Id"][extname][outputName].isNull()) {
										const char *itemName = jsonConfig["itemIndex2Id"][extname][outputName].asCString();
										int itemId = pullItemImage(itemName, firstPage);
										
										mapInfo.mapjson[layer2name[1]][event_y][event_x] = itemId;
										proccessedEvent = true;
										if (cmdLength > 4) {
											Json::Value remainingCmd;
											if (!runVirtualEvent(cmdList, cmdLength, remainingCmd, 3, events[i], true, firstPage)) {
												sprintf_s(outputName, "%d号地图[%s],%d号事件%s[%d,%d]：无法完全转换的获得物品后事件。\n", mapInfo.mapId, mapInfo.mapTitle.c_str(), eventId, name, ori_x, ori_y);
												errorLog += outputName;
											}
											sprintf_s(outputName, "%d,%d", event_x, event_y);
											mapInfo.mapjson["afterGetItem"][outputName] = remainingCmd;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		
		// default event
		if (!proccessedEvent) {
			RV rname = rgss.rb_iv_get(events[i], "@name");
			Tool::convertString(name, rgss.rb_string_value_ptr(&rname));
			if (mapInfo.canCutMap && (event_x < 0 || event_y < 0 || event_x > options.cutWidth-1 || event_y > options.cutWidth - 1)) {
				sprintf_s(outputName, "%d号地图[%s],%d号事件%s[%d,%d]：地图剪裁后，该事件超出地图范围。\n", mapInfo.mapId, mapInfo.mapTitle.c_str(), eventId, name, ori_x, ori_y);
				errorLog += outputName;
			}
			else if(FIX2NUM(rgss.rb_iv_get(firstPage, "@trigger")) < 3){
				// real event convert
				RV graphics = rgss.rb_iv_get(firstPage, "@graphic");
				RV rcname = rgss.rb_iv_get(graphics, "@character_name");
				bool step_anime = rgss.rb_iv_get(firstPage, "@step_anime") == RR::Qtrue;
				char *character_name = rgss.rb_string_value_ptr(&rcname);
				int direction = FIX2NUM(rgss.rb_iv_get(graphics, "@direction"));
				int pattern = FIX2NUM(rgss.rb_iv_get(graphics, "@pattern"));
				int hue = FIX2NUM(rgss.rb_iv_get(graphics, "@character_hue"));
				int opacity = FIX2NUM(rgss.rb_iv_get(graphics, "@opacity"));
				int tile_id = FIX2NUM(rgss.rb_iv_get(graphics, "@tile_id"));
				bool noneCharacterName = character_name[0] == 0;
				bool cheapEvent = false;
				if (pageCount == 2) {
					RV secondPage = RR::RARRAY_PTR(pages)[1];
					if (RR::RARRAY_LEN(rgss.rb_iv_get(secondPage, "@list")) == 1) {
						RV condition = rgss.rb_iv_get(secondPage, "@condition");
						if (rgss.rb_iv_get(condition, "@self_switch_valid") == RR::Qtrue) {
							RV switchName = rgss.rb_iv_get(condition, "@self_switch_ch");
							char *selfSwitchSymbol = rgss.rb_string_value_ptr(&switchName);
							if (strcmp(selfSwitchSymbol, "A") == 0) {
								RV graphics2 = rgss.rb_iv_get(RR::RARRAY_PTR(pages)[1], "@graphic");
								RV rcname = rgss.rb_iv_get(graphics2, "@character_name");
								char *character_name2 = rgss.rb_string_value_ptr(&rcname);
								if (strlen(character_name2) == 0) {
									cheapEvent = true;
									pageCount = 1;
								}
							}
						}
					}
				}
				Json::Value allPageEvents;
				bool isErrorHappened = false;
				for (int pageIndex = 0; pageIndex < pageCount; pageIndex++) {
					Json::Value thisPageEvents;
					thisPageEvents.resize(0);
					RV thisPage = RR::RARRAY_PTR(pages)[pageIndex];
					RV thisList = rgss.rb_iv_get(thisPage, "@list");
					int listSize = RR::RARRAY_LEN(thisList);
					RV *commands = RR::RARRAY_PTR(thisList);
					if (!runVirtualEvent(commands, listSize, thisPageEvents, 0, events[i], cheapEvent, firstPage)) {
						isErrorHappened = true;
					}
					allPageEvents.append(thisPageEvents);
				}
				int h5eventId = 0;
				if (!noneCharacterName || tile_id>0) {
					if (tile_id>384) {
						sprintf_s(name, "%s,%d,%d,%d", mapInfo.tilesetName, hue, opacity, tile_id);
						if (log["uknBlocks"][name].isNull()) {
							h5eventId = currentBlockIndex;
							sprintf_s(outputName, "unknownEvent%d", h5eventId);
							registerBlock("npcs", outputName, currentBlockIndex);
							int dstY = jsonIcon["npcs"][outputName].asInt() * 32;
							sprintf_s(extname, "Graphics\\Tilesets\\%s", mapInfo.tilesetName);
							rgss.rgss_get_filefullname(extname, fullname);
							sprintf_s(extname, "%s\\project\\images\\npcs.png", h5Dir);
							int gx = (tile_id - 384) % 8 * 32;
							int gy = (tile_id - 384) / 8 * 32;
							bltImage(fullname, extname, gx, gy, 32, 32, 0, dstY, hue, opacity);
							bltImage(fullname, extname, gx, gy, 32, 32, 32, dstY, hue, opacity);
							log["uknBlocks"][name] = h5eventId;
						}
					}
					else {
						sprintf_s(name, "%s,%d,%d,%d,%d", character_name, hue, opacity, direction, pattern);
						if (log["uknBlocks"][name].isNull()) {
							h5eventId = currentBlockIndex;
							sprintf_s(outputName, "unknownEvent%d", h5eventId);
							registerBlock("npcs", outputName, currentBlockIndex);
							int dstY = jsonIcon["npcs"][outputName].asInt() * 32;
							sprintf_s(extname, "Graphics\\Characters\\%s", character_name);
							rgss.rgss_get_filefullname(extname, fullname);
							sprintf_s(extname, "%s\\project\\images\\npcs.png", h5Dir);
							if (step_anime) {
								bltImage(fullname, extname, 32, (direction / 2 - 1) * 32, 64, 32, 0, dstY, hue, opacity);
							}
							else {
								bltImage(fullname, extname, pattern * 32, (direction / 2 - 1) * 32, 32, 32, 0, dstY, hue, opacity);
								bltImage(fullname, extname, pattern * 32, (direction / 2 - 1) * 32, 32, 32, 32, dstY, hue, opacity);
							}
							log["uknBlocks"][name] = h5eventId;
						}
						else {
							h5eventId = log["uknBlocks"][name].asInt();
						}
					}
					
				}
				Json::Value thisEventContents;
				if (pageCount == 1) {
					thisEventContents = allPageEvents[0];
				}
				else {
					Json::Value switchEvent;
					switchEvent["type"] = "switch";
					switchEvent["condition"] = "事件页";
					switchEvent["caseList"].resize(0);
					for (int pi = 0; pi < pageCount; pi++) {
						Json::Value swcondition;
						swcondition["case"] = pi;
						swcondition["action"] = allPageEvents[pi];
						switchEvent["caseList"].append(swcondition);
					}
					thisEventContents.append(switchEvent);
				}
				sprintf_s(outputName, "%d,%d", event_x, event_y);
				if (thisEventContents.size() > 0) {
					mapInfo.mapjson["events"][outputName] = thisEventContents;
				}
				RV rname = rgss.rb_iv_get(events[i], "@name");
				Tool::convertString(name, rgss.rb_string_value_ptr(&rname));
				if (isErrorHappened) {
					sprintf_s(outputName, "%d号地图[%s],%d号事件%s[%d,%d]：无法完全识别的事件内容。(H5ID:%d)\n", mapInfo.mapId, mapInfo.mapTitle.c_str(), eventId, name, ori_x, ori_y, h5eventId);
					errorLog += outputName;
				}
				mapInfo.mapjson[layer2name[1]][event_y][event_x] = h5eventId;
			}
			else {
				RV rname = rgss.rb_iv_get(events[i], "@name");
				Tool::convertString(name, rgss.rb_string_value_ptr(&rname));
				sprintf_s(outputName, "%d号地图[%s],%d号事件%s[%d,%d]：自动和并行事件无法转换。\n", mapInfo.mapId, mapInfo.mapTitle.c_str(), eventId, name, ori_x, ori_y);
				errorLog += outputName;
			}
		}
	}
}

bool Convertor::runVirtualEvent(RV* commands, int listSize, Json::Value& thisPageEvents, int startIndex, RV thisEvent,bool cheapEvent,RV firstPage) {
	bool ret = true;
	VirtualEvent *interpreter = new VirtualEvent(commands, listSize, thisPageEvents, rgss, cheapEvent, startIndex, jsonConfig, log, thisEvent);
	interpreter->run();
	ret = interpreter->state != 1;
	int len = interpreter->myLog["enemies"].size();
	for (int i = 0; i < len; i++) {
		int enemyId = interpreter->myLog["enemies"][i].asInt();
		pullEnemyImage(enemyId, firstPage);
	}
	if (options.convertAudio) {
		len = interpreter->myLog["bgms"].size();
		for (int i = 0; i < len; i++) {
			pullBGM(interpreter->myLog["bgms"][i].asCString());
		}
		len = interpreter->myLog["se"].size();
		for (int i = 0; i < len; i++) {
			pullSE(interpreter->myLog["se"][i].asCString());
		}
	}
	delete interpreter;
	return ret;
}

void Convertor::pullBGM(const char* bgmName) {
	static char dstDir[MAX_PATH];
	if (log["bgmsDone"][bgmName].isNull()) {
		sprintf_s(dstDir, "%s\\project\\sounds\\%s", h5Dir, log["bgms"][bgmName][0].asCString());
		moveSound((char*)log["bgms"][bgmName][1].asCString(), dstDir);
		jsonData["main"]["bgms"].append(log["bgms"][bgmName][0]);
		log["bgmsDone"][bgmName] = true;
	}
}

void Convertor::pullSE(const char* seName) {
	static char dstDir[MAX_PATH];
	if (log["seDone"][seName].isNull()) {
		sprintf_s(dstDir, "%s\\project\\sounds\\%s", h5Dir, log["se"][seName][0].asCString());
		moveSound((char*)log["se"][seName][1].asCString(), dstDir);
		jsonData["main"]["sounds"].append(log["se"][seName][0]);
		log["seDone"][seName] = true;
	}
}

int Convertor::pullDoorImage(const char* doorName, RV firstPage) {
	static char srcDir[MAX_PATH];
	static char dstDir[MAX_PATH];
	int doorId = 0;
	if (log["door"][doorName].isNull()) {
		int tindex = 0, aindex = 0;
		int tsize = jsonIcon["terrains"].size();
		if (jsonIcon["terrains"][doorName].isNull()) {
			jsonIcon["terrains"][doorName] = tsize;
			tindex = tsize;
		}
		else {
			tindex = jsonIcon["terrains"][doorName].asInt();
		}
		int asize = jsonIcon["animates"].size();
		if (jsonIcon["animates"][doorName].isNull()) {
			jsonIcon["animates"][doorName] = asize;
			aindex = asize;
		}
		else {
			aindex = jsonIcon["animates"][doorName].asInt();
		}
		bool foundDoor = false;
		Json::Value::Members mem = jsonMap.getMemberNames();
		for (auto iter = mem.begin(); iter != mem.end(); iter++) {
			if (strcmp(jsonMap[*iter]["id"].asCString(), doorName) == 0) {
				doorId = atoi(iter[0].c_str());
				foundDoor = true;
				break;
			}
		}
		if (!foundDoor) {
			static char bid[10];
			sprintf_s(bid, "%d", currentBlockIndex);
			Json::Value content;
			content["cls"] = "terrains";
			content["id"] = doorName;
			content["trigger"] = "openDoor";
			jsonMap[bid] = content;
			doorId = currentBlockIndex;
			currentBlockIndex++;
		}
		log["door"][doorName] = doorId;
		RV graphics = rgss.rb_iv_get(firstPage, "@graphic");
		RV rcname = rgss.rb_iv_get(graphics, "@character_name");
		char *character_name = rgss.rb_string_value_ptr(&rcname);
		if (character_name[0] != 0) {
			sprintf_s(dstDir, "Graphics\\Characters\\%s", character_name);
			rgss.rgss_get_filefullname(dstDir, srcDir);
			sprintf_s(dstDir, "%s\\project\\images\\animates.png", h5Dir);
			sprintf_s(outputName, "%s\\project\\images\\terrains.png", h5Dir);
			int hue = FIX2NUM(rgss.rb_iv_get(graphics, "@character_hue"));
			int opacity = FIX2NUM(rgss.rb_iv_get(graphics, "@opacity"));
			int dstY = aindex * 32;
			int srcX = FIX2NUM(rgss.rb_iv_get(graphics, "@pattern")) * 32;
			bltImage(srcDir, dstDir, srcX, 0, 32, 32, 0, dstY, hue, opacity);
			bltImage(srcDir, dstDir, srcX, 32, 32, 32, 32, dstY, hue, opacity);
			bltImage(srcDir, dstDir, srcX, 64, 32, 32, 64, dstY, hue, opacity);
			bltImage(srcDir, dstDir, srcX, 96, 32, 32, 96, dstY, hue, opacity);
			bltImage(srcDir, outputName, srcX, 0, 32, 32, 0, tindex * 32, hue, opacity);
		}
		else {
			sprintf_s(dstDir, "%s\\project\\images\\animates.png", h5Dir);
			sprintf_s(outputName, "%s\\project\\images\\terrains.png", h5Dir);
			int dstY = aindex * 32;
			clearImage(dstDir, 0, dstY, 128, 32);
			clearImage(outputName, 0, tindex * 32, 32, 32);
		}

	}
	else {
		doorId = log["door"][doorName].asInt();
	}
	return doorId;
}

int Convertor::pullItemImage(const char* itemName, RV firstPage) {
	static char srcDir[MAX_PATH];
	static char dstDir[MAX_PATH];
	int itemId = 0;
	if (log["items"][itemName].isNull()) {
		int tindex = 0;
		int tsize = jsonIcon["items"].size();
		if (jsonIcon["items"][itemName].isNull()) {
			jsonIcon["items"][itemName] = tsize;
			tindex = tsize;
		}
		else {
			tindex = jsonIcon["items"][itemName].asInt();
		}
		bool foundItem = false;
		Json::Value::Members mem = jsonMap.getMemberNames();
		for (auto iter = mem.begin(); iter != mem.end(); iter++) {
			if (strcmp(jsonMap[*iter]["id"].asCString(), itemName) == 0) {
				itemId = atoi(iter[0].c_str());
				foundItem = true;
				break;
			}
		}
		if (!foundItem) {
			static char bid[10];
			sprintf_s(bid, "%d", currentBlockIndex);
			Json::Value content;
			content["cls"] = "items";
			content["id"] = itemName;
			jsonMap[bid] = content;
			itemId = currentBlockIndex;
			currentBlockIndex++;
		}
		log["items"][itemName] = itemId;
		RV graphics = rgss.rb_iv_get(firstPage, "@graphic");
		RV rcname = rgss.rb_iv_get(graphics, "@character_name");
		char *character_name = rgss.rb_string_value_ptr(&rcname);
		if (character_name[0] != 0) {
			sprintf_s(dstDir, "Graphics\\Characters\\%s", character_name);
			rgss.rgss_get_filefullname(dstDir, srcDir);
			sprintf_s(dstDir, "%s\\project\\images\\items.png", h5Dir);
			int hue = FIX2NUM(rgss.rb_iv_get(graphics, "@character_hue"));
			int opacity = FIX2NUM(rgss.rb_iv_get(graphics, "@opacity"));
			int dstY = tindex * 32;
			int srcX = FIX2NUM(rgss.rb_iv_get(graphics, "@pattern")) * 32;
			int srcY = (FIX2NUM(rgss.rb_iv_get(graphics, "@direction")) / 2 - 1) * 32;
			bltImage(srcDir, dstDir, srcX, srcY, 32, 32, 0, dstY, hue, opacity);
		}
		else {
			sprintf_s(dstDir, "%s\\project\\images\\items.png", h5Dir);
			int dstY = tindex * 32;
			clearImage(dstDir, 0, dstY, 32, 32);
		}
	}
	else {
		itemId = log["items"][itemName].asInt();
	}
	return itemId;
}

int Convertor::pullPotionImage(const char* potionName, RV firstPage) {
	static char srcDir[MAX_PATH];
	static char dstDir[MAX_PATH];
	int potionId = 0;
	if (log["gainHp"][potionName].isNull()) {
		int tindex = 0;
		int tsize = jsonIcon["items"].size();
		if (jsonIcon["items"][potionName].isNull()) {
			jsonIcon["items"][potionName] = tsize;
			tindex = tsize;
		}
		else {
			tindex = jsonIcon["items"][potionName].asInt();
		}
		bool foundPotion = false;
		Json::Value::Members mem = jsonMap.getMemberNames();
		for (auto iter = mem.begin(); iter != mem.end(); iter++) {
			if (strcmp(jsonMap[*iter]["id"].asCString(), potionName) == 0) {
				potionId = atoi(iter[0].c_str());
				foundPotion = true;
				break;
			}
		}
		if (!foundPotion) {
			static char bid[10];
			sprintf_s(bid, "%d", currentBlockIndex);
			Json::Value content;
			content["cls"] = "items";
			content["id"] = potionName;
			jsonMap[bid] = content;
			potionId = currentBlockIndex;
			currentBlockIndex++;
		}
		log["gainHp"][potionName] = potionId;
		RV graphics = rgss.rb_iv_get(firstPage, "@graphic");
		RV rcname = rgss.rb_iv_get(graphics, "@character_name");
		char *character_name = rgss.rb_string_value_ptr(&rcname);
		if (character_name[0] != 0) {
			sprintf_s(dstDir, "Graphics\\Characters\\%s", character_name);
			rgss.rgss_get_filefullname(dstDir, srcDir);
			sprintf_s(dstDir, "%s\\project\\images\\items.png", h5Dir);
			int hue = FIX2NUM(rgss.rb_iv_get(graphics, "@character_hue"));
			int opacity = FIX2NUM(rgss.rb_iv_get(graphics, "@opacity"));
			int dstY = tindex * 32;
			int srcX = FIX2NUM(rgss.rb_iv_get(graphics, "@pattern")) * 32;
			int srcY = (FIX2NUM(rgss.rb_iv_get(graphics, "@direction")) / 2 - 1) * 32;
			bltImage(srcDir, dstDir, srcX, srcY, 32, 32, 0, dstY, hue, opacity);
		}
		else {
			sprintf_s(dstDir, "%s\\project\\images\\items.png", h5Dir);
			int dstY = tindex * 32;
			clearImage(dstDir, 0, dstY, 32, 32);
		}

	}
	else {
		potionId = log["gainHp"][potionName].asInt();
	}
	return potionId;
}

int Convertor::pullPowerUpImage(const char* powerName, RV firstPage) {
	static char srcDir[MAX_PATH];
	static char dstDir[MAX_PATH];
	int powerId = 0;
	if (log["power"][powerName].isNull()) {
		int tindex = 0;
		int tsize = jsonIcon["items"].size();
		if (jsonIcon["items"][powerName].isNull()) {
			jsonIcon["items"][powerName] = tsize;
			tindex = tsize;
		}
		else {
			tindex = jsonIcon["items"][powerName].asInt();
		}
		bool foundPower = false;
		Json::Value::Members mem = jsonMap.getMemberNames();
		for (auto iter = mem.begin(); iter != mem.end(); iter++) {
			if (strcmp(jsonMap[*iter]["id"].asCString(), powerName) == 0) {
				powerId = atoi(iter[0].c_str());
				foundPower = true;
				break;
			}
		}
		if (!foundPower) {
			static char bid[10];
			sprintf_s(bid, "%d", currentBlockIndex);
			Json::Value content;
			content["cls"] = "items";
			content["id"] = powerName;
			jsonMap[bid] = content;
			powerId = currentBlockIndex;
			currentBlockIndex++;
		}
		log["power"][powerName] = powerId;
		RV graphics = rgss.rb_iv_get(firstPage, "@graphic");
		RV rcname = rgss.rb_iv_get(graphics, "@character_name");
		char *character_name = rgss.rb_string_value_ptr(&rcname);
		if (character_name[0] != 0) {
			sprintf_s(dstDir, "Graphics\\Characters\\%s", character_name);
			rgss.rgss_get_filefullname(dstDir, srcDir);
			sprintf_s(dstDir, "%s\\project\\images\\items.png", h5Dir);
			int hue = FIX2NUM(rgss.rb_iv_get(graphics, "@character_hue"));
			int opacity = FIX2NUM(rgss.rb_iv_get(graphics, "@opacity"));
			int dstY = tindex * 32;
			int srcX = FIX2NUM(rgss.rb_iv_get(graphics, "@pattern")) * 32;
			int srcY = (FIX2NUM(rgss.rb_iv_get(graphics, "@direction")) / 2 - 1) * 32;
			bltImage(srcDir, dstDir, srcX, srcY, 32, 32, 0, dstY, hue, opacity);
		}
		else {
			sprintf_s(dstDir, "%s\\project\\images\\items.png", h5Dir);
			int dstY = tindex * 32;
			clearImage(dstDir, 0, dstY, 32, 32);
		}
	}
	else {
		powerId = log["power"][powerName].asInt();
	}
	return powerId;
}

int Convertor::pullEnemyImage(int enemyId,RV firstPage) {
	static char name[50];
	static char srcDir[MAX_PATH];
	static char dstDir[MAX_PATH];
	sprintf_s(name, "exEnemy%d", enemyId);
	int enemyCode = 0;
	if (jsonEnemy[name].isNull()) {
		pullEnemyData(name, enemyId);
		enemyCode = 300 + enemyId;
		sprintf_s(srcDir, "%d", enemyCode);
		Json::Value content;
		content["cls"] = "enemys";
		content["id"] = name;
		jsonMap[srcDir] = content;
		if (jsonIcon["enemys"][name].isNull()) {
			jsonIcon["enemys"][name] = enemyId - 1;
		}
		registerBlock("enemys", name, currentBlockIndex);
		RV graphics = rgss.rb_iv_get(firstPage, "@graphic");
		RV rcname = rgss.rb_iv_get(graphics, "@character_name");
		char *character_name = rgss.rb_string_value_ptr(&rcname);
		if (character_name[0] != 0) {
			sprintf_s(dstDir, "Graphics\\Characters\\%s", character_name);
			rgss.rgss_get_filefullname(dstDir, srcDir);
			sprintf_s(dstDir, "%s\\project\\images\\enemys.png", h5Dir);
			int bltY = (FIX2NUM(rgss.rb_iv_get(graphics, "@direction")) / 2 - 1) * 32;
			int hue = FIX2NUM(rgss.rb_iv_get(graphics, "@character_hue"));
			int opacity = FIX2NUM(rgss.rb_iv_get(graphics, "@opacity"));
			bltImage(srcDir, dstDir, 32, bltY, 64, 32, 0, jsonIcon["enemys"][name].asInt() * 32, hue, opacity);
		}
		else {
			sprintf_s(dstDir, "%s\\project\\images\\enemys.png", h5Dir);
			int bltY = (FIX2NUM(rgss.rb_iv_get(graphics, "@direction")) / 2 - 1) * 32;
			clearImage(dstDir, 0, jsonIcon["enemys"][name].asInt() * 32, 64, 32);
		}
		log["enemy"][name] = enemyCode;
	}
	else {
		enemyCode = log["enemy"][name].asInt();
	}
	return enemyCode;
}

void Convertor::pullEnemyData(char* name,int enemyId) {
	sprintf_s(scripts, scriptGetEnemyData, enemyId);
	rgss.eval(scripts);
	RV renemy = rgss.rdata;
	Json::Value enemy;
	RV rname = rgss.rb_iv_get(renemy, "@name");
	char *bname = rgss.rb_string_value_ptr(&rname);
	char *pTmp = NULL;
	strtok_s(bname, ":", &pTmp);
	Tool::convertString(outputName, bname);
	enemy["name"] = outputName;
	enemy["hp"] = FIX2NUM(rgss.rb_iv_get(renemy, "@maxhp"));
	enemy["atk"] = FIX2NUM(rgss.rb_iv_get(renemy, "@atk"));
	enemy["def"] = FIX2NUM(rgss.rb_iv_get(renemy, "@pdef"));
	enemy["money"] = FIX2NUM(rgss.rb_iv_get(renemy, "@gold"));
	enemy["experience"] = FIX2NUM(rgss.rb_iv_get(renemy, "@exp"));
	enemy["point"] = 0;
	enemy["special"] = 0;
	jsonEnemy[name] = enemy;
}

void Convertor::getRGSSMapInfo(int mapId) {
	char rgssMapName[MAX_PATH];
	mapInfo.mapId = mapId;
	mapInfo.mapjson.clear();
	sprintf_s(rgssMapName, "Data/Map%03d.rxdata", mapId);
	sprintf_s(scripts, scriptGetMapData, rgssMapName, mapId);
	rgss.eval(scripts);
	RV *args = RR::RARRAY_PTR(rgss.rdata);
	char mt[50];
	Tool::convertString(mt, rgss.rb_string_value_ptr(&args[1]));
	mapInfo.mapTitle = mt;
	mapInfo.tilesetName = rgss.rb_string_value_ptr(&args[0]);
	Data_Get_Struct(args[2], RR::RGSSTable, mapInfo.mapTable);
	mapInfo.autotileList = RR::RARRAY_PTR(args[3]);
	Data_Get_Struct(args[4], RR::RGSSTable, mapInfo.mapPass);
	mapInfo.canCutMap = (options.cutMap && mapInfo.mapTable->xsize == 20 && mapInfo.mapTable->ysize == 15);
	cout << rgssMapName << endl;
}

void Convertor::release() {
	delete globalDataStr;
	delete globalMapStr;
	delete globalIconStr;
	delete globalEnemyStr;
}

bool Convertor::loadH5Info() {
	sprintf_s(globalDataDir, "%s\\project\\data.js", h5Dir);
	sprintf_s(globalMapDir, "%s\\project\\maps.js", h5Dir);
	sprintf_s(globalIconDir, "%s\\project\\icons.js", h5Dir);
	sprintf_s(globalEnemyDir, "%s\\project\\enemys.js", h5Dir);
	globalDataStr = Tool::ReadEntireUTF8File(globalDataDir);
	globalMapStr = Tool::ReadEntireUTF8File(globalMapDir);
	globalIconStr = Tool::ReadEntireUTF8File(globalIconDir);
	globalEnemyStr = Tool::ReadEntireUTF8File(globalEnemyDir);
	jsonDataStr = Tool::GetJsonBegin(globalDataStr);
	jsonMapStr = Tool::GetJsonBegin(globalMapStr);
	jsonIconStr = Tool::GetJsonBegin(globalIconStr);
	jsonEnemyStr = Tool::GetJsonBegin(globalEnemyStr);
	JSONCPP_STRING errs;

	/*regex removeCommit("//.*");
	string finalMapStr = regex_replace(jsonMapStr, removeCommit, "");*/
	int gbdataLen = strlen(jsonDataStr);
	int gbmapLen = strlen(jsonMapStr);
	int gbiconLen = strlen(jsonIconStr);
	int gbenemyLen = strlen(jsonEnemyStr);
	
	Json::CharReader* reader = charbuilder.newCharReader();
	if (!reader->parse(jsonDataStr, jsonDataStr + gbdataLen, &jsonData, &errs))
	{
		release();
		return false;
	}
	if (!reader->parse(jsonMapStr, jsonMapStr + gbmapLen, &jsonMap, &errs))
	{
		release();
		return false;
	}
	if (!reader->parse(jsonIconStr, jsonIconStr + gbiconLen, &jsonIcon, &errs))
	{
		release();
		return false;
	}
	jsonEnemy.clear();
	jsonEnemy = Json::objectValue;
	/*if (!reader->parse(jsonEnemyStr, jsonEnemyStr + gbenemyLen, &jsonEnemy, &errs))
	{
		release();
		return false;
	}*/
	return true;
}

void Convertor::preprocessData() {
	jsonData["main"]["floorIds"].resize(0);
	jsonData["main"]["tilesets"].resize(0);
	jsonData["main"]["images"].resize(0);
	jsonData["main"]["bgms"].resize(0);
	jsonData["main"]["sound"].resize(0);

	jsonData["main"]["images"].append("bg.jpg");
	jsonData["main"]["images"].append("winskin.png");
	jsonData["main"]["bgms"].append("bgm.mp3");
	jsonData["main"]["sound"].append("floor.mp3");
	jsonData["main"]["sound"].append("attack.mp3");
	jsonData["main"]["sound"].append("door.mp3");
	jsonData["main"]["sound"].append("item.mp3");
	jsonData["main"]["sound"].append("equip.mp3");
	jsonData["main"]["sound"].append("zone.mp3");
	jsonData["main"]["sound"].append("jump.mp3");
	jsonData["main"]["sound"].append("pickaxe.mp3");
	jsonData["main"]["sound"].append("bomb.mp3");
	jsonData["main"]["sound"].append("centerFly.mp3");
}

void Convertor::writeJsonMapFile() {
	char test[] = "\"\\\b\f\n\r\t";
	char jsAdapt[50];
	sprintf_s(jsAdapt, "main.floors.%s = \n", mapInfo.mapName);
	string json_file = jsAdapt + Json::writeString(builder, mapInfo.mapjson);
	sprintf_s(outputName, "%s\\project\\floors\\fux%d.js", h5Dir, mapInfo.mapId);
	writeFile(&json_file, outputName);
}

void Convertor::writeJsonGlobalDataFile() {
	jsonDataStr[0] = 0;
	auto test = jsonData["main"]["levelChoose"][0][0].asCString();
	Tool::convertString(outputName, (char*)test);
	string data_file = globalDataStr + Json::writeString(builder, jsonData);
	writeFile(&data_file, globalDataDir);

	jsonMapStr[0] = 0;
	string map_file = globalMapStr + Json::writeString(builder, jsonMap);
	writeFile(&map_file, globalMapDir);

	jsonIconStr[0] = 0;
	string icon_file = globalIconStr + Json::writeString(builder, jsonIcon);
	writeFile(&icon_file, globalIconDir);

	jsonEnemyStr[0] = 0;
	string enemy_file = globalEnemyStr + Json::writeString(builder, jsonEnemy);
	writeFile(&enemy_file, globalEnemyDir);
	cout << "end" << endl;
}

void Convertor::setupJsonMapTileData() {
	static char fullname[MAX_PATH];
	static char extname[MAX_PATH];
	static char blockH5Id[15];
	//static char *baseName = 0;
	unsigned int picWidth = 0, picHeight = 0;
	int currentTileIndex = getTilesetIndex(mapInfo.tilesetName);
	bool canCutMap = mapInfo.canCutMap;
	long xstart = canCutMap ? options.cutX : 0;
	long xend = canCutMap ? options.cutX+options.cutWidth : mapInfo.mapTable->xsize;
	long ystart = canCutMap ? options.cutY : 0;
	long yend = canCutMap ? options.cutY+options.cutWidth : mapInfo.mapTable->ysize;
	int autotileIndex = 0;
	for (long z = 0; z < mapInfo.mapTable->zsize; z++) {
		Json::Value layerData;
		for (long y = ystart; y < yend; y++) {
			Json::Value lineData;
			for (long x = xstart; x < xend; x++) {
				WORD oriTileId = mapInfo.mapTable->data_ptr[x + mapInfo.mapTable->xsize * (y + mapInfo.mapTable->ysize * z)];
				unsigned int finalTileId = 0;
				if (oriTileId >= 384) {
					finalTileId = oriTileId - 384 + currentTileIndex;
				}
				else if (oriTileId >= 48) {
					autotileIndex = (oriTileId - 48) / 48;
					char *autotileFileName = rgss.rb_string_value_ptr(&mapInfo.autotileList[autotileIndex]);
					if (autotileFileName[0] != 0) {
						if (log["bmp"]["autotile"][autotileFileName].isNull()) {
							sprintf_s(extname, "Graphics\\Autotiles\\%s", autotileFileName);
							rgss.rgss_get_filefullname(extname, fullname);
							//Tool::GetFileBaseName(&baseName, fullname);
							sprintf_s(blockH5Id, "exAutoTile%d", currentBlockIndex);
							sprintf_s(extname, "%s\\project\\images\\%s.png", h5Dir, blockH5Id);

							Json::Value remtile;
							remtile["dir"] = fullname;
							remtile["h5Id"] = currentBlockIndex;
							remtile["aid"] = blockH5Id;
							Tool::GetPicWidthHeight(fullname, &picWidth, &picHeight);
							if (picWidth == 128 && picHeight == 32) {
								registerBlock("animates", blockH5Id, currentBlockIndex);
								sprintf_s(extname, "%s\\project\\images\\animates.png", h5Dir);
								bltImage(fullname, extname, 0, 0, 128, 32, 0, jsonIcon["animates"][blockH5Id].asInt() * 32, 0, 255);
							}
							else {
								registerBlock("autotile", blockH5Id, currentBlockIndex);
								moveImage(fullname, extname);
							}
							log["bmp"]["autotile"][autotileFileName] = remtile;
							finalTileId = remtile["h5Id"].asInt();
						}
						else {
							finalTileId = log["bmp"]["autotile"][autotileFileName]["h5Id"].asInt();
						}
					}
					else {
						finalTileId = 0;
					}
					
				}
				lineData.append(finalTileId);
			}
			layerData.append(lineData);
		}
		mapInfo.mapjson[layer2name[z]] = layerData;
	}
}

void Convertor::setupJsonMapInfo() {
	sprintf_s(mapInfo.mapName, "fux%d", mapInfo.mapId);
	mapInfo.mapjson["floorId"] = mapInfo.mapName;
	mapInfo.mapjson["title"] = mapInfo.mapTitle;
	mapInfo.mapjson["name"] = mapInfo.mapTitle;
	mapInfo.mapjson["canFlyTo"] = true;
	mapInfo.mapjson["canUseQuickShop"] = true;
	mapInfo.mapjson["cannotViewMap"] = false;
	mapInfo.mapjson["images"].resize(0);
	mapInfo.mapjson["item_ratio"] = 1;
	mapInfo.mapjson["defaultGround"] = "ground";
	rgss.eval(scriptGetMapOri);
	RV RMapData = rgss.rdata;
	if (rgss.rb_iv_get(RMapData, "@autoplay_bgm") == RR::Qtrue) {
		RV bgmData = rgss.rb_iv_get(RMapData, "@bgm");
		RV bgmName = rgss.rb_iv_get(bgmData, "@name");
		Tool::convertString(outputName, rgss.rb_string_value_ptr(&bgmName));
		if (log["bgms"][outputName].isNull()) {
			static char srcDir[MAX_PATH];
			static char dstDir[MAX_PATH];
			static char h5Name[MAX_PATH];
			sprintf_s(dstDir, "Audio\\BGM\\%s", outputName);
			rgss.rgss_get_filefullname(dstDir, srcDir);
			char *extendName;
			Tool::GetFileExtendName(&extendName, srcDir);
			sprintf_s(h5Name, "exBGM%d.%s", log["bgmCount"].asInt(), extendName);
			log["bgms"][outputName].append(h5Name);
			log["bgms"][outputName].append(srcDir);
			log["bgmCount"] = log["bgmCount"].asInt() + 1;
			pullBGM(outputName);
			mapInfo.mapjson["bgm"] = h5Name;
		}
		else {
			mapInfo.mapjson["bgm"] = log["bgms"][outputName][0];
		}
	}
	else {
		mapInfo.mapjson["bgm"] = Json::nullValue;
	}
	mapInfo.mapjson["firstArrive"].resize(0);
	mapInfo.mapjson["parallelDo"] = "";
	mapInfo.mapjson["events"] = Json::objectValue;
	mapInfo.mapjson["changeFloor"] = Json::objectValue;
	mapInfo.mapjson["afterBattle"] = Json::objectValue;
	mapInfo.mapjson["afterGetItem"] = Json::objectValue;
	mapInfo.mapjson["afterOpenDoor"] = Json::objectValue;
	mapInfo.mapjson["cannotMove"] = Json::objectValue;
	mapInfo.mapjson["width"] = mapInfo.canCutMap ? options.cutWidth : mapInfo.mapTable->xsize;
	mapInfo.mapjson["height"] = mapInfo.canCutMap ? options.cutWidth : mapInfo.mapTable->ysize;
	setupJsonMapTileData();
}

void Convertor::addMapToGlobalData() {
	jsonData["main"]["floorIds"].append(mapInfo.mapName);
}

void Convertor::processGlobalData() {
	Json::ArrayIndex tilesetLength = tilesetOrder.size();
	for (unsigned long i = 0; i < tilesetLength; i++) {
		jsonData["main"]["tilesets"].append(tilesetRem[tilesetOrder[(int)i].asCString()]["baseName"]);
	}
	//rgss.eval(scriptGetStartPos);
	//RV *posInfo = RR::RARRAY_PTR(rgss.rdata);
	//char name[15];
	//sprintf_s(name, "fux%d", FIX2NUM(posInfo[0]));
	jsonData["firstData"]["floorId"] = "fux1";
	//jsonData["firstData"]["hero"]["loc"]["x"] = FIX2NUM(posInfo[1]);
	//jsonData["firstData"]["hero"]["loc"]["y"] = FIX2NUM(posInfo[2]);
	//jsonData["firstData"]["hero"]["loc"]["direction"] = "down";//dir2name[FIX2NUM(posInfo[3]) / 2 - 1];
	jsonData["firstData"]["startText"].resize(0);
	jsonData["firstData"]["startText"].append("RGSS2H5 2019.1 Fux2 && ckcz123");
}

bool Convertor::getAppOptions(char** arguments,int argc) {
	rmDir = Tool::char2wchar(arguments[1]);
	int len = strlen(arguments[2]);
	h5Dir = new char[len + 1];
	memcpy(h5Dir, arguments[2], len);
	h5Dir[len] = 0;

	options.cutMap = false;
	options.convertAudio = false;
	options.forceStart = 0;
	options.forceEnd = 0;
	options.onlyConfig = false;
	for (int i = 3; i < argc; i++) {
		string easystr = arguments[i];
		if (easystr == "-c" || easystr == "-cutMap") {
			options.cutMap = true;
			i++;
			int argn = strtoul(arguments[i], NULL, 10);
			if (argn == 13) {
				options.cutWidth = 13;
				options.cutX = 6;
				options.cutY = 1;
			}
			else if (argn == 15) {
				options.cutWidth = 15;
				options.cutX = 5;
				options.cutY = 0;
			}
			else {
				return false;
			}
		}
		else if (easystr == "-f" || easystr == "-force") {
			i++;
			int argn = strtoul(arguments[i], NULL, 10);
			if (argn != 0) {
				options.forceStart = argn;
			}
			else {
				return false;
			}
			i++;
			argn = strtoul(arguments[i], NULL, 10);
			if (argn != 0 && argn>options.forceStart) {
				options.forceEnd = argn;
			}
			else {
				return false;
			}
		}
		else if (easystr == "-a" || easystr == "-audio") {
			options.convertAudio = true;
		}
		else if (easystr == "-o" || easystr == "-onlyConfig") {
			options.onlyConfig = true;
		}
	}
	return true;
}

void Convertor::createConfig(char* configDir) {
	Json::Value doorData;
	doorData["1"] = "yellowDoor";
	doorData["2"] = "blueDoor";
	doorData["3"] = "redDoor";
	doorData["5"] = "greenDoor";
	Json::Value doorName;
	doorName["黄门"] = "yellowDoor";
	doorName["蓝门"] = "blueDoor";
	doorName["红门"] = "redDoor";
	Json::Value potionData;
	potionData["1"] = "redPotion";
	potionData["2"] = "bluePotion";
	potionData["3"] = "yellowPotion";
	potionData["4"] = "greenPotion";
	Json::Value eventData;
	eventData["openDoor"] = 4;
	eventData["battle"] = 1;
	eventData["gainHp"] = 2;
	eventData["gainItem"] = 11;
	eventData["powerUp"] = 3;
	eventData["changeFloor"] = 7;
	eventData["ignoreCommonFloor"] = 17;
	eventData["ignoreBGMCheck"] = 29;
	eventData["eventEnd"] = 13;
	Json::Value varData;
	varData["doorId"] = 5;
	varData["itemId"] = 20;
	varData["itemType"] = 78;
	varData["powerUpType"] = 11;
	varData["potionId"] = 9;
	Json::Value swtData;
	swtData["upFloor"] = 2;
	Json::Value itemData;
	itemData["1"]["1"] = "yellowKey";
	itemData["1"]["2"] = "blueKey";
	itemData["1"]["3"] = "redKey";
	itemData["1"]["12"] = "fly";
	itemData["1"]["4"] = "book";
	itemData["1"]["9"] = "pickaxe";
	itemData["1"]["13"] = "centerFly";
	itemData["1"]["17"] = "superPotion";
	itemData["1"]["18"] = "bomb";
	itemData["1"]["20"] = "coin";
	itemData["1"]["41"] = "greenKey";
	Json::Value powerData;
	powerData["1"] = "redJewel";
	powerData["51"] = "blueJewel";
	powerData["151"] = "greenJewel";
	jsonConfig["doorIndex2Id"] = doorData;
	jsonConfig["doorName2Id"] = doorName;
	jsonConfig["commonEventList"] = eventData;
	jsonConfig["variableList"] = varData;
	jsonConfig["switchList"] = swtData;
	jsonConfig["itemIndex2Id"] = itemData;
	jsonConfig["powerIndex2Id"] = powerData;
	jsonConfig["potionIndex2Id"] = potionData;
	jsonConfig["configVersion"] = versionCode;
	string json_file = Json::writeString(builder, jsonConfig);
	writeFile(&json_file, configDir);
}

bool Convertor::getAppConfig(char *exeDir) {
	char configDir[MAX_PATH];
	workDir = Tool::GetDirByFullName(exeDir);
	sprintf_s(configDir, "%s\\config.json", h5Dir);
	char *configStr = Tool::ReadEntireUTF8File(configDir);
	jsonConfig.clear();
	if (configStr) {
		int len = strlen(configStr);
		JSONCPP_STRING errs;
		Json::CharReader* reader = charbuilder.newCharReader();
		if (!reader->parse(configStr, configStr + len, &jsonConfig, &errs))
		{
			delete configStr;
			return false;
		}
		if (jsonConfig["configVersion"].isNull() || (unsigned int)jsonConfig["configVersion"].asInt() < versionCode) {
			createConfig(configDir);
		}
	}
	else {
		createConfig(configDir);
	}
	delete configStr;
	return true;
}

void Convertor::writeFile(string *data,char *dir) {
	std::ofstream ofs;
	char *realData = Tool::string2UTF8((char*)data[0].c_str());
	ofs.open(dir);
	ofs << realData;
	ofs.close();
	delete realData;
	realData = NULL;
}

unsigned int Convertor::getTilesetIndex(char* tilesetName) {
	unsigned int result = 0;
	if (tilesetRem[tilesetName].isNull()) {
		Json::Value TilesetHolder;
		char extname[MAX_PATH];
		char fullname[MAX_PATH];
		char baseName[MAX_PATH];
		//unsigned int width = 0, height = 0;
		sprintf_s(extname, "Graphics\\Tilesets\\%s", tilesetName);
		rgss.rgss_get_filefullname(extname,fullname);
		//Tool::GetPicWidthHeight(fullname, &width, &height);
		//Tool::GetFileBaseName(&baseName, fullname);
		sprintf_s(baseName, "exTileset%d.png", TilesetIndexCount);
		sprintf_s(extname, "%s\\project\\images\\%s", h5Dir, baseName);
		//Tool::CopyEntireFile(fullname, extname);
		moveImage(fullname, extname);
		TilesetHolder["startIndex"] = TilesetIndexCount;
		TilesetHolder["baseName"] = baseName;
		result = TilesetIndexCount;
		TilesetIndexCount += 10000;
		tilesetRem[tilesetName] = TilesetHolder;
		tilesetOrder.append(tilesetName);
	}
	else {
		result = tilesetRem[tilesetName]["startIndex"].asInt();
	}
	return result;
}

void Convertor::moveImage(char *srcDir, char *dstDir) {
	Json::Value arr;
	arr.append(srcDir);
	arr.append(dstDir);
	log["move"].append(arr);
	// test
	//Tool::CopyEntireFile(srcDir, dstDir);
}

void Convertor::moveSound(char *srcDir, char *dstDir) {
	Json::Value arr;
	arr.append(srcDir);
	arr.append(dstDir);
	log["moveSound"].append(arr);
	// test
	//Tool::CopyEntireFile(srcDir, dstDir);
}

void Convertor::bltImage(char *srcDir, char *dstDir, int sx, int sy, int sw, int sh, int dx, int dy, int hue, int opacity) {
	Json::Value commands;
	log["blt"][srcDir][dstDir].append(sx);
	log["blt"][srcDir][dstDir].append(sy);
	log["blt"][srcDir][dstDir].append(sw);
	log["blt"][srcDir][dstDir].append(sh);
	log["blt"][srcDir][dstDir].append(dx);
	log["blt"][srcDir][dstDir].append(dy);
	log["blt"][srcDir][dstDir].append(hue);
	log["blt"][srcDir][dstDir].append(opacity);
}

void Convertor::clearImage(char *dstDir, int dx, int dy, int dw, int dh) {
	Json::Value commands;
	log["clear"][dstDir].append(dx);
	log["clear"][dstDir].append(dy);
	log["clear"][dstDir].append(dw);
	log["clear"][dstDir].append(dh);
}

void Convertor::writeLogs() {
	char logDir[MAX_PATH];
	string logs = "move:\n";
	int len = log["move"].size();
	for (int i = 0; i < len; i++) {
		logs = logs + log["move"][i][0].asCString() + "\t" + log["move"][i][1].asCString() + "\n";
	}
	logs += "\n\n\nblt:\n";
	Json::Value::Members mem = log["blt"].getMemberNames();
	for (auto iter = mem.begin(); iter != mem.end(); iter++) {
		logs = logs + *iter + "\n";
		Json::Value::Members memIn = log["blt"][*iter].getMemberNames();
		for (auto iter2 = memIn.begin(); iter2 != memIn.end(); iter2++) {
			Json::Value obj = log["blt"][*iter][*iter2];
			int itemCount = obj.size() / 8;
			for (int zi = 0; zi < itemCount; zi++) {
				logs = logs + *iter2;
				for (int i = 0; i < 8; i++) {
					sprintf_s(outputName, "%d", obj[zi * 8 + i].asInt());
					logs = logs + "\t" + outputName;
				}
				logs += "\n";
			}
		}
	}
	logs += "\n\n\nclear:\n";
	mem = log["clear"].getMemberNames();
	for (auto iter = mem.begin(); iter != mem.end(); iter++) {
		logs = logs + *iter + "\n";
		Json::Value obj = log["clear"][*iter];
		int itemCount = obj.size() / 4;
		for (int zi = 0; zi < itemCount; zi++) {
			for (int i = 0; i < 4; i++) {
				sprintf_s(outputName, "%d", obj[zi * 8 + i].asInt());
				logs = logs + "\t" + outputName;
			}
			logs += "\n";
		}
	}
	logs += "\n\n\nmoveSound:\n";
	len = log["moveSound"].size();
	for (int i = 0; i < len; i++) {
		logs = logs + log["moveSound"][i][0].asCString() + "\t" + log["moveSound"][i][1].asCString() + "\n";
	}
	sprintf_s(logDir, "%s\\bitmapWorks.log", workDir);
	std::ofstream ofs;
	ofs.open(logDir);
	ofs << logs;
	ofs.close();
	
	sprintf_s(outputName, "%s\\error.log", h5Dir);
	ofs.open(outputName);
	ofs << errorLog;
	ofs.close();
}

void Convertor::registerBlock(const char* cls, char* aid, int index) {
	static char bid[10];
	sprintf_s(bid, "%d", index);
	Json::Value content;
	content["cls"] = cls;
	content["id"] = aid;
	jsonMap[bid] = content;
	Json::ArrayIndex nid = jsonIcon[cls].size();
	if (jsonIcon[cls][aid].isNull()) {
		jsonIcon[cls][aid] = nid;
	}
	currentBlockIndex++;
}

char Convertor::scriptGetAllMapList[] = "$holdmapdata = $data_mapinfo.keys.sort_by{|i| $data_mapinfo[i].order};Fux2.sendData($holdmapdata)";
char Convertor::scriptGetMapName[] = "$holdstring = $data_mapinfo[%d].name;Fux2.sendData($holdstring)";
char Convertor::scriptGetMapData[] = "$itmap = load_data(\"%s\");Fux2.sendData([$data_tilesets[$itmap.tileset_id].tileset_name,$data_mapinfo[%d].name,$itmap.data,$data_tilesets[$itmap.tileset_id].autotile_names,$data_tilesets[$itmap.tileset_id].passages])";
char Convertor::scriptGetMapOri[] = "Fux2.sendData($itmap)";
char Convertor::scriptGetMapEvents[] = "$holdmapevent = $itmap.events.keys.map{|eid| $itmap.events[eid]};Fux2.sendData($holdmapevent)";
char Convertor::scriptGetEnemyData[] = "$holdEnemy = $data_enemies[%d];Fux2.sendData($holdEnemy)";
char Convertor::scriptGetAllEnemyData[] = "Fux2.sendData($data_enemies)";
char Convertor::scriptGetStartPos[] = "Fux2.sendData([$data_system.start_map_id,$data_system.start_x,$data_system.start_y,])";
const char *Convertor::layer2name[3] = { "bgmap" ,"map" ,"fgmap" };
const char *Convertor::dir2name[4] = { "down" ,"left" ,"right","up" };
unsigned int Convertor::versionCode = 1003;