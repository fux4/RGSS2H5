#include "stdafx.h"
#include "RGSSLoader.h"

void RGSSLoader::onRGSSDataArrived(RV obj, RV data) {
	rdata = data;
}

void RGSSLoader::onRGSSLog(RV obj, RV info) {
	cout << (char*)(RR::RSTRING(info)->as.heap.ptr) << endl;
}

bool RGSSLoader::initAllItems(wchar_t *workDir) {
	hRGSSCore = LoadLibrary(rgssModuleName);
	if (!hRGSSCore) return false;
#define __get_check(fn) p##fn = (RGSS1Runtime::fn)::GetProcAddress(hRGSSCore, #fn);
	__get_check(RGSSInitialize);
	__get_check(RGSSEval);
#undef __get_check
#define __get_addr(fc) fc = (RGSS1Runtime::pfn_##fc)((DWORD)RGSS1Runtime::addr_##fc + (DWORD)hRGSSCore);
	__get_addr(rb_define_module)
	__get_addr(rb_define_module_function)
	__get_addr(rb_string_value_ptr)
	__get_addr(rb_iv_get)
	__get_addr(rgss_get_filefullname)
#undef __get_addr
	SetCurrentDirectory(workDir);
	pRGSSInitialize(hRGSSCore);
	eval("$data_actors = load_data(\"Data/Actors.rxdata\");			\
		$data_classes = load_data(\"Data/Classes.rxdata\");				\
		$data_skills = load_data(\"Data/Skills.rxdata\");				\
		$data_items = load_data(\"Data/Items.rxdata\");					\
		$data_weapons = load_data(\"Data/Weapons.rxdata\");				\
		$data_armors = load_data(\"Data/Armors.rxdata\");				\
		$data_enemies = load_data(\"Data/Enemies.rxdata\");				\
		$data_troops = load_data(\"Data/Troops.rxdata\");				\
		$data_states = load_data(\"Data/States.rxdata\");				\
		$data_animations = load_data(\"Data/Animations.rxdata\");		\
		$data_tilesets = load_data(\"Data/Tilesets.rxdata\");			\
		$data_common_events = load_data(\"Data/CommonEvents.rxdata\");	\
		$data_system = load_data(\"Data/System.rxdata\");				\
		$data_mapinfo = load_data(\"Data/MapInfos.rxdata\")");
	return true;
}

bool RGSSLoader::initAllFunctions() {
	fux2 = rb_define_module("Fux2");
	rb_define_module_function(fux2, "sendData", (RF)onRGSSDataArrived, 1);
	//rb_define_module_function(fux2, "log", (RF)onRGSSLog, 1);
	return true;
}

bool RGSSLoader::eval(const char* script) {
	return pRGSSEval(script) == RR::Qfalse;
}

RV RGSSLoader::rdata = 4;
wchar_t RGSSLoader::rgssModuleName[] = L"rgss.dll";