#include "stdafx.h"
#include "conf.h"
#include "conf_flag_mng.h"

//constructor

FlgMng::FlgMng(CConf* pconf, int ID) : m_pcfg(pconf), m_id(ID) {
	//..
}

//get flags

bool FlgMng::GetFlag(int flg) {

	int* varval = m_pcfg->id_to_ref_int(m_id);
	return  *varval & flg;
}

//set flags

bool FlgMng::SetFlag(HWND hWnd, UINT uid, int flg) {
	PFC_ASSERT(GetDlgItem(hWnd, uid));
	bool flgval = uButton_GetCheck(hWnd, uid);
	return SetFlag(flg, flgval);
}

bool FlgMng::SetFlag(int flg, bool flgval) {
	
	if (int* varval = m_pcfg->id_to_ref_int(m_id)) {
		std::bitset<8> bs = *varval;
		bs.set(std::log2(flg), flgval);
		*varval = bs.to_ulong();	
	}
	return flgval;
}
