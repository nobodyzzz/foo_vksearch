#include "StdAfx.h"
#include "vkSearchWindow.h"

vkSearchWindow::vkSearchWindow(const wxString& title, searchOptions options) : wxFrame(NULL, wxID_ANY, title){	
	m_tracks = NULL;
	m_popupMenu = NULL;
	m_apiId = wxString::FromAscii(options.api_id);
	m_secret = wxString::FromAscii(options.secret);
	m_viewerId = wxString::FromAscii(options.viewer_id);
	m_radioMaxArtists = options.radioMaxArtists;
	m_radioMaxTracks = options.radioMaxTraks;
	m_closeAfterAdd = options.closeAfterAdd;
	m_addTracksFn = options.addTracksFn;
	m_searchThread = 0;
	m_noLog = new wxLogNull();
	m_vbox = new wxBoxSizer(wxVERTICAL);
	m_hbox1 = new wxBoxSizer(wxHORIZONTAL);
	m_hbox2 = new wxBoxSizer(wxHORIZONTAL);
	m_searchPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxCAPTION | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxTHICK_FRAME, wxT("search panel"));
	m_queryTextBox = new wxTextCtrl(m_searchPanel, QUERY_EDIT);
	m_searchButton = new wxButton(m_searchPanel, SEARCH_BUTTON, wxT("search"));
	m_searchResult = new wxListCtrl(m_searchPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);	
	m_addAll = new wxButton(m_searchPanel, ADDALL_BUTTON, wxT("Add all"));
	m_addSelected = new wxButton(m_searchPanel, ADDSELECTED_BUTTON, wxT("Add selected"));
	m_deleteSelected = new wxButton(m_searchPanel, DELETESELECTED_BUTTON, wxT("Delete selected"));
	m_keepPrevious = new wxCheckBox(m_searchPanel, wxID_ANY, wxT("Keep previous search result"));

	m_searchResult->InsertColumn(0, wxT("Artist"));
	m_searchResult->InsertColumn(1, wxT("Title"));
	m_searchResult->InsertColumn(2, wxT("Duration"), wxLIST_FORMAT_RIGHT);


	m_hbox1->Add(m_queryTextBox, 1, wxRIGHT, 8);
	m_hbox1->Add(m_searchButton, 0);
	m_hbox2->Add(m_addAll, 1, wxRIGHT);
	m_hbox2->Add(m_addSelected, 1, wxRIGHT);
	m_hbox2->Add(m_deleteSelected, 1, wxRIGHT);	
	m_vbox->Add(m_hbox1, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);
	m_vbox->Add(m_keepPrevious, 0, wxLEFT, 10);
	m_vbox->Add(-1, 10);
	m_vbox->Add(m_searchResult, 5, wxEXPAND | wxALL);
	m_vbox->Add(m_hbox2, 0, wxEXPAND, 1);
	m_searchPanel->SetSizer(m_vbox);

	int width = GetSize().GetWidth();
	int point = width / 8;

	m_searchResult->SetColumnWidth(0, point*2);
	m_searchResult->SetColumnWidth(1, point*4 - 15);
	m_searchResult->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);
	SetSize(384, 472);

	CreateStatusBar();
	SetStatusText(wxT("vk.com audio search"));

	Connect(SEARCH_BUTTON, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(vkSearchWindow::OnSearchButtonClick));
	Connect(ADDALL_BUTTON, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(vkSearchWindow::OnAddAllClick));
	Connect(ADDSELECTED_BUTTON, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(vkSearchWindow::OnAddSelectedClick));
	Connect(DELETESELECTED_BUTTON, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(vkSearchWindow::OnDeleteSelectedClick));
	Connect(wxEVT_CONTEXT_MENU, wxContextMenuEventHandler(vkSearchWindow::OnContextMenu));
	m_queryTextBox->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(vkSearchWindow::OnKeyDown));
	m_searchResult->Connect(wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler(vkSearchWindow::OnSearchItemActivate));
	
	wxSocketBase::Initialize();
}

vkSearchWindow::~vkSearchWindow(void)
{
	if(m_tracks){
		CleanUpSearchResult();
	}
	if(m_searchThread){
		TerminateThread(m_searchThread, 0);
	}
	wxSocketBase::Initialize();
	delete m_noLog;
}

