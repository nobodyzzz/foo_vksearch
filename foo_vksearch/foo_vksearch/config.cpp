#include "stdafx.h"
#include "config.h"
static const GUID g_vksearch_options_branch = { 0xa6aa8d69, 0x6537, 0x406d, { 0xb1, 0xcc, 0xb3, 0xf0, 0x58, 0xd1, 0x6f, 0x66 } };
static const GUID g_vksearch_advanced_options_branch = { 0x94f0fddb, 0xe804, 0x455b, { 0xbe, 0xbf, 0xe9, 0x3, 0x3, 0xdd, 0xb5, 0xa } };
static const GUID g_api_id = { 0xf3dd9e3, 0xc210, 0x4cf2, { 0xa1, 0xfd, 0x14, 0xf6, 0xf9, 0xfc, 0x2d, 0x2 } };
static const GUID g_secret = { 0xea3f1f84, 0x8c8, 0x42e3, { 0xb4, 0x95, 0x7d, 0xaa, 0x83, 0xd3, 0x82, 0x5f } };
static const GUID g_viewer_id = { 0xc885f5c3, 0x7491, 0x474f, { 0xaa, 0xf2, 0x44, 0xb4, 0x1c, 0xe4, 0x14, 0xa1 } };
static const GUID g_radio_max_artists = { 0x7d15dcae, 0x89d7, 0x4434, { 0x81, 0x9c, 0x2e, 0xeb, 0xa, 0xe5, 0x61, 0x3d } };
static const GUID g_radio_max_tracks = { 0x4c035500, 0xac5d, 0x4dba, { 0xbb, 0x1b, 0x5c, 0xb3, 0x5, 0xe7, 0xc, 0x40 } };
static const GUID g_close_after_add = { 0x4c035500, 0xac5d, 0x4dba, { 0xbb, 0x1b, 0x5c, 0xb3, 0x5, 0xe7, 0xc, 0x40 } };
static const GUID g_lastfm_session =  { 0x4e28dcc9, 0x7a27, 0x4e72, { 0x89, 0x50, 0xe0, 0x0, 0x86, 0x70, 0x98, 0x91 } };


advconfig_branch_factory vksearch_options_branch("vk.com audio search", g_vksearch_options_branch, advconfig_branch::guid_root, 0);
advconfig_branch_factory vksearch_advanced_branch("Advanced", g_vksearch_advanced_options_branch, g_vksearch_options_branch, 0);


advconfig_string_factory api_id("api_id", g_api_id, g_vksearch_advanced_options_branch, 0, "1866832");
advconfig_string_factory secret("secret", g_secret, g_vksearch_advanced_options_branch, 0, "Zb0o0GO7Ud");
advconfig_string_factory viewer_id("viewer_id", g_viewer_id, g_vksearch_advanced_options_branch, 0, "1704360");

advconfig_integer_factory radio_max_artists("Max artists number in radio mode", 
											g_radio_max_artists, 
											g_vksearch_options_branch,
											0,
											10, 2, 100);
advconfig_integer_factory radio_max_tracks("Max artist's tracks number in radio mode", 
										   g_radio_max_tracks, 
										   g_vksearch_options_branch,
										   0,
										   10, 1, 50);
advconfig_checkbox_factory close_on_add("Close search window after track(s) add",
									   g_close_after_add,
									   g_vksearch_options_branch,
									   0,
									   false);
cfg_string lastfm_session(g_lastfm_session, "");