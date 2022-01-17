#include "app/vg_app.h"
#include <memory>
#include <iostream>
#include "windows.h"
std::shared_ptr<VGApplication> app;

int main(void) {
	app = getAppInstance();
	try {
		app->appName("hello scanline vector graphic")
			->viewport(0, 0, 1200, 1024)
			->loadPathFile("./input/rvg/test.rvg")
			->run();
	}catch (std::exception& e) {
		printf("\n");
		std::cerr << e.what() << "\n";
	}
	catch (...) {
		printf("\n");
		std::cerr << "Unknown Exception\n";
	}
	return 0;
}