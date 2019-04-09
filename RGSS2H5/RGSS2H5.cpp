// RGSS2H5.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Convertor.h"

Convertor tool;

int main(int argc, char *argv[])
{
	int error_code = 0;
	if (argc >= 3) {
		cout << "start!" << endl;
		if (tool.initAll(argv, argc)) {
			if (tool.options.onlyConfig) {
				error_code = 4;
			}
			else {
				error_code = tool.convertMapData();
			}
		}
		else {
			error_code = 2;
		}
	}
	else {
		error_code = 1;
	}
	//system("pause");
    return error_code;
}

