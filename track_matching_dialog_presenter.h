#pragma once

#include <vector>
#include <variant>
#include <any>

#include "atlctrls.h"
#include "pfc/pfc.h"
#include "SDK/album_art.h"

#include "libPPUI/listview_helper.h"
#include "libPPUI/CListControlOwnerData.h"

#include "multiformat.h"

#include "track_matching_utils.h"
#include "tag_writer.h"

#include "resource.h"


#define ENCODE_DISCOGS(d,t)		((d==-1||t==-1) ? -1 : ((d<<16)|t))
#define DEF_TIME_COL_WIDTH 45  //96dpi

enum class lsmode { tracks_ui = 0, art, unknown, default = 0 };

class coord_presenters;

//DISCOGS TRACKS + LOCAL FILES...

struct file_match_nfo {
	int track = -1;
	int disc = -1;
};

//match_info_t: display, formatted length
typedef std::pair<pfc::string8, pfc::string8> match_info_t;

//track_match_t: match info_t, (for tracks: ENCODE_DISCOGS(mapping.discogs_disc, mapping.discogs_track), for files: file index)
typedef std::pair<match_info_t, file_match_nfo> track_match_t;

//file_match_t: match_info_t, file_match_nfo
typedef std::pair<match_info_t, size_t> file_match_t;

//discogs tracks vector ...
typedef std::vector<track_match_t> vtracks_t;
typedef vtracks_t::iterator track_it;
//local files vector ...
typedef std::vector<file_match_t> vfile_t;
typedef vfile_t::iterator file_it;

//ARTWORK IMAGES ...

//typedef std::vector<std::pair<std::pair<int, Discogs::Image_ptr>, std::vector<pfc::string8>>> getimages_t;
typedef std::pair<int, Discogs::Image_ptr> ndx_image_t;
typedef std::vector<pfc::string8> image_info_t;
typedef std::pair < ndx_image_t, image_info_t> ndx_image_info_row_t;

//discogs images vector...
typedef std::vector<ndx_image_info_row_t> getimages_t;
typedef getimages_t::iterator getimages_it;

//local files images...
typedef std::pair<int, GUID> ndx_image_file_t;
typedef std::pair < ndx_image_file_t, image_info_t> ndx_image_file_info_row_t;

//local images files vector ...
typedef std::vector<ndx_image_file_info_row_t> getimages_file_t;
typedef getimages_file_t::iterator getimages_file_it;

using V = std::variant<
	std::pair<size_t, track_it>,
	std::pair<size_t, file_it>,
	std::pair<size_t, getimages_it>,
	std::pair<size_t, getimages_file_it>>;

typedef std::vector<V>::iterator var_it_t;

class presenter {

public:

	CImageList m_lstimg;
	CImageList m_lstimg_small;
	DWORD m_dwHeaderStyle = 0L;

public:

	presenter(coord_presenters* coord, foo_discogs_conf conf) : presenter() {

		mm_hWnd = NULL;
		m_conf = conf;
		m_coord = coord;
		//m_wlist = NULL;
		m_listID = 0;
		m_loaded = false;
		m_dwHeaderStyle = 0L;
		m_row_height = false;

		m_tag_writer = nullptr;
		m_release = nullptr;
		m_conf_col_woa = {};
		
	}
	presenter() : mm_hWnd(NULL), m_conf(CONF),
		m_coord(), /*m_wlist(),*/ m_listID(0), m_loaded(false),
		m_dwHeaderStyle(0L), m_row_height(false)  {

		m_tag_writer = nullptr;
		m_release = nullptr;
		m_conf_col_woa = {};

	}

	~presenter() {
		
		//should be automatic...
		//BOOL bres = ImageList_Destroy(m_lstimg);

	}

	void Load();
	void Unload();
	std::vector<int> Woas() { return m_conf_col_woa; }

	bool IsLoaded() { return loaded(); }
	bool IsTile();
	bool RowHeigth() { return m_row_height; }

	void Init(HWND hwndDlg, TagWriter_ptr tag_writer, foo_discogs_conf& conf, UINT ID);
	int listID() { return m_listID; }

	HWND GetListView() { 
		return uGetDlgItem(mm_hWnd, m_listID);
	}
	
