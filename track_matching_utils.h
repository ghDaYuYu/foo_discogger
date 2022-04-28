#pragma once
#include "conf.h"
#include "utils.h"
#include "libPPUI/CListControlOwnerData.h"

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

const size_t kBlockSize = 31;		//0..30
const size_t kMax_Artwork = 200;	//alb + art -> 400

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
	ID_PREVIEW_CMD_BACK = 1,
	ID_PREVIEW_CMD_PREVIEW,
	ID_PREVIEW_CMD_WRITE_TAGS,
	ID_PREVIEW_CMD_WRITE_ARTWORK,
	ID_ART_TOOGLE_TRACK_ART_MODES,
	ID_SELECT_ALL,
	ID_INVERT_SELECTION,
	ID_REMOVE,
	ID_CROP,
	ID_MOVE,
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

	t_uint32 setbitflag_range(af an_af, bool val, size_t from, size_t to) {
		t_uint32* key = fl_pval(an_af);
		for (size_t i = from; i < to; i++) {
			t_uint32 bitmask = 1 << i;
			if (val) *key |= bitmask;
			else *key &= ~bitmask;
		}
		return *key;
	}

	t_uint32* fl_pval(af an_af) {
		switch (an_af) {
		case (af::alb_emb): return &ucfg_album_embed;
		case (af::alb_sd): return &ucfg_album_save_to_dir;
		case (af::alb_sa): return &ucfg_album_save_all;
		case (af::alb_ovr): return &ucfg_album_ovr;
		case (af::art_emb): return &ucfg_art_embed;
		case (af::art_sd): return &ucfg_art_save_to_dir;
		case (af::art_sa): return &ucfg_art_save_all;
		case (af::art_ovr): return &ucfg_art_ovr;
		default: PFC_ASSERT(false);
		}

		return 0;
	}

	void setflag(af fl, size_t pos,  bool val) {
		//todo: call fl_pval
		PFC_ASSERT(pos <= kBlockSize); 
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

	void setflag_range(af an_af, bool val, size_t from, size_t to) {
		PFC_ASSERT(to <= kBlockSize);
		setbitflag_range(an_af, val, from, to);
	}

	bool getbitflag(af an_af, size_t pos) {

		t_uint32* key = fl_pval(an_af);
		t_uint32 bitmask = 1 << pos;
		return (*key & bitmask) == bitmask;
	}

	bool getflag(af fl, size_t pos) {
		PFC_ASSERT(pos <= kBlockSize); //30bits
		return getbitflag(fl, pos);
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

		ucfg_album_embed = (cfg_album_embed ? setbitflag_range(af::alb_emb, true, 0, 31) : 0); //per track
		ucfg_album_save_to_dir = (cfg_album_save_to_dir ? setbitflag_range(af::alb_sd, true, 0, 1) : 0);
		ucfg_album_save_all = ((cfg_album_save_to_dir || cfg_album_embed) && cfg_album_save_all ? setbitflag_range(af::alb_sa, true, 0, 31) : 0); //todo
		ucfg_album_ovr = ((cfg_album_save_to_dir || cfg_album_embed) && cfg_album_ovr ? setbitflag_range(af::alb_ovr, true, 0, 31) : 0);
		ucfg_art_embed = (cfg_art_embed ? setbitflag_range(af::art_emb, true, 0, 31) : 0);
		ucfg_art_save_to_dir = (cfg_art_save_to_dir ? setbitflag_range(af::art_sd, true, 0, 1) : 0);
		ucfg_art_save_all = ((cfg_art_save_to_dir || cfg_art_embed) && cfg_art_save_all ? setbitflag_range(af::art_sa, true, 0, 31) : 0); //todo
		ucfg_art_ovr = ((cfg_art_save_to_dir || cfg_art_embed) &&  cfg_art_ovr ? setbitflag_range(af::art_ovr, true, 0, 31) : 0);
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

		populated = false;
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

	bool hasCustomSaveOrEmbed(const art_src src, const foo_conf& cfg) {
	
		uartwork uartconf = uartwork(cfg);
		bool bcust_save_or_embed = false;
		
		if (src == art_src::unknown || src == art_src::alb) {
			bcust_save_or_embed = ucfg_album_save_to_dir != uartconf.ucfg_album_save_to_dir;
			bcust_save_or_embed |= ucfg_album_ovr != uartconf.ucfg_album_ovr;
			bcust_save_or_embed |= ucfg_album_embed != uartconf.ucfg_album_embed;
		}
		
		if (src == art_src::unknown || src == art_src::art) {
			bcust_save_or_embed |= ucfg_art_save_to_dir != uartconf.ucfg_art_save_to_dir;
			bcust_save_or_embed |= ucfg_art_ovr != uartconf.ucfg_art_ovr;
			bcust_save_or_embed |= ucfg_art_embed != uartconf.ucfg_art_embed;
		}	
		return bcust_save_or_embed;
	}
};

const struct multi_uartwork {

	std::vector<uartwork> vuart;
	bool populated = false;

	void prep_block(size_t ndx) { if (vuart.size() < ndx + 1) vuart.emplace_back(uartwork()); }
	
	std::pair<size_t, size_t> get_block(size_t pos) {
		auto pres = std::pair(pos / kBlockSize /*31*/, pos % (kBlockSize));
		prep_block(pres.first); //todo: init once
		return pres;
	}

	t_uint32 setbitflag(t_uint32& key, size_t pos, bool val) {
		auto pbp = get_block(pos);
		return vuart.at(pbp.first).setbitflag(key, pbp.second, val);
	}

	t_uint32 setbitflag_range(af an_af, bool val, size_t from, size_t to) {

		auto pbp_first = get_block(from);
		auto pbp_last = get_block(to);

		bool block_range = pbp_last.first - pbp_first.first;
		for (size_t i = pbp_first.first; i < pbp_last.first; i++) {

			if (i == pbp_first.first) {
				//first block
				size_t ito = block_range ? kBlockSize - 1 : pbp_last.second;
				vuart.at(i).setbitflag_range(an_af, val, pbp_first.second, ito);
			}
			else if (i == pbp_last.first) {
				//last block
				if (block_range) {
					vuart.at(i).setbitflag_range(an_af, val, 0, pbp_last.second);
				}
			}
			else {
				//intermediates
				vuart.at(i).setbitflag_range(an_af, val, 0, kBlockSize - 1);
			}
		}
		return 0;	
	}

	void setflag(af fl, size_t pos, bool val) {
		auto pbp = get_block(pos);
		vuart.at(pbp.first).setflag(fl, pbp.second, val);
	}
	
	void setflag_range(af fl, bool val, size_t from, size_t to) {
		setbitflag_range(fl, val, from, to);
	}

	bool getbitflag(af an_af, size_t pos) {
		auto pbp = get_block(pos);
		return vuart.at(pbp.first).getbitflag(an_af, pbp.second);
	}

	bool save_to_dir(const art_src src) {
		return std::find_if(vuart.begin(), vuart.end(),	[=](const auto & uart) {
			return src == art_src::alb? uart.ucfg_album_save_to_dir != 0 : uart.ucfg_art_save_to_dir != 0;
			}) != vuart.end();
	}

	bool embed_art(const art_src src) {
		return std::find_if(vuart.begin(), vuart.end(),	[=](const auto & uart) {
			return src == art_src::alb ? uart.ucfg_album_embed != 0 : uart.ucfg_art_embed != 0;
			}) != vuart.end();
	}

	bool ovr_art(const art_src src) {
		return std::find_if(vuart.begin(), vuart.end(), [=](const auto & uart) {
			return src == art_src::alb ? uart.ucfg_album_ovr != 0 : uart.ucfg_art_ovr != 0;
			}) != vuart.end();
	}

	bool getflag(af fl, size_t pos) {
		auto pbp = get_block(pos);
		return vuart.at(pbp.first).getflag(fl, pbp.second);
	}

	multi_uartwork(
		bool cfg_album_embed,
		bool cfg_album_save_to_dir,
		bool cfg_album_save_all,
		bool cfg_album_ovr,
		bool cfg_art_embed,
		bool cfg_art_save_to_dir,
		bool cfg_art_save_all,
		bool cfg_art_ovr) {

		vuart.emplace_back(uartwork(cfg_album_embed,
			cfg_album_save_to_dir,
			cfg_album_save_all,
			cfg_album_ovr,
			cfg_art_embed,
			cfg_art_save_to_dir,
			cfg_art_save_all,
			cfg_art_ovr));

		populated = false;
	}

	multi_uartwork() {

		populated = false;
	};

	inline bool operator==(const multi_uartwork& rhs) {
		if (vuart.size() != rhs.vuart.size()) return false;
		if (!vuart.size()) {
			if (!rhs.vuart.size()) return true;
			return false;
		} 
		
		for (auto uart = vuart.begin(); uart != vuart.end(); uart++) {
			size_t ndx = std::distance(vuart.begin(), uart);
			bool eq = *uart == rhs.vuart.at(ndx);
			if (!eq) return false;
		}
		return true;
	}

	multi_uartwork(const CConf& conf) : multi_uartwork(
		conf.embed_album_art,
		conf.save_album_art,
		conf.album_art_fetch_all,
		conf.album_art_overwrite,
		conf.embed_artist_art,
		conf.save_artist_art,
		conf.artist_art_fetch_all,
		conf.artist_art_overwrite
	) {
		//..
	}

	bool isEmpty() {
		for (auto uart : vuart) {
			if (!(uart == uartwork())) return false;
		}
		return true;
	}

	bool hasCustomSaveOrEmbed(const art_src src, const foo_conf& cfg) {
		for (auto uart : vuart) {
			if (uart.hasCustomSaveOrEmbed(src, cfg)) return true;
		}
		return false;
	}
};

inline uartwork_guids CONFARTGUIDS;
inline multi_uartwork CONF_MULTI_ARTWORK;

const pfc::string8 THUMB_EXTENSION = ".png";
const size_t FLAG_FIRST_COL = 3;

std::pair<HBITMAP, HBITMAP> MemoryBlockToTmpBitmap(std::pair<pfc::string8, pfc::string8> cache_path, size_t pos, MemoryBlock small_art);
std::pair<HBITMAP, HBITMAP> GenerateTmpBitmapsFromRealSize(pfc::string8 release_id, size_t pos, pfc::string8 source_full_path, std::pair<pfc::string8, pfc::string8> &temp_file_names);
std::pair<pfc::string8, pfc::string8> ReadDimSizeFromFile(pfc::string8 path, pfc::string8 file_name);
MemoryBlock MemoryBlockToPngIcon(MemoryBlock buffer);