wxString* vkSearchWindow::StringHash( char* string )
{
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	BYTE rgbHash[MD5LEN] = {0};
	DWORD cbHash = MD5LEN;
	wxString* result = NULL;

	if(string){
		if(CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
			if(CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)){
				if(CryptHashData(hHash, (const BYTE*)string, strlen(string), 0))
				{
					if(CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)){
						result = new wxString();
						result->Printf(	wxT("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"), 
							rgbHash[ 0], rgbHash[1], rgbHash[ 2], rgbHash[ 3], rgbHash[ 4], rgbHash[ 5], rgbHash[ 6], 
							rgbHash[ 7], rgbHash[8], rgbHash[ 9], rgbHash[10], rgbHash[11], rgbHash[12], rgbHash[13], 
							rgbHash[14], rgbHash[15]);

					}
				}
				CryptDestroyHash(hHash);
			}
			CryptReleaseContext(hProv, 0);
		}
	}
	return result;
}
char* vkSearchWindow::BuildQueryString( wxString query )
{
	char* result = NULL;
	if(query.Len()){
		wxString* hash;
		char signature[INTERNET_MAX_URL_LENGTH + 1] = {0};
		
		sprintf_s(	signature, INTERNET_MAX_URL_LENGTH, "%sapi_id=%scount=200method=audio.searchq=%stest_mode=1%s",
					m_viewerId.ToAscii(), m_apiId.ToAscii(), query.ToUTF8().data(), m_secret.ToAscii());

		hash = StringHash(signature);
		if(hash){
			result = new char[INTERNET_MAX_URL_LENGTH + 1];
			
			sprintf_s(	result, 
						INTERNET_MAX_URL_LENGTH, 
						"http://api.vkontakte.ru/api.php?api_id=%s&count=200&method=audio.search&sig=%s&test_mode=1&q=%s",
						m_apiId.ToAscii(), hash->ToAscii(), query.ToUTF8().data());
		}
	}
	return result;
}
bool vkSearchWindow::BuildTrackList( char *queryString )
{
	bool success = false;

	if(queryString){
		wxURL url(wxString::FromAscii(queryString));

		if(url.IsOk()){
			wxInputStream* result = url.GetInputStream();

			if(result){
				try{
					wxXmlDocument xml(*result);

					if(xml.GetRoot()->GetName() != wxT("error")){
						wxXmlNode *node = xml.GetRoot()->GetChildren();

						while(node){
							if(node->GetName() == wxT("audio")){
								wxXmlNode *track = node->GetChildren();
								wxString artist; 
								wxString title; 
								wxString url; 
								long duration = 0;

								while(track){
									wxString name = track->GetName();
									if(name == wxT("artist")){
										artist = track->GetChildren()->GetContent();
									} else if(name == wxT("title")){
										title = track->GetChildren()->GetContent();
									} else if(name == wxT("url")){
										url = track->GetChildren()->GetContent();
									} else if(name == wxT("duration")){
										track->GetChildren()->GetContent().ToLong(&duration);
									}
									track = track->GetNext();
								}
								Audio* audio = new Audio();
								audio->artist = artist; audio->title = title; audio->url = url;  audio->duration = (int)duration;  m_tracks->push_back(audio);
							}
							node = node->GetNext();
						}
						success = true;
					} else {
						SetStatusText(wxT("vk.com request error: ") + xml.GetRoot()->GetChildren()->GetContent());
						success = false;
					}
				}catch(...){}
				wxDELETE(result);

			}
		}
	}

	return success;
}	

