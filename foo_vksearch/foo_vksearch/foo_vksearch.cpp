// foo_vksearch.cpp : Defines the exported functions for the DLL application.
//
#include "stdafx.h"
#include "config.h"


DECLARE_COMPONENT_VERSION(NAME, "0.0.2", "search audio tracks using vk.com and last.fm");

class vksearch_mainmenu_command : public mainmenu_commands{
	virtual t_uint32 get_command_count() {
		return 1;
	}
	virtual GUID get_command(t_uint32 p_index) {
		// {3AC2D417-7E10-4c37-B372-46769DFD587A}
		static const GUID vksearch_guid = { 0x3ac2d417, 0x7e10, 0x4c37, { 0xb3, 0x72, 0x46, 0x76, 0x9d, 0xfd, 0x58, 0x7a } };

		if (p_index == 0)
			return vksearch_guid;
		return pfc::guid_null;
	}
	virtual void get_name(t_uint32 p_index, pfc::string_base & p_out) {
		if (p_index == 0)
			p_out = "Add tracks from vk.com";
	}

	virtual bool get_description(t_uint32 p_index, pfc::string_base & p_out) {
		if (p_index == 0)
			p_out = NAME;
		else
			return false;
		return true;
	}
	virtual GUID get_parent()
	{
		return mainmenu_groups::file_add;
	}
	virtual void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) {
		if (p_index == 0 && core_api::assert_main_thread()) {
			searchOptions opts;
			pfc::string8 api_id_value;
			pfc::string8 secret_value;
			pfc::string8 viewer_id_value;
			int artists_max = (int)radio_max_artists.get();
			int tracks_max = (int)radio_max_tracks.get();
			bool close_after_add = close_on_add.get();

			api_id.get(api_id_value);
			secret.get(secret_value);
			viewer_id.get(viewer_id_value);
			opts.api_id = api_id_value.get_ptr();
			opts.secret = secret_value.get_ptr();
			opts.viewer_id = viewer_id_value.get_ptr();
			opts.lastfm_session = lastfm_session;
			opts.radioMaxArtists = artists_max;
			opts.radioMaxTraks = tracks_max;
			opts.closeAfterAdd = close_after_add;
			opts.addTracksFn = &vksearch_mainmenu_command::add_tracks;
			opts.saveLastFmSessionFn = &vksearch_mainmenu_command::save_lastfm_token;
			
			CreateSearchWindow(opts);
		}
	}
	virtual bool get_display(t_uint32 p_index, pfc::string_base & p_text, t_uint32 & p_flags)
	{
		get_name(p_index,p_text);
		return true;
	}
	static void add_tracks(std::vector<std::string> tracks){
		static_api_ptr_t<playlist_manager> plm;
		pfc::list_t<const char*> p_urls;
		for(size_t i = 0; i < tracks.size(); i++){
			p_urls.add_item(tracks[i].c_str());
		}
		plm->activeplaylist_add_locations( p_urls, true, FindWindow(FOOBAR_WINDOW_CLASS, NULL));
	}
	static void save_lastfm_token(const char* token){
		lastfm_session = token;
	}

};

static mainmenu_commands_factory_t< vksearch_mainmenu_command > foo_menu;

