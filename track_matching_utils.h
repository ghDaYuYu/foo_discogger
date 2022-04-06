#pragma once
#include "conf.h"
#include "utils.h"
#include "../../libPPUI/CListControlOwnerData.h"

namespace listview_helper {
	unsigned insert_column(HWND p_listview, unsigned p_index, const char* p_name, unsigned p_width_dlu, int fmt);
}

namespace tileview_helper {
	void tv_insert_column(HWND hlist, int icol,	pfc::string8 heading, int format,	int width, int subitem);
	void tv_insert_item_with_image(HWND hlist, int iItem, pfc::string8 strItem, int iImage, LPARAM param = 0);
	BOOL tv_set_item_tile_lines(HWND hlist, int iItem, UINT* arrColumns, int nLines);
	BOOL tv_tileview_line_count(HWND hlist, int nLines);
	BOOL tv_set_tileview_tile_fixed_width(HWND hlist, int nWidth);
}

pfc::string8 duration_to_str(int seconds);
size_t get_extended_style(HWND list);
std::vector<int> build_woas(HWND list);
std::vector<int> build_woas_libppui(CListControlOwnerData* uilist);


bool isPrimary(pfc::string8 type);
bool fixPrimary(pfc::string8& type);

typedef std::pair<LPARAM, std::vector<pfc::string8>> lvrow;
typedef std::vector<lvrow> vlvrow; //lv row container
typedef std::pair<HWND, vlvrow> hwndrow; //lv list
typedef std::vector<hwndrow> vlvlist; //lv list container

enum class af { alb_emb, alb_sd, alb_sa, alb_ovr, art_emb, art_sd, art_sa, art_ovr };
enum class art_src { unknown, alb, art };

struct lv_art_param {
	size_t ndx;
	art_src src;

	lv_art_param(LPARAM lparam) {
		src = HIWORD(lparam) == (int)art_src::alb ? art_src::alb : art_src::art;
		ndx = LOWORD(lparam);
	}

	lv_art_param(size_t list_position, art_src source)
		:ndx(list_position), src(source) {}

	size_t lv_position() { return ndx; }
	art_src art_source() { return src; }

	bool is_album() { return src == art_src::alb; }

	LPARAM lparam() { return MAKELPARAM(ndx, (int)src); }
};

enum {
	ID_ART_TOOGLE_TRACK_ART_MODES = 1,
	ID_SELECT_ALL,
	ID_INVERT_SELECTION,
	ID_REMOVE,
	ID_CROP,
	ID_SPACE,
	ID_SUBMENU_SELECTOR_ALIGN,
	ID_LEFT_ALIGN,
	ID_CENTER_ALIGN,
	ID_RIGHT_ALIGN,
	ID_ART_WRITE,
	ID_ART_OVR,
	ID_ART_EMBED,
	/*ID_ART_DO_ATTRIB,
	ID_ART_WRITE_ALL,
	ID_ART_OVR_ALL,
	ID_ART_EMBED_ALL,*/
	ID_ART_RESET,
	ID_SUBMENU_SELECTOR_TEMPLATES,
	ID_ART_DETAIL,
	ID_ART_TILE,
	ID_ART_PREVIEW,
	ID_ART_IMAGE_VIEWER,
	ID_ART_TEMPLATE1,
	ID_ART_TEMPLATE2,
	ID_ART_TEMPLATE3,
	ID_ART_TEMPLATE4,
	ID_ART_TEMPLATE5,
};

typedef std::vector<GUID> uartwork_guids;