	void SetTagWriter(TagWriter_ptr tag_writer) {
		m_tag_writer = tag_writer;		
		m_release = tag_writer->release;
	}

	void SetRelease(Release_ptr release) { m_release = release; }

	virtual size_t GetVLvRow(size_t list_position, var_it_t& out) = 0;

	virtual size_t GetDataLvSize() = 0;
	virtual size_t GetDataSize() = 0;

	virtual size_t DeleteLvRow(size_t position) = 0;

	virtual void Reset() {}

	virtual bool SwapMapItem(size_t key1, size_t key2) = 0;
	virtual void ReorderMapItem(size_t const* order, size_t count) = 0;

  std::vector<pfc::string8>m_vtitles;

	virtual void build_cfg_columns(foo_discogs_conf* m_conf = NULL) {};

protected:

	virtual void display() {}
	virtual void create_columns() {}
	virtual void define_columns() {}
	virtual void insert_columns() {}
	virtual void display_columns() {}

  virtual void update_list_width(bool initialize) {}
	virtual void set_row_height(bool assign) {}

	bool loaded() { return m_loaded; }

	void set_lv_images_lists();

	std::pair<CImageList*, CImageList*> get_imagelists() {
		return std::pair<CImageList*, CImageList*>(&m_lstimg, &m_lstimg_small);
	}

public:
protected:

	HWND mm_hWnd;
	bool m_loaded;

	foo_discogs_conf& m_conf;
	TagWriter_ptr m_tag_writer;
	Release_ptr m_release;

	std::vector<int> m_conf_col_woa;
	bool m_row_height;

private:

  int m_listID;
	coord_presenters* m_coord;
};


class track_presenter : public presenter {

public:

	track_presenter(coord_presenters* coord, foo_discogs_conf conf) : m_ui_list(NULL), presenter(coord, conf) {
	}

	track_presenter() : m_ui_list(NULL) {}

	virtual void AddRow(std::any track) {}

	virtual size_t GetVRow(size_t list_position, var_it_t& out) = 0;

	virtual size_t GetVLvRow(size_t list_position, var_it_t& out) = 0;

	virtual size_t GetDataSize() = 0; // override { return ~0; };
	virtual size_t GetDataLvSize() = 0; //override { return ~0; };

	size_t DeleteLvRow(size_t position) = 0;
	
	virtual bool SwapMapItem(size_t key1, size_t key2) = 0;
	virtual void ReorderMapItem(size_t const* order, size_t count) = 0;

	virtual void SetUIList(CListControlOwnerData* uilist);
	void SetHeaderColumnFormat(size_t icol, size_t fmt);
	size_t GetHeaderColumnFormat(size_t icol);
	size_t HitTest(CPoint point);

	void Reset() override {}

protected:

	void display();
	void insert_columns() override;
	void update_list_width(bool initialize);
	void set_row_height(bool assign) override;

	CListControlOwnerData* m_ui_list;

private:


};

class discogs_track_libui_presenter : public track_presenter {

public:

	discogs_track_libui_presenter(coord_presenters* coord, foo_discogs_conf conf) :
		track_presenter(coord, conf) {
		m_vtracks = {};
		m_lvtracks = {};
		m_ui_list = NULL;
	}

	discogs_track_libui_presenter() {
	
		m_vtracks = {};
		m_lvtracks = {};
		m_ui_list = NULL;
	}

	~discogs_track_libui_presenter() {
		int debug = 0;
	}

	void AddRow(std::any track) override;

	size_t GetVRow(size_t list_position, var_it_t& out) override;

	size_t GetVLvRow(size_t list_position, var_it_t& out) {
		std::vector<V>::iterator v_it;
		v_it = m_lvtracks.begin();
		std::advance(v_it, list_position);
		out = v_it;
		return std::distance(m_vtracks.begin(), std::get<0>(*v_it).second);
	}

	t_size GetDataSize() override { return m_vtracks.size(); }
	t_size GetDataLvSize() override { return m_lvtracks.size(); }

	size_t DeleteLvRow(size_t position) override {
		std::vector<V>::iterator v_it = m_lvtracks.begin() + position;
		track_it img_it = std::get<0>(*v_it).second;
		size_t ndx_deleted = std::distance(m_vtracks.begin(), img_it);
		m_lvtracks.erase(m_lvtracks.begin() + position);
		return ndx_deleted;
	}

