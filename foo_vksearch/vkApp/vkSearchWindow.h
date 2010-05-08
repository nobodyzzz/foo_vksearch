#pragma once

#include <stdio.h>
#include <windows.h>
#include <Commctrl.h>
#include <Wincrypt.h>
#include <Wininet.h>
#include <atlutil.h>
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include <functional>
#include <string>
#include "wx/wxprec.h"
#include "wx/textctrl.h"
#include "wx/listctrl.h"
#include "wx/url.h"
#include "wx/XML/xml.h"
#include "wx/clipbrd.h"
#include "wx/file.h"
#include "wx/filename.h"
#include "wx/stdpaths.h"
#include "wx/log.h"

#define MD5LEN 16

enum {
	QUERY_EDIT = 0x122,
	SEARCH_BUTTON,
	ADDALL_BUTTON,
	ADDSELECTED_BUTTON,
	DELETESELECTED_BUTTON,
	COPY_URL_MENU,
	ADDSELECTED_MENU,
	DELETESELECTED_MENU
};

typedef struct tagAudio
{
	wxString artist;
	wxString title;
	wxString url;
	int duration;

}Audio;

enum States
{
	Idle,
	ReadingArtist,
	ReadingTitle,
	ReadingUrl,
	ReadingDuration,
};

struct delete_object
{
	template <typename T>
	void operator()(T *ptr){ delete ptr;}
};

struct equal_tracks : std::binary_function<Audio*, Audio*, bool>{
	bool operator()(const Audio* op1, const Audio* op2) const{
		wxString tmp1; 
		wxString tmp2;
		wxString debug;

		tmp1.Printf(wxT("%s%s%d"), op1->artist, op1->title, op1->duration);
		tmp2.Printf(wxT("%s%s%d"), op2->artist, op2->title, op2->duration);
		return tmp1.IsSameAs(tmp2, false);
	}
};

struct compare_tracks{
	bool operator()(Audio* op1, Audio* op2){
		wxString tmp1; 
		wxString tmp2;

		tmp1.Printf(wxT("%s%s%d"), op1->artist, op1->title, op1->duration);
		tmp2.Printf(wxT("%s%s%d"), op2->artist, op2->title, op2->duration);
		return tmp1.CmpNoCase(tmp2) < 0;
	}
};

typedef void (*pfn_add_tracks)(std::vector<std::string> tracks);

/*typedef struct {
	char* api_id;
	char* secret;
	char* viewer_id;
	int radioMaxArtists;
	int radioMaxTraks;
	bool closeAfterAdd;
	pfn_add_tracks addTracksFn;
}searchOptions;*/


class vkSearchWindow : 	public wxFrame
{
private:
	wxTextCtrl* m_queryTextBox;
	wxButton* m_searchButton;
	wxButton* m_addAll;
	wxButton* m_addSelected;
	wxButton* m_deleteSelected;
	wxBoxSizer* m_vbox;
	wxBoxSizer* m_hbox1;
	wxBoxSizer* m_hbox2;
	wxPanel* m_searchPanel;
	wxMenu*	m_popupMenu;
	wxCheckBox* m_keepPrevious;
	wxListCtrl *m_searchResult;
	std::list<Audio*>* m_tracks;
	wxLogNull* m_noLog;
	wxString m_apiId;
	wxString m_secret;
	wxString m_viewerId;
	int m_radioMaxArtists;
	int m_radioMaxTracks;
	pfn_add_tracks m_addTracksFn;
	HANDLE m_searchThread;
	bool m_closeAfterAdd;
	bool m_vkRequestError;

	wxString* StringHash(char* string);
	char* BuildQueryString(wxString query);
	bool BuildTrackList(char *queryString);
	DWORD __stdcall SearchThread();
	void CleanUpSearchResult();
	void DoSearch();
	Audio* VkSearchTrack(wxString artistName, wxString trackName);
	void AddTrackToResultList(Audio *track);
	bool LastFmArtistSearch(wxString artist, int maxCount = 50);
	bool LastFmTagSearch(wxString tag);
	bool LastFmArtistRadio(wxString artist);
	void DeleteSelectedTracks();
	void CopyUrlsToClipboard();
	void AddSelected();
	wxString Unescape(wxString string);
	void OnSearchButtonClick(wxCommandEvent& evt);
	void OnAddAllClick(wxCommandEvent& evt);
	void OnAddSelectedClick(wxCommandEvent& evt);
	void OnDeleteSelectedClick(wxCommandEvent& evt);
	void OnPopupClick(wxCommandEvent& evt);
	void OnContextMenu(wxContextMenuEvent& evt);
	void OnKeyDown(wxKeyEvent& event);
	void OnSearchItemActivate(wxListEvent& evt);
public:
	vkSearchWindow(const wxString& title, searchOptions options);
	~vkSearchWindow(void);
};
