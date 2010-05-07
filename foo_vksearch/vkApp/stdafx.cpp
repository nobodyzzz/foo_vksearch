#include "stdafx.h"
#include "vkApp.h"


void CreateSearchWindow(searchOptions& opts){
	int argc = 0;
	wxChar** argv = NULL;

	wxApp::SetInstance(new vkApp(opts));
	wxEntryStart(argc, argv);
	wxTheApp->OnInit();
	wxTheApp->OnRun();
	wxTheApp->OnExit();
	wxEntryCleanup();
}