	bool SwapMapItem(size_t key1, size_t key2) override {
		bool bres = key1 < m_lvtracks.size() && key2 < m_lvtracks.size();

		if (bres) std::swap(m_lvtracks[key1], m_lvtracks[key2]);
		return bres;
	}

	void ReorderMapItem(size_t const* order, size_t count) override {	
		pfc::reorder_t(m_lvtracks, order, count);
	}

	void Reset() override { m_vtracks.clear(); m_lvtracks.clear(); }

protected:

	void define_columns() override;
	void build_cfg_columns(foo_discogs_conf* out_conf) override;

private:

	vtracks_t m_vtracks;
	std::vector<V> m_lvtracks;

};

class file_track_libui_presenter : public track_presenter {

public:

	file_track_libui_presenter(coord_presenters* coord, foo_discogs_conf conf) :
		track_presenter(coord, conf) {

		m_ui_list = NULL;
		m_vfiles = {};
		m_lvfiles = {};
	}

	file_track_libui_presenter() {
		m_ui_list = NULL;
		m_vfiles = {};
		m_lvfiles = {};
	}

	~file_track_libui_presenter() {
		//int debug = 0;
	}

	void AddRow(std::any file) override;

	size_t GetVRow(size_t list_position, var_it_t& out) override;

	size_t GetVLvRow(size_t list_position, var_it_t& out) {
		std::vector<V>::iterator v_it;
		v_it = m_lvfiles.begin();
		std::advance(v_it, list_position);
		out = v_it;
		return std::distance(m_vfiles.begin(), std::get<1>(*v_it).second);
	}

	t_size GetDataSize() override { return m_vfiles.size(); }
	t_size GetDataLvSize() override { return m_lvfiles.size(); }

	size_t DeleteLvRow(size_t position) override {
		std::vector<V>::iterator v_it = m_lvfiles.begin() + position;
		file_it img_it = std::get<1>(*v_it).second;
		size_t ndx_deleted = std::distance(m_vfiles.begin(), img_it);
		m_lvfiles.erase(m_lvfiles.begin() + position);
		return ndx_deleted;
	}
	bool SwapMapItem(size_t key1, size_t key2) override {
		bool bres = key1 < m_lvfiles.size() && key2 < m_lvfiles.size();
		if (bres) std::swap(m_lvfiles.at(key1), m_lvfiles.at(key2));
		return bres;
	}
	void ReorderMapItem(size_t const* order, size_t count) override {
		pfc::reorder_t(m_lvfiles, order, count);
	}
	void Reset() override { m_vfiles.clear(); m_lvfiles.clear(); }

protected:

	void define_columns() override;
	void build_cfg_columns(foo_discogs_conf* out_conf) override;

private:

	vfile_t m_vfiles;
	std::vector<V> m_lvfiles;

};

class artwork_presenter : public presenter {

public:

	artwork_presenter(coord_presenters* coord, foo_discogs_conf conf) :
		presenter(coord, conf) {

	}

	artwork_presenter() {}

	virtual size_t get_icon_id(size_t iImageList) = 0;

	virtual void AddRow(std::any track) {}

	virtual size_t GetVRow(size_t list_position, var_it_t& out) = 0;
	size_t GetVLvRow(size_t list_position, var_it_t& out) = 0;

	size_t GetDataSize() = 0;
	size_t GetDataLvSize() = 0;

	size_t DeleteLvRow(size_t position) = 0;

	bool SwapMapItem(size_t key1, size_t key2) = 0;
	void ReorderMapItem(size_t const* order, size_t count) = 0;
	
	std::pair<pfc::string8, pfc::string8> TagWriterImgInfo(art_src art_source, size_t pos) {
		return get_tag_writer_img_finfo(art_source, pos);
	}

	void Reset() override {}


protected:

	void display();
	void update_imagelist(size_t img_ndx, size_t max_img, std::pair<HBITMAP, HBITMAP> hRES);

	void set_row_height(bool assign) override;
	void insert_columns() override;

	pfc::string8 get_header_label(pfc::string8 header);
	pfc::string8 get_tickit_label(af att, bool val);

