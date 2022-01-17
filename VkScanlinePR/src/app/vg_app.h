#pragma once
#ifndef GALAXYSAILING_VG_APP_H_
#define GALAXYSAILING_VG_APP_H_s
#include <memory>

class VGApplication {
public:
	virtual void run() = 0;
	virtual VGApplication* viewport(int x, int y, int w, int h) = 0;
	virtual VGApplication* appName(const char* name) = 0;
	
	virtual VGApplication* loadPathFile(const char* filename) = 0;
};

std::shared_ptr<VGApplication> getAppInstance();

#endif