const struct uartwork {

	t_uint32 ucfg_album_embed;
	t_uint32 ucfg_album_save_to_dir;
	t_uint32 ucfg_album_save_all;
	t_uint32 ucfg_album_ovr;
	t_uint32 ucfg_art_embed;
	t_uint32 ucfg_art_save_to_dir;
	t_uint32 ucfg_art_save_all;
	t_uint32 ucfg_art_ovr;

	bool populated;

	t_uint32 setbitflag(t_uint32& key, size_t pos, bool val) {
		t_uint32 bitmask = 1 << pos;
		if (val) key |= bitmask;
		else key &= ~bitmask;
		
		return key;
	}

	t_uint32 setbitflag_range(t_uint32& key, bool val, size_t from = 0, size_t to = 0) {
		for (size_t i = from; i < to; i++) {
			t_uint32 bitmask = 1 << i;
			if (val) key |= bitmask;
			else key &= ~bitmask;
		}

		return key;
	}

	void setflag(af fl, size_t pos,  bool val) {
		PFC_ASSERT(pos <= 30); //todo: extend 30 bits flag limit
		if (fl == af::alb_emb)			{ setbitflag(ucfg_album_embed, pos, val); }
		else if (fl == af::alb_sd)		{ setbitflag(ucfg_album_save_to_dir, pos, val);
																		if (!val) setflag(af::alb_ovr, pos, false); }
		else if (fl == af::alb_sa)		{ setbitflag(ucfg_album_save_all, pos, val); }
		else if (fl == af::alb_ovr)		{ setbitflag(ucfg_album_ovr, pos, ucfg_album_save_to_dir && val); }
		else if (fl == af::art_emb)		{ setbitflag(ucfg_art_embed, pos, val); }
		else if (fl == af::art_sd)		{ setbitflag(ucfg_art_save_to_dir, pos, val); 
																		if (!val) setflag(af::art_ovr, pos, false); }
		else if (fl == af::art_sa)		{ setbitflag(ucfg_art_save_all, pos, val); }
		else if (fl == af::art_ovr)		{ setbitflag(ucfg_art_ovr, pos, ucfg_art_save_to_dir && val); }
	}

	void setflag_range(af fl, bool val, size_t from, size_t to) {
		PFC_ASSERT(to <= 30);
		if (fl == af::alb_emb)			{ setbitflag_range(ucfg_album_embed, val, from, to); }
		else if (fl == af::alb_sd)		{ setbitflag_range(ucfg_album_save_to_dir, val, from, to); }
		else if (fl == af::alb_sa)		{ setbitflag_range(ucfg_album_save_all, val, from, to); }
		else if (fl == af::alb_ovr)		{ setbitflag_range(ucfg_album_ovr, val, from, to); }
		else if (fl == af::art_emb)		{ setbitflag_range(ucfg_art_embed, val, from, to); }
		else if (fl == af::art_sd)		{ setbitflag_range(ucfg_art_save_to_dir, val, from, to); }
		else if (fl == af::art_sa)		{ setbitflag_range(ucfg_art_save_all, val, from, to); }
		else if (fl == af::art_ovr)		{ setbitflag_range(ucfg_art_ovr, val, from, to); }
	}

	bool getbitflag(t_uint32 key, size_t pos) {
		t_uint32 bitmask = 1 << pos;
		return (key & bitmask) == bitmask;
	}

	bool getflag(af fl, size_t pos) {
		PFC_ASSERT(pos <= 30); //use 30 bits limit
		bool bres = false;
		if (fl == af::alb_emb) { bres = getbitflag(ucfg_album_embed, pos); }
		else if (fl == af::alb_sd) { bres = getbitflag(ucfg_album_save_to_dir, pos); }
		else if (fl == af::alb_sa) { bres = getbitflag(ucfg_album_save_all, pos); }
		else if (fl == af::alb_ovr) { bres = getbitflag(ucfg_album_ovr, pos); }
		else if (fl == af::art_emb) { bres = getbitflag(ucfg_art_embed, pos); }
		else if (fl == af::art_sd) { bres = getbitflag(ucfg_art_save_to_dir, pos); }
		else if (fl == af::art_sa) { bres = getbitflag(ucfg_art_save_all, pos); }
		else if (fl == af::art_ovr) { bres = getbitflag(ucfg_art_ovr, pos); }
		return bres;
	}

	uartwork(
		bool cfg_album_embed,
		bool cfg_album_save_to_dir,
		bool cfg_album_save_all,
		bool cfg_album_ovr,
		bool cfg_art_embed,
		bool cfg_art_save_to_dir,
		bool cfg_art_save_all,
		bool cfg_art_ovr
	) {

		populated = false;
		ucfg_album_embed = 0;
		ucfg_album_save_to_dir = 0;
		ucfg_album_save_all = 0;
		ucfg_album_ovr = 0;
		ucfg_art_embed = 0;
		ucfg_art_save_to_dir = 0;
		ucfg_art_save_all = 0;
		ucfg_art_ovr = 0;

		ucfg_album_embed = (cfg_album_embed ? setbitflag_range(ucfg_album_embed, true, 0, 31) : 0); //per track
		ucfg_album_save_to_dir = (cfg_album_save_to_dir ? setbitflag_range(ucfg_album_save_to_dir, true, 0, 1) : 0);
		ucfg_album_save_all = ((cfg_album_save_to_dir || cfg_album_embed) && cfg_album_save_all ? setbitflag_range(ucfg_album_save_all, true, 0, 31) : 0); //todo
		ucfg_album_ovr = ((cfg_album_save_to_dir || cfg_album_embed) && cfg_album_ovr ? setbitflag_range(ucfg_album_ovr, true, 0, 31) : 0);
		ucfg_art_embed = (cfg_art_embed ? setbitflag_range(ucfg_art_embed, true, 0, 31) : 0);
		ucfg_art_save_to_dir = (cfg_art_save_to_dir ? setbitflag_range(ucfg_art_save_to_dir, true, 0, 1) : 0);
		ucfg_art_save_all = ((cfg_art_save_to_dir || cfg_art_embed) && cfg_art_save_all ? setbitflag_range(ucfg_art_save_all, true, 0, 31) : 0); //todo
		ucfg_art_ovr = ((cfg_art_save_to_dir || cfg_art_embed) &&  cfg_art_ovr ? setbitflag_range(ucfg_art_ovr, true, 0, 31) : 0);
	}

	uartwork() {
		ucfg_album_embed = 0;
		ucfg_album_save_to_dir = 0;
		ucfg_album_save_all = 0;
		ucfg_album_ovr = 0;
		ucfg_art_embed = 0;
		ucfg_art_save_to_dir = 0;
		ucfg_art_save_all = 0;
		ucfg_art_ovr = 0;
	}

	inline bool operator==(const uartwork& rhs) {
		return (ucfg_album_embed == rhs.ucfg_album_embed &&
			ucfg_album_save_to_dir == rhs.ucfg_album_save_to_dir &&
			ucfg_album_save_all == rhs.ucfg_album_save_all &&
			ucfg_album_ovr == rhs.ucfg_album_ovr &&
			ucfg_art_embed == rhs.ucfg_art_embed &&
			ucfg_art_save_to_dir == rhs.ucfg_art_save_to_dir &&
			ucfg_art_save_all == rhs.ucfg_art_save_all &&
			ucfg_art_ovr == rhs.ucfg_art_ovr);
	}

	uartwork(const CConf & conf) {

		*this = uartwork(
			conf.embed_album_art,
			conf.save_album_art,
			conf.album_art_fetch_all,
			conf.album_art_overwrite,
			conf.embed_artist_art,
			conf.save_artist_art,
			conf.artist_art_fetch_all,
			conf.artist_art_overwrite
		);
	}

	bool isEmpty() {
		return (ucfg_album_embed == 0 &&
			ucfg_album_save_to_dir == 0 &&
			ucfg_album_save_all == 0 &&
			ucfg_album_ovr == 0 &&
			ucfg_art_embed == 0 &&
			ucfg_art_save_to_dir == 0 &&
			ucfg_art_save_all == 0 &&
			ucfg_art_ovr == 0);
	}
};

extern uartwork CONFARTWORK;
extern uartwork_guids CONFARTGUIDS;

const pfc::string8 THUMB_EXTENSION = ".png";

const size_t FLAG_FIRST_COL = 3; 

std::pair<HBITMAP, HBITMAP> MemoryBlockToTmpBitmap(std::pair<pfc::string8, pfc::string8> cache_path, size_t pos, MemoryBlock small_art);
std::pair<HBITMAP, HBITMAP> GenerateTmpBitmapsFromRealSize(pfc::string8 release_id, size_t pos, pfc::string8 source_full_path, std::pair<pfc::string8, pfc::string8> &temp_file_names);
std::pair<pfc::string8, pfc::string8> ReadDimSizeFromFile(pfc::string8 path, pfc::string8 file_name);
MemoryBlock MemoryBlockToPngIcon(MemoryBlock buffer);