	std::pair<pfc::string8, pfc::string8> get_tag_writer_img_finfo(art_src src, size_t pos);

};


class discogs_artwork_presenter : public artwork_presenter {

public:

	discogs_artwork_presenter(coord_presenters* coord, foo_discogs_conf conf) :
		artwork_presenter(coord, conf) {

		m_vimages = {};
		m_lvimages = {};

		m_uart = uartwork(conf);
	}

	discogs_artwork_presenter() {}

	size_t get_icon_id(size_t iImageList) override;

	void AddRow(std::any track) override;

	//void GetRow(size_t list_position, getimages_it& out) override;
	size_t GetVRow(size_t list_position, var_it_t& out) override;

	size_t GetVLvRow(size_t list_position, var_it_t& out) {

		std::vector<V>::iterator v_it;
		v_it = m_lvimages.begin();
		std::advance(v_it, list_position);
		out = v_it;
		return std::distance(m_vimages.begin(), std::get<2>(*v_it).second);
	}

	t_size GetDataSize() override { return m_vimages.size(); }
	t_size GetDataLvSize() override { return m_lvimages.size(); }

	size_t DeleteLvRow(size_t position) override {
		std::vector<V>::iterator v_it = m_lvimages.begin() + position;
		getimages_it img_it = std::get<2>(*v_it).second;
		size_t ndx_deleted = std::distance(m_vimages.begin(), img_it);
		m_lvimages.erase(m_lvimages.begin() + position);
		return ndx_deleted;
	}

	bool SwapMapItem(size_t key1, size_t key2) override {
		bool bres = key1 < m_lvimages.size() && key2 < m_lvimages.size();
		if (bres) std::swap(m_lvimages.at(key1), m_lvimages.at(key2));
		return bres;
	}
	void ReorderMapItem(size_t const* order, size_t count) override {
		pfc::reorder_t(m_lvimages, order, count);
	}

	void Populate();
	void PopulateConfArtWork();
	bool AddArtwork(size_t img_ndx, art_src art_source, MemoryBlock small_art);
	
	art_src GetSrcTypeAtPos(size_t list_position) { return get_vimages_src_type_at_pos(list_position); }
	
	uartwork SetUartwork(uartwork uart) { m_uart = uart; }
	uartwork SetUartwork_guids(uartwork_guids uart_guids) { m_uart_guids = uart_guids; }
	uartwork* GetUartwork() { return &m_uart; }
	uartwork_guids* GetUartwork_guids() { return &m_uart_guids; }

	void Reset() override { m_vimages.clear(); m_lvimages.clear(); m_uart = uartwork(m_conf); }

protected:

	void define_columns() override;
	void update_list_width(bool initialize) override;
	void display_columns() override;
	void build_cfg_columns(foo_discogs_conf* out_conf) override;

	art_src get_vimages_src_type_at_pos(size_t pos);
	size_t get_ndx_at_pos(size_t pos);

private:
	uartwork m_uart;
	uartwork_guids m_uart_guids;

	getimages_t m_vimages;

	std::vector<V> m_lvimages;
	
};


class files_artwork_presenter : public artwork_presenter {

public:

	files_artwork_presenter(coord_presenters* coord, foo_discogs_conf conf) :
		artwork_presenter(coord, conf) {

		m_vimage_files = {};
		m_lvimage_files = {};

		m_vtemp_files.resize(album_art_ids::num_types());

	}

	files_artwork_presenter() {}

	~files_artwork_presenter() {

		for (auto tmp_pair : m_vtemp_files) {
			uDeleteFile(tmp_pair.first);
			uDeleteFile(tmp_pair.second);
		}
	}

	size_t get_icon_id(size_t iImageList) override;

	void AddRow(std::any file) override;

	size_t GetVRow(size_t list_position, var_it_t& out) override;

	size_t GetVLvRow(size_t list_position, var_it_t& out) {
		std::vector<V>::iterator v_it;
		v_it = m_lvimage_files.begin();
		std::advance(v_it, list_position);
		out = v_it;
		return std::distance(m_vimage_files.begin(), std::get<3>(*v_it).second);
	}

	t_size GetDataSize() override { return m_vimage_files.size(); }
	t_size GetDataLvSize() override { return m_lvimage_files.size(); }

