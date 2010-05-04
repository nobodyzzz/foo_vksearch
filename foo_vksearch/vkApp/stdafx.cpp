#include "stdafx.h"
#include "vkApp.h"


void CreateSearchWindow(searchOptions& opts){
	int argc = 0;
	wxApp::SetInstance(new vkApp(opts));
	wxEntryStart(argc, NULL);
	wxTheApp->OnInit();
	wxTheApp->OnRun();
	wxTheApp->OnExit();
	wxEntryCleanup();
}