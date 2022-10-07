#pragma once
#include "resource.h"

class CTristate {

  const UINT IDC = IDC_CHK_SKIP_ARTWORK;

public:

  CTristate(CWindow* parent) : parent(parent) { }

  void SetTristate(UINT prestate, UINT poststate);
  void RemoveTristate();
  void SetBistate();
  void SetBistate(UINT state);

  void Init(UINT state, BOOL enabled);
  UINT GetState();
  bool IsTri();

private:

  CWindow* parent;
  HWND getWnd() { return uGetDlgItem(parent->m_hWnd, IDC); }
};