DWORD __stdcall vkSearchWindow::SearchThread(){
	wxString query = m_queryTextBox->GetValue();
	char *queryString = NULL;
	bool success = false;

	m_searchButton->Disable();
	SetStatusText(wxT("working..."));
	if(m_tracks && !m_keepPrevious->IsChecked()){
		CleanUpSearchResult();

	}
	if(!m_tracks){
		m_tracks = new std::list<Audio*>();
	}
	m_vkRequestError = false;
	if(query.StartsWith(wxT("tag:"))){
		query.Replace(wxT("tag:"), wxEmptyString);
		success = LastFmTagSearch(query);
	} else if(query.StartsWith(wxT("vk:"))){
		query.Replace(wxT("vk:"), wxEmptyString);
		queryString = BuildQueryString(query);
		if(queryString){
			if(BuildTrackList(queryString)){
				m_tracks->sort(compare_tracks());
				m_tracks->unique(equal_tracks());
				std::list<Audio*>::iterator track = m_tracks->begin();

				while(track != m_tracks->end()){
					AddTrackToResultList(*track);
					track++;
				}
			}
		}
	} else if(query.StartsWith(wxT("radio:"))){
		query.Replace(wxT("radio:"), wxEmptyString);
		success = LastFmArtistRadio(query);
	} else {
		success = LastFmArtistSearch(query);
	}
	if(!m_vkRequestError && success){
		SetStatusText(wxT("done"));
	}
	m_searchButton->Enable();
	m_searchThread = 0;
	return TRUE;
}
void vkSearchWindow::DoSearch()
{
	union {
		DWORD (__stdcall vkSearchWindow::*method)();
		DWORD (__stdcall *fn)(void*);
	} ptr = { &vkSearchWindow::SearchThread};

	m_searchThread = CreateThread(NULL, 0, ptr.fn, this, 0, NULL);
}
Audio* vkSearchWindow::VkSearchTrack(wxString artistName, wxString trackName){
	wxString query;
	char* queryString = NULL;
	Audio* audio = NULL;

	if(artistName.Len() && trackName.Len()){
		query.Printf(wxT("%s - %s"), artistName, trackName);
		queryString = BuildQueryString(query);
		if(queryString){
			wxURL url(wxString::FromAscii(queryString));

			if(url.IsOk()){
				wxInputStream* result = url.GetInputStream();

				if(result){
					try{
						wxXmlDocument xml;
						
						if( xml.Load(*result) && xml.GetRoot() && xml.GetRoot()->GetName() != wxT("error")){
							wxXmlNode *node = xml.GetRoot()->GetChildren();

							while(node){
								if(node->GetName() == wxT("audio")){
									wxXmlNode *track = node->GetChildren();
									wxString artist;
									wxString title;
									wxString url;
									long duration = 0;

									while(track){
										wxString name = track->GetName();
										if(track->GetChildren()){
											if(name == wxT("artist")){
												artist = track->GetChildren()->GetContent();
											} else if(name == wxT("title")){
												title = track->GetChildren()->GetContent();
											} else if(name == wxT("url")){
												url = track->GetChildren()->GetContent();
											} else if(name == wxT("duration")){
												track->GetChildren()->GetContent().ToLong(&duration);
											}
										}
										track = track->GetNext();
									}
									if(artist.IsSameAs(artistName, false) && title.IsSameAs(trackName, false)){
										audio = new Audio();

										audio->artist = artist;
										audio->title = title;
										audio->url = url;
										audio->duration = (int)duration;
									}
								}
								node = node->GetNext();
							}
						} else {
							if(xml.GetRoot()){
								SetStatusText(wxT("vk.com request error: ") + xml.GetRoot()->GetChildren()->GetContent());
							} else {
								SetStatusText(wxT("vk.com request error"));
							}
							m_vkRequestError = true;
						}
					} catch(...){}

					wxDELETE(result);
				}
			}
		}
	}
	return audio;
}
void vkSearchWindow::AddTrackToResultList(Audio *track){
	wxString duration;
	long index = 0;
	wxMBConvUTF8 conv;
	wxListItem newTrack;

	newTrack.SetText(wxString(track->artist.c_str()));
	newTrack.SetId(m_searchResult->GetItemCount());
	newTrack.SetData((void*)track->url.c_str());
	duration.Printf(wxT("%2d:%02d"), track->duration / 60, track->duration % 60);						
	index = m_searchResult->InsertItem(newTrack);
	m_searchResult->SetItem(index, 1, wxString(track->title.c_str()));
	m_searchResult->SetItem(index, 2, duration);

}
bool vkSearchWindow::LastFmArtistSearch( wxString artist, int maxCount /*= 50*/ )
{
	wxString requestUrl;
	wxURI requestUri;
	std::auto_ptr<TCHAR> escapedArtist(new TCHAR[INTERNET_MAX_URL_LENGTH + 1]);
	DWORD chEscaped = 0;
	bool success = false;

	AtlEscapeUrl(artist.c_str(), escapedArtist.get(), &chEscaped, INTERNET_MAX_URL_LENGTH);
	requestUrl.Printf(wxT("http://ws.audioscrobbler.com/1.0/artist/%s/toptracks.xml"),  escapedArtist);
	requestUri.Create(requestUrl);
	wxURL url(requestUri);
	if(url.IsOk()){
		wxInputStream *toptracks = url.GetInputStream();
		int trackCount = 0;

		if(toptracks){
			wxXmlDocument xml(*toptracks);
			wxXmlNode *node = xml.GetRoot()->GetChildren();

			while(node && !m_vkRequestError){
				if(node->GetName() == wxT("track")){
					wxXmlNode *track = node->GetChildren();

					if(track->GetName() == wxT("name")){
						wxString name = track->GetChildren()->GetContent();
						Audio* track = VkSearchTrack(artist, name);

						if(track && std::find_if(m_tracks->begin(), m_tracks->end(), std::bind2nd(equal_tracks(), track)) == m_tracks->end()){
							m_tracks->push_back(track);
							AddTrackToResultList(track);
							trackCount++;
							if(trackCount == maxCount){
								break;
							}
						}
					}
				}
				wxMilliSleep(333);
				node = node->GetNext();
			}
			success = !m_vkRequestError;
			wxDELETE(toptracks);
		} else {
			SetStatusText(wxT("last.fm request failed. try vk:") + artist);
		}
	}
	return success;
}
bool vkSearchWindow::LastFmTagSearch( wxString tag )
{
	wxString requestUrl;
	wxURI requestUri;
	std::auto_ptr<TCHAR> escapedTag(new TCHAR[INTERNET_MAX_URL_LENGTH + 1]);
	DWORD chEscaped = 0;
	bool success = false;

	AtlEscapeUrl(tag.c_str(), escapedTag.get(), &chEscaped, INTERNET_MAX_URL_LENGTH);
	requestUrl.Printf(wxT("http://ws.audioscrobbler.com/1.0/tag/%s/toptracks.xml"), escapedTag);
	requestUri.Create(requestUrl);
	wxURL url(requestUri);
	if(url.IsOk()){
		wxInputStream *toptracks = url.GetInputStream();
		if(toptracks){
			wxXmlDocument xml(*toptracks);
			wxXmlNode *track = xml.GetRoot()->GetChildren();

			while(track && !m_vkRequestError){
				if(track->GetName() == wxT("track")){
					wxXmlNode *artist = track->GetChildren();
					wxString trackName = track->GetAttribute(wxT("name"), wxEmptyString);
					wxString artistName = artist->GetAttribute(wxT("name"), wxEmptyString);
					Audio* audio = VkSearchTrack(artistName, trackName);

					if(audio){						
						m_tracks->push_back(audio);
						AddTrackToResultList(audio);
					}

				}
				wxMilliSleep(333);
				track = track->GetNext();
			}
			wxDELETE(toptracks);
			success = !m_vkRequestError;
		} else {
			SetStatusText(wxT("last.fm request failed..."));
		}
	}
	return success;
}
bool vkSearchWindow::LastFmArtistRadio( wxString artist )
{
	wxString requestUrl;
	wxURI requestUri;
	std::auto_ptr<TCHAR> escapedArtist(new TCHAR[INTERNET_MAX_URL_LENGTH + 1]);
	DWORD chEscaped = 0;
	bool success = false;

	LastFmArtistSearch(artist, m_radioMaxTracks);
	AtlEscapeUrl(artist.c_str(), escapedArtist.get(), &chEscaped, INTERNET_MAX_URL_LENGTH);
	requestUrl.Printf(wxT("http://ws.audioscrobbler.com/1.0/artist/%s/similar.xml"),  escapedArtist);
	requestUri.Create(requestUrl);
	wxURL url(requestUri);
	if(url.IsOk()){
		wxInputStream *similar = url.GetInputStream();
		int count = 0;

		if(similar){
			wxXmlDocument xml(*similar);
			wxXmlNode *node = xml.GetRoot()->GetChildren();

			while(node && !m_vkRequestError){
				if(node->GetName() == wxT("artist")){
					wxXmlNode *similarArtist = node->GetChildren();

					if(similarArtist->GetName() == wxT("name")){
						wxString name = similarArtist->GetChildren()->GetContent();

						LastFmArtistSearch(name, m_radioMaxTracks);
						count++;
						if(count == m_radioMaxArtists){
							break;
						}
					}
				}
				node = node->GetNext();
			}
			wxDELETE(similar);
			success = !m_vkRequestError;
		} else {
			SetStatusText(wxT("last.fm request failed..."));
		}
	}
	return success;
}
void vkSearchWindow::DeleteSelectedTracks(){
	long item = -1;

	while((item = m_searchResult->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != wxNOT_FOUND)
		m_searchResult->DeleteItem(item--);
}
void vkSearchWindow::AddSelected(){
	if(m_addTracksFn){
		long item = -1;
		std::vector<std::string> tracks;

		while((item = m_searchResult->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != wxNOT_FOUND){
			wxString url((wxChar*)m_searchResult->GetItemData(item));
			tracks.push_back(std::string(url.ToAscii()));
		}
		m_addTracksFn(tracks);
		if(m_closeAfterAdd)
		{
			Close();
		}
	}
}
void vkSearchWindow::CopyUrlsToClipboard(){
	long itemIdx = -1;
	wxString urls;
	wxTextDataObject* url = new wxTextDataObject();

	while((itemIdx = m_searchResult->GetNextItem(itemIdx, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != wxNOT_FOUND){
		urls.Append((wxChar*)m_searchResult->GetItemData(itemIdx));
		urls.Append(wxT("\n"));
	}
	urls = urls.Mid(0, urls.Len() - 1);
	if(wxTheClipboard->Open()){
		url->SetText(urls);
		wxTheClipboard->SetData(url);
		wxTheClipboard->Close();
	}
}
void vkSearchWindow::OnSearchButtonClick(wxCommandEvent& evt){
	DoSearch();
}
void vkSearchWindow::OnAddAllClick(wxCommandEvent& evt){
	if(m_addTracksFn){
		std::vector<std::string> tracks;
		for(long i = 0; i < m_searchResult->GetItemCount(); i++){
			wxString url((wxChar*)m_searchResult->GetItemData(i));
			tracks.push_back(std::string(url.ToAscii()));
		}
		m_addTracksFn(tracks);
		if(m_closeAfterAdd)
		{
			Close();
		}
	}
}
void vkSearchWindow::OnAddSelectedClick(wxCommandEvent& evt){
	AddSelected();
}	
void vkSearchWindow::OnDeleteSelectedClick(wxCommandEvent& evt){
	DeleteSelectedTracks();
}
void vkSearchWindow::OnPopupClick(wxCommandEvent& evt){
	switch(evt.GetId()){
			case COPY_URL_MENU:
				CopyUrlsToClipboard();
				break;
			case ADDSELECTED_MENU:
				AddSelected();
				break;
			case DELETESELECTED_MENU:
				DeleteSelectedTracks();
				break;
	}
}
void vkSearchWindow::OnContextMenu(wxContextMenuEvent& evt){		
	if(!m_popupMenu){
		m_popupMenu = new wxMenu();

		m_popupMenu->Append(ADDSELECTED_MENU, wxT("Add selected tracks to playlist"));
		m_popupMenu->Append(DELETESELECTED_MENU, wxT("Delete selected tracks"));
		m_popupMenu->Append(COPY_URL_MENU, wxT("Copy url(s) to clipboard"));
		Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(vkSearchWindow::OnPopupClick));
	}
	if(m_searchResult->IsMouseInWindow() && m_searchResult->GetItemCount() != 0){
		PopupMenu(m_popupMenu);
	}
}
void vkSearchWindow::OnKeyDown(wxKeyEvent& event){
	if(event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_NUMPAD_ENTER){
		vkSearchWindow* _this = (vkSearchWindow*)GetParent()->GetParent();
		_this->DoSearch();			
	} else {
		event.Skip();
	}
}

void vkSearchWindow::CleanUpSearchResult()
{
	m_searchResult->DeleteAllItems();
	std::for_each(m_tracks->begin(), m_tracks->end(), delete_object());
	delete m_tracks;				
	m_tracks = NULL;
}
void vkSearchWindow::OnSearchItemActivate( wxListEvent& evt )
{
	vkSearchWindow* _this = (vkSearchWindow*)GetParent()->GetParent();
	_this->AddSelected();
}