/*
 * Copyright (C) 2011-2024 MicroSIP (http://www.microsip.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "StdAfx.h"   
#include "ButtonEx.h"   
#include "FourPhoneTheme.h"

// CButtonEx   
IMPLEMENT_DYNAMIC(CButtonEx, CMFCButton)
CButtonEx::CButtonEx()   
{   
	m_nFlatStyle = CMFCButton::BUTTONSTYLE_NOBORDERS;
	m_bTransparent = false;
	m_FaceColor = FourPhoneTheme::Brand();
	m_TextColor = FourPhoneTheme::White();
	m_Hovered = false;
	m_TrackingMouse = false;
}
   
CButtonEx::~CButtonEx()   
{   
}

BEGIN_MESSAGE_MAP(CButtonEx, CMFCButton)   
    //{{AFX_MSG_MAP(CButtonEx)
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CButtonEx::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_TrackingMouse) {
		TRACKMOUSEEVENT event = {};
		event.cbSize = sizeof(event);
		event.dwFlags = TME_LEAVE;
		event.hwndTrack = GetSafeHwnd();
		if (TrackMouseEvent(&event)) {
			m_TrackingMouse = true;
		}
	}
	if (!m_Hovered) {
		m_Hovered = true;
		Invalidate();
	}
	CMFCButton::OnMouseMove(nFlags, point);
}

LRESULT CButtonEx::OnMouseLeave(WPARAM, LPARAM)
{
	m_TrackingMouse = false;
	m_Hovered = false;
	Invalidate();
	return 0;
}

void CButtonEx::DrawItem(LPDRAWITEMSTRUCT drawItem)
{
	if (drawItem == NULL || drawItem->hDC == NULL) {
		return;
	}

	CDC dc;
	dc.Attach(drawItem->hDC);
	CRect bounds(drawItem->rcItem);
	dc.FillSolidRect(bounds, FourPhoneTheme::Canvas());

	const bool disabled = (drawItem->itemState & ODS_DISABLED) != 0;
	const bool pressed = (drawItem->itemState & ODS_SELECTED) != 0;
	COLORREF background = m_FaceColor;
	COLORREF text = m_TextColor;
	if (disabled) {
		background = RGB(226, 231, 233);
		text = FourPhoneTheme::Muted();
	}
	else if (pressed) {
		background = m_FaceColor == FourPhoneTheme::Brand()
			? FourPhoneTheme::BrandDark()
			: RGB(185, 45, 56);
	}
	else if (m_Hovered) {
		background = m_FaceColor == FourPhoneTheme::Brand()
			? RGB(8, 163, 113)
			: RGB(207, 57, 68);
	}

	CRect shape(bounds);
	shape.DeflateRect(2, 2);
	CPen pen(PS_SOLID, 1, background);
	CBrush brush(background);
	CPen* oldPen = dc.SelectObject(&pen);
	CBrush* oldBrush = dc.SelectObject(&brush);
	const int radius = FourPhoneTheme::Scale(GetSafeHwnd(), 10);
	dc.RoundRect(shape, CPoint(radius, radius));
	dc.SelectObject(oldBrush);
	dc.SelectObject(oldPen);

	CString caption;
	GetWindowText(caption);
	const int oldMode = dc.SetBkMode(TRANSPARENT);
	const COLORREF oldColor = dc.SetTextColor(text);
	CFont* font = GetFont();
	CFont* oldFont = font != NULL
		? dc.SelectObject(font)
		: NULL;
	dc.DrawText(
		caption,
		shape,
		DT_CENTER | DT_VCENTER | DT_SINGLELINE |
		DT_END_ELLIPSIS | DT_NOPREFIX);
	if (oldFont != NULL) {
		dc.SelectObject(oldFont);
	}
	dc.SetTextColor(oldColor);
	dc.SetBkMode(oldMode);
	if ((drawItem->itemState & ODS_FOCUS) != 0) {
		CRect focus(shape);
		focus.DeflateRect(4, 4);
		dc.DrawFocusRect(focus);
	}
	dc.Detach();
}

BOOL CButtonEx::EnableWindow(BOOL bEnable)
{
	if (bEnable) {
		SetTextColor(m_TextColor);
		SetFaceColor(m_FaceColor, true);
	}
	else {
//		SetTextColor(RGB(123, 123, 123));
		SetTextColor(RGB(0, 0, 0));
		SetFaceColor(RGB(222, 222, 222), true);
	}
	return CMFCButton::EnableWindow(bEnable);
}