	size_t DeleteLvRow(size_t position) override { 
		std::vector<V>::iterator v_it = m_lvimage_files.begin() + position;
		getimages_file_it img_it = std::get<3>(*v_it).second;
		size_t ndx_deleted = std::distance(m_vimage_files.begin(), img_it);
		m_lvimage_files.erase(m_lvimage_files.begin() + position);
		return ndx_deleted;
	}
	
	bool SwapMapItem(size_t key1, size_t key2) override {
		bool bres = key1 < m_lvimage_files.size() && key2 < m_lvimage_files.size();
		if (bres) std::swap(m_lvimage_files.at(key1), m_lvimage_files.at(key2));
		return bres;
	}
	void ReorderMapItem(size_t const* order, size_t count) override {
		pfc::reorder_t(m_lvimage_files, order, count);
	}
	size_t GetLvRow(size_t position, getimages_file_it& out) {
		if (!m_lvimage_files.size() || position >= m_lvimage_files.size()) return pfc_infinite;

		std::vector<V>::iterator v_it = m_lvimage_files.begin();
		std::advance(v_it, position);
		out = std::get<3>(*v_it).second;
		return std::distance(m_vimage_files.begin(), out);
	}

	void Reset() override { m_vimage_files.clear(); m_lvimage_files.clear(); }
	void ImageListReset(pfc::array_t<GUID> album_art_ids);

	void GetExistingArtwork();
	void Populate();
	size_t AddFileArtwork(size_t img_ndx, art_src art_source,
		std::pair<HBITMAP, HBITMAP> callback_mb, std::pair<pfc::string8, pfc::string8> temp_file_names);

protected:

	void define_columns() override;
	void display_columns() override;
	void update_list_width(bool initialize) override;

	void build_cfg_columns(foo_discogs_conf* out_conf) override;

	void update_img_defs(size_t img_ndx, size_t img_ids);

private:

	getimages_file_t m_vimage_files;
	std::vector<V> m_lvimage_files;

	std::vector <std::pair<pfc::string8, pfc::string8>> m_vtemp_files;
};

class coord_presenters {

public:
	
	coord_presenters(HWND hparent, const foo_discogs_conf discogs_conf) :

		m_hWnd(hparent),
		m_conf(discogs_conf),

		m_discogs_track_libui_presenter(this, discogs_conf),
		m_file_track_libui_presenter(this, discogs_conf),
		m_discogs_art_presenter(this, discogs_conf),
		m_file_art_presenter(this, discogs_conf),

		m_cImageTileMode(0),
		m_lsmode(lsmode::default) {}

	~coord_presenters() {
		for (auto bin : form_mode) {
			bin.second->first.Unload();
			bin.second->second.Unload();
		}
	}

	void swap_map_elements(HWND hwnd, const size_t key1, const size_t key2, lsmode mode = lsmode::default);
	void reorder_map_elements(HWND hwnd, size_t const* order, size_t count, lsmode mode = lsmode::default);

  void set_listview_mode() {}

	void populate_track_ui_mode();
	void populate_artwork_mode(size_t select = 0);

  size_t GetTileMode() { return m_cImageTileMode; }
	void SetTileMode(int mode) {}

	void ListUserCmd(HWND hwnd, lsmode mode, int cmd, bit_array_bittable cmdmask, bit_array_bittable are_albums, bool cmdmod = false);

	void PullConf(lsmode mode, bool tracks, foo_discogs_conf* out_conf);

	uartwork* GetUartwork() {
		//return form_mode["artwork"]->first.GetUartwork();
		return m_discogs_art_presenter.GetUartwork();
	}

	uartwork_guids* GetUartwork_GUIDS() {

		return m_discogs_art_presenter.GetUartwork_guids();
	}

	std::pair<pfc::string8, pfc::string8> GetDiscogsArtInfo(art_src art_source, size_t list_position) {
		return m_discogs_art_presenter.TagWriterImgInfo(art_source, list_position);
	}
	std::pair<pfc::string8, pfc::string8> GetFileArtInfo(art_src art_source, size_t list_position) {
		return m_file_art_presenter.TagWriterImgInfo(art_source, list_position);
	}

