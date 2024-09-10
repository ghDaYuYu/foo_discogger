#pragma once
#include <bitset>

class CConf;

class FlgMng {

public:

	FlgMng() : m_id(0), m_pcfg(nullptr) {};
	FlgMng(CConf* pconf, int ID);
	FlgMng(CConf* conf, int ID, int *flag_var) : m_pcfg(conf), m_id(ID) {}

	int id() { return m_id; }
	bool SetFlag(HWND hWnd, UINT uid, int flg);
	bool SetFlag(int flg, bool flgval);
	bool GetFlag(int flg);
	friend class CConf;

private:

	int m_id;
	CConf* m_pcfg;
};
