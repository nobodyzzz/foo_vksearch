// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <vector>
#include <string>

typedef void (*pfn_add_tracks)(std::vector<std::string> tracks);
typedef void (*pfn_save_lastfm_session)(const char* token);

typedef struct {
	const char* api_id;
	const char* secret;
	const char* viewer_id;
	const char* lastfm_session;
	int radioMaxArtists;
	int radioMaxTraks;
	bool closeAfterAdd;
	pfn_add_tracks addTracksFn;
	pfn_save_lastfm_session saveLastFmSessionFn;
}searchOptions;

void CreateSearchWindow(searchOptions& opts);
