// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_TOOLS_PLAYER_WTL_SEEK_H_
#define MEDIA_TOOLS_PLAYER_WTL_SEEK_H_

#include "media/tools/player_wtl/player_wtl.h"

// Movie seek dialog.
// TODO(fbachard): Frame properties only work for images, so
// this tab is removed until movie frame properties can be added.
class CSeek
    : public CSimpleDialog<IDD_SEEK>,
      public CMessageFilter,
      public CIdleHandler {
 public:
  CSeek() {
  }

  virtual BOOL PreTranslateMessage(MSG* pMsg) {
    return FALSE;
  }

  BEGIN_MSG_MAP(CSeek)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    CHAIN_MSG_MAP(CSimpleDialog<IDD_SEEK>)
  END_MSG_MAP()

  LRESULT OnPaint(UINT /*uMsg*/,
                  WPARAM /*wParam*/,
                  LPARAM /*lParam*/,
                  BOOL& bHandled) {
    static float previous_position = -1.0f;

    float position = media::Movie::GetInstance()->GetPosition();
    if (static_cast<int>(position * 10) !=
        static_cast<int>(previous_position * 10)) {
      previous_position = position;
      wchar_t szBuff[200];
      float duration = media::Movie::GetInstance()->GetDuration();
      float fps = 29.97f;
      wsprintf(szBuff, L"%i.%i / %i.%i, %i / %i",
               static_cast<int>(position),
               static_cast<int>(position * 10) % 10,
               static_cast<int>(duration),
               static_cast<int>(duration * 10) % 10,
               static_cast<int>(position * fps),
               static_cast<int>(duration * fps));
      SetDlgItemText(IDC_SEEKLOCATION, szBuff);
      bHandled = TRUE;
      return FALSE;
    }
    bHandled = FALSE;
    return FALSE;
  }

  virtual BOOL OnIdle() {
    wchar_t szBuff[200];
    float position = media::Movie::GetInstance()->GetPosition();
    float duration = media::Movie::GetInstance()->GetDuration();
    // TODO(fbarchard): Use frame rate property when it exists.
    float fps = 29.97f;
    wsprintf(szBuff, L"%i.%i / %i.%i, %i / %i",
             static_cast<int>(position),
             static_cast<int>(position * 10) % 10,
             static_cast<int>(duration),
             static_cast<int>(duration * 10) % 10,
             static_cast<int>(position * fps),
             static_cast<int>(duration * fps));
    SetDlgItemText(IDC_SEEKLOCATION, szBuff);
    return FALSE;
  }

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/,
                       BOOL& /*bHandled*/) {
    CMessageLoop* pLoop = g_module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->AddMessageFilter(this);
    pLoop->AddIdleHandler(this);
    return TRUE;
  }

  LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/,
                    BOOL& bHandled) {
    // unregister message filtering and idle updates
    CMessageLoop* pLoop = g_module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->RemoveMessageFilter(this);
    pLoop->RemoveIdleHandler(this);
    bHandled = FALSE;
    return 1;
  }
};

#endif  // MEDIA_TOOLS_PLAYER_WTL_SEEK_H_
