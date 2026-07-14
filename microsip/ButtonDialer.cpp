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

#include "stdafx.h"
#include "ButtonDialer.h"
#include "Strsafe.h"
#include "const.h"
#include "FourPhoneTheme.h"

/////////////////////////////////////////////////////////////////////////////
// CButtonDialer

CButtonDialer::CButtonDialer()
{
	forceNumeric = false;
	m_map.SetAt(_T("1"), _T(""));
	m_map.SetAt(_T("2"), _T("ABC"));
	m_map.SetAt(_T("3"), _T("DEF"));
	m_map.SetAt(_T("4"), _T("GHI"));
	m_map.SetAt(_T("5"), _T("JKL"));
	m_map.SetAt(_T("6"), _T("MNO"));
	m_map.SetAt(_T("7"), _T("PQRS"));
	m_map.SetAt(_T("8"), _T("TUV"));
	m_map.SetAt(_T("9"), _T("WXYZ"));
	m_map.SetAt(_T("0"), _T(""));
	m_map.SetAt(_T("*"), _T(""));
	m_map.SetAt(_T("#"), _T(""));
}

CButtonDialer::~CButtonDialer()
{
	CloseTheme();
}


BEGIN_MESSAGE_MAP(CButtonDialer, CButton)
	ON_WM_THEMECHANGED()
	ON_WM_MOUSEMOVE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CButtonDialer message handlers

void CButtonDialer::PreSubclassWindow()
{
	OpenTheme();

	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONT lf;
	GetObject(hFont, sizeof(LOGFONT), &lf);
	lf.lfHeight = 12;
	CDC *pDC = GetDC();
	if (pDC) {
		dpiY = GetDeviceCaps(pDC->m_hDC, LOGPIXELSY);
		lf.lfHeight = MulDiv(lf.lfHeight, dpiY, 96);
		ReleaseDC(pDC);
	}
	else {
		dpiY = 96;
	}
	StringCchCopy(lf.lfFaceName, LF_FACESIZE, _T("Segoe UI"));
	lf.lfWeight = FW_SEMIBOLD;
	lf.lfQuality = CLEARTYPE_QUALITY;
	m_FontLetters.CreateFontIndirect(&lf);

	DWORD dwStyle = ::GetClassLong(m_hWnd, GCL_STYLE);
	dwStyle &= ~CS_DBLCLKS;
	::SetClassLong(m_hWnd, GCL_STYLE, dwStyle);
}

LRESULT CButtonDialer::OnThemeChanged()
{
	CloseTheme();
	OpenTheme();
	return 0L;
}

void CButtonDialer::OnSize(UINT type, int w, int h)
{
	CButton::OnSize(type, w, h);
}

void CButtonDialer::OnMouseMove(UINT nFlags, CPoint point)
{
	CRect rect;
	GetClientRect(&rect);
	if (rect.PtInRect(point)) {
		if (GetCapture() != this) {
			SetCapture();
			Invalidate();
		}
	}
	else {
		ReleaseCapture();
		Invalidate();
	}
}

void CButtonDialer::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC dc;
	dc.Attach(lpDrawItemStruct->hDC);
	CRect rt(lpDrawItemStruct->rcItem);
	dc.FillSolidRect(rt, FourPhoneTheme::Canvas());
	dc.SetBkMode(TRANSPARENT);

	const UINT state = lpDrawItemStruct->itemState;
	const bool pressed = (state & ODS_SELECTED) != 0;
	const bool disabled = (state & ODS_DISABLED) != 0;
	const bool hovered = GetCapture() == this;
	COLORREF background = FourPhoneTheme::White();
	COLORREF border = FourPhoneTheme::Line();
	if (pressed) {
		background = FourPhoneTheme::BrandSurface();
		border = FourPhoneTheme::Brand();
	}
	else if (hovered) {
		background = RGB(248, 252, 250);
		border = RGB(183, 217, 202);
	}
	if (disabled) {
		background = RGB(241, 243, 244);
		border = FourPhoneTheme::Line();
	}

	CString strTemp;
	GetWindowText(strTemp);

	CRect shape(rt);
	shape.DeflateRect(
		FourPhoneTheme::Scale(GetSafeHwnd(), 3),
		FourPhoneTheme::Scale(GetSafeHwnd(), 2));
	CPen borderPen(PS_SOLID, 1, border);
	CBrush backgroundBrush(background);
	CPen* oldPen = dc.SelectObject(&borderPen);
	CBrush* oldBrush = dc.SelectObject(&backgroundBrush);
	const int radius = FourPhoneTheme::Scale(GetSafeHwnd(), 10);
	dc.RoundRect(shape, CPoint(radius, radius));
	dc.SelectObject(oldBrush);
	dc.SelectObject(oldPen);

	CString letters;
	const COLORREF mainText = disabled
		? FourPhoneTheme::Muted()
		: FourPhoneTheme::Ink();
	const COLORREF oldColor = dc.SetTextColor(mainText);
	if (!forceNumeric && m_map.Lookup(strTemp, letters)) {
		CRect digitBounds(shape);
		if (!letters.IsEmpty()) {
			digitBounds.bottom -= FourPhoneTheme::Scale(
				GetSafeHwnd(),
				5);
		}
		dc.DrawText(
			strTemp,
			digitBounds,
			DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

		HFONT hOldFont = (HFONT)SelectObject(dc.m_hDC, m_FontLetters);
		CRect letterBounds(shape);
		letterBounds.top = shape.CenterPoint().y +
			FourPhoneTheme::Scale(GetSafeHwnd(), 5);
		dc.SetTextColor(FourPhoneTheme::Muted());
		dc.DrawText(
			letters,
			letterBounds,
			DT_CENTER | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
		SelectObject(dc.m_hDC, hOldFont);
	}
	else {
		dc.SetTextColor(
			forceNumeric
				? mainText
				: FourPhoneTheme::Muted());
		dc.DrawText(
			strTemp,
			shape,
			DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}
	dc.SetTextColor(oldColor);

	if ((state & ODS_FOCUS) != 0) {
		CRect focus(shape);
		focus.DeflateRect(3, 3);
		dc.DrawFocusRect(focus);
	}
	dc.Detach();
}
