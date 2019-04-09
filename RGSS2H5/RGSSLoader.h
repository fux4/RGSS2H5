#pragma once
#include "RGSS1Runtime.h"
#include <iostream>
using namespace std;

class RGSSLoader {

private:
	HMODULE hRGSSCore = nullptr;

	RR::RGSSInitialize pRGSSInitialize;
	RR::RGSSEval pRGSSEval;

	RR::pfn_rb_define_module rb_define_module;
	RR::pfn_rb_define_module_function rb_define_module_function;

	RV fux2;

	static wchar_t rgssModuleName[];

	static void onRGSSDataArrived(RV, RV);
	static void onRGSSLog(RV, RV);

public:

	bool initAllItems(wchar_t*);
	bool initAllFunctions();
	bool eval(const char*);

	RR::pfn_rb_string_value_ptr rb_string_value_ptr;
	RR::pfn_rgss_get_filefullname rgss_get_filefullname;
	RR::pfn_rb_iv_get rb_iv_get;

	static RV rdata;

};