	/*
	using V = std::variant<
	std::pair<size_t, track_it>,
	std::pair<size_t, file_it>,
	std::pair<size_t, getimages_it>,
	std::pair<size_t, getimages_file_it>>;
	*/

	size_t Get_V_LvRow(lsmode mode, bool tracks, size_t list_position, var_it_t& out) {
		
		presenter* pres;
		if (tracks)
			pres = &form_mode[mode]->first;
		else
			pres = &form_mode[mode]->second;

		if (list_position >= pres->GetDataLvSize()) {
			return pfc_infinite;
		}
		else
			return pres->GetVLvRow(list_position, out);
	}

	void FileArtDeleteImageList(pfc::array_t<GUID> album_art_ids);

	size_t GetDiscogsTrackUiLvSize() {	return m_discogs_track_libui_presenter.GetDataLvSize();	}
	size_t GetFileTrackUiLvSize() {	return m_file_track_libui_presenter.GetDataLvSize(); };
	size_t GetDiscogsArtRowLvSize() {	return m_discogs_art_presenter.GetDataLvSize();	}
	size_t GetFileArtRowLvSize() { return m_file_art_presenter.GetDataLvSize();	}

	size_t GetDiscogsTrackUiAtLvPos(size_t list_position, track_it& out);
	size_t GetFileTrackUiAtLvPos(size_t list_position, file_it& out);
	size_t GetFileArtAtLvPos(size_t list_position, getimages_file_it& out);

	size_t GetLvFileArtRow(size_t list_position, getimages_file_it& out) {
		return m_file_art_presenter.GetLvRow(list_position, out);
	}

	art_src GetSourceTypeAtPos(size_t list_position) {
		return m_discogs_art_presenter.GetSrcTypeAtPos(list_position);
	}

	void SetTagWriter(TagWriter_ptr tag_writer);
	TagWriter_ptr GetTagWriter() { return m_tag_writer; }

	void Initialize(HWND hparent, const foo_discogs_conf* dcconf);
	void SetMode(lsmode mode);
	void Show(bool showleft = true, bool showright = true);

	void InitUiList(HWND hwnd, lsmode mode, bool tracks, CListControlOwnerData* uilist);
	std::pair<size_t, presenter*> HitUiTest(CPoint point);
	void SetUiColumnFormat(size_t icol, presenter* pres, size_t fmt);
	size_t GetUiColumnFormat(size_t icol, presenter* pres);

	std::vector<pfc::string8> Get_Titles(lsmode mode, bool tracks);
	
	void Reset(HWND hlist, lsmode mode);

	void InitFormMode(lsmode mode, UINT lvleft, UINT lvright);

	bool show_artwork_preview(size_t image_ndx, art_src art_source, MemoryBlock small_art);
	bool show_file_artwork_preview(size_t image_ndx, art_src art_source, std::pair<HBITMAP, HBITMAP> callback_mb,
		std::pair<pfc::string8, pfc::string8> temp_file_names);

	foo_discogs_conf* cfgRef();

private:

	void ShowFormMode(lsmode mode, bool showleft, bool showright /*, bool populate*/);

private:

	HWND m_hWnd;
	lsmode m_lsmode;
    size_t m_cImageTileMode;

	typedef std::pair<presenter&, presenter&> binomial_t;
	typedef std::vector<binomial_t> binomials_t;
	typedef binomials_t::iterator binomials_it;
	
	binomials_t binomials;
	std::map<lsmode, binomials_it> form_mode;

	discogs_track_libui_presenter m_discogs_track_libui_presenter;
	file_track_libui_presenter m_file_track_libui_presenter;
	discogs_artwork_presenter m_discogs_art_presenter;
	files_artwork_presenter m_file_art_presenter;

	Release_ptr m_tag_writer_release;
	TagWriter_ptr m_tag_writer;
	titleformat_hook_impl_multiformat m_hook;
	file_info_impl m_info;
	playable_location_impl m_location;

	foo_discogs_conf m_conf;
	
};

/*
#define LV_VIEW_ICON            0x0000
#define LV_VIEW_DETAILS         0x0001
#define LV_VIEW_SMALLICON       0x0002
#define LV_VIEW_LIST            0x0003
#define LV_VIEW_TILE            0x0004
#define LV_VIEW_MAX             0x0004
*/
