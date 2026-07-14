#include "stdafx.h"
#include "FourPhoneTheme.h"

#include <algorithm>
#include <dwmapi.h>
#include <strsafe.h>
#include <uxtheme.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "UxTheme.lib")

namespace
{
	BOOL CALLBACK ApplyFontCallback(HWND window, LPARAM parameter)
	{
		::SendMessage(
			window,
			WM_SETFONT,
			static_cast<WPARAM>(parameter),
			TRUE);
		return TRUE;
	}

	BOOL CALLBACK PrepareControlCallback(HWND window, LPARAM)
	{
		TCHAR className[32] = {};
		if (!GetClassName(window, className, _countof(className))) {
			return TRUE;
		}

		if (_tcsicmp(className, _T("Edit")) == 0) {
			const int margin = FourPhoneTheme::Scale(window, 8);
			::SendMessage(
				window,
				EM_SETMARGINS,
				EC_LEFTMARGIN | EC_RIGHTMARGIN,
				MAKELPARAM(margin, margin));
		}
		else if (_tcsicmp(className, WC_LISTVIEW) == 0 ||
			_tcsicmp(className, WC_TREEVIEW) == 0) {
			SetWindowTheme(window, L"Explorer", NULL);
		}
		return TRUE;
	}

	UINT GetWindowDpi(HWND window)
	{
		typedef UINT(WINAPI* GetDpiForWindowFunction)(HWND);
		static GetDpiForWindowFunction getDpiForWindow =
			reinterpret_cast<GetDpiForWindowFunction>(
				GetProcAddress(
					GetModuleHandle(_T("user32.dll")),
					"GetDpiForWindow"));
		if (window != NULL && getDpiForWindow != NULL) {
			const UINT dpi = getDpiForWindow(window);
			if (dpi != 0) {
				return dpi;
			}
		}

		UINT dpi = 96;
		HDC dc = ::GetDC(window);
		if (dc != NULL) {
			dpi = static_cast<UINT>(GetDeviceCaps(dc, LOGPIXELSY));
			::ReleaseDC(window, dc);
		}
		return dpi;
	}

	void DrawRoundRect(
		CDC& dc,
		const CRect& bounds,
		COLORREF fillColor,
		COLORREF borderColor,
		int radius,
		int borderWidth = 1)
	{
		CPen border(PS_SOLID, borderWidth, borderColor);
		CBrush fill(fillColor);
		CPen* oldPen = dc.SelectObject(&border);
		CBrush* oldBrush = dc.SelectObject(&fill);
		dc.RoundRect(bounds, CPoint(radius, radius));
		dc.SelectObject(oldBrush);
		dc.SelectObject(oldPen);
	}
}

namespace FourPhoneTheme
{
	COLORREF Brand()
	{
		return RGB(16, 185, 129);
	}

	COLORREF BrandDark()
	{
		return RGB(5, 150, 105);
	}

	COLORREF BrandLight()
	{
		return RGB(52, 211, 153);
	}

	COLORREF BrandSurface()
	{
		return RGB(236, 253, 245);
	}

	COLORREF Ink()
	{
		return RGB(15, 21, 32);
	}

	COLORREF InkSoft()
	{
		return RGB(77, 86, 99);
	}

	COLORREF Muted()
	{
		return RGB(125, 135, 146);
	}

	COLORREF Line()
	{
		return RGB(231, 234, 236);
	}

	COLORREF Canvas()
	{
		return RGB(245, 247, 248);
	}

	COLORREF White()
	{
		return RGB(255, 255, 255);
	}

	COLORREF Danger()
	{
		return RGB(225, 74, 83);
	}

	int Scale(HWND window, int value)
	{
		return MulDiv(
			value,
			static_cast<int>(GetWindowDpi(window)),
			96);
	}

	bool CreateFont(
		CWnd* window,
		CFont& font,
		int pointSize,
		int weight)
	{
		if (font.GetSafeHandle() != NULL) {
			font.DeleteObject();
		}

		LOGFONT value = {};
		const HWND handle = window != NULL
			? window->GetSafeHwnd()
			: NULL;
		value.lfHeight = -MulDiv(pointSize, Scale(handle, 96), 72);
		value.lfWeight = weight;
		value.lfQuality = CLEARTYPE_QUALITY;
		StringCchCopy(
			value.lfFaceName,
			_countof(value.lfFaceName),
			_T("Segoe UI"));
		return font.CreateFontIndirect(&value) != FALSE;
	}

	void ApplyFontToChildren(CWnd* window, CFont* font)
	{
		if (window == NULL || font == NULL ||
			window->GetSafeHwnd() == NULL ||
			font->GetSafeHandle() == NULL) {
			return;
		}

		window->SetFont(font, TRUE);
		EnumChildWindows(
			window->GetSafeHwnd(),
			ApplyFontCallback,
			reinterpret_cast<LPARAM>(font->GetSafeHandle()));
	}

	void PrepareControls(CWnd* window)
	{
		if (window == NULL || window->GetSafeHwnd() == NULL) {
			return;
		}
		EnumChildWindows(
			window->GetSafeHwnd(),
			PrepareControlCallback,
			0);
	}

	void DrawBrandLogo(
		CDC& dc,
		const CRect& bounds,
		CFont* wordmarkFont)
	{
		if (bounds.IsRectEmpty()) {
			return;
		}

		const int markWidth = (std::min)(bounds.Height(), bounds.Width() / 4);
		const int inset = (std::max)(1, markWidth / 8);
		POINT mark[6] = {
			{ bounds.left, bounds.top },
			{ bounds.left + markWidth - inset, bounds.top },
			{ bounds.left + markWidth - inset, bounds.top + markWidth / 2 },
			{ bounds.left + markWidth / 2, bounds.top + markWidth / 2 },
			{ bounds.left + markWidth / 5, bounds.top + markWidth - inset },
			{ bounds.left, bounds.top + markWidth - inset }
		};

		CBrush brandBrush(Brand());
		CBrush* oldBrush = dc.SelectObject(&brandBrush);
		CPen brandPen(PS_SOLID, 1, Brand());
		CPen* oldPen = dc.SelectObject(&brandPen);
		dc.Polygon(mark, _countof(mark));
		dc.SelectObject(oldPen);
		dc.SelectObject(oldBrush);

		CFont localFont;
		CFont* font = wordmarkFont;
		if (font == NULL) {
			LOGFONT value = {};
			value.lfHeight = -(std::max)(12, bounds.Height() * 3 / 5);
			value.lfWeight = FW_BOLD;
			value.lfQuality = CLEARTYPE_QUALITY;
			StringCchCopy(
				value.lfFaceName,
				_countof(value.lfFaceName),
				_T("Segoe UI"));
			localFont.CreateFontIndirect(&value);
			font = &localFont;
		}

		CFont* oldFont = dc.SelectObject(font);
		const int oldMode = dc.SetBkMode(TRANSPARENT);
		const COLORREF oldColor = dc.SetTextColor(Ink());
		CRect textBounds(bounds);
		textBounds.left += markWidth + (std::max)(4, markWidth / 5);
		dc.DrawText(
			_T("4PHONE"),
			textBounds,
			DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		dc.SetTextColor(oldColor);
		dc.SetBkMode(oldMode);
		dc.SelectObject(oldFont);
	}

	void ApplyWindowChrome(HWND window)
	{
		if (window == NULL) {
			return;
		}

		const DWORD rounded = 2;
		DwmSetWindowAttribute(
			window,
			static_cast<DWMWINDOWATTRIBUTE>(33),
			&rounded,
			sizeof(rounded));

		const COLORREF border = Line();
		DwmSetWindowAttribute(
			window,
			static_cast<DWMWINDOWATTRIBUTE>(34),
			&border,
			sizeof(border));
	}

	void ApplyListColors(CListCtrl* list)
	{
		if (list == NULL || list->GetSafeHwnd() == NULL) {
			return;
		}
		list->SetBkColor(White());
		list->SetTextBkColor(White());
		list->SetTextColor(Ink());
		SetWindowTheme(list->GetSafeHwnd(), L"Explorer", NULL);
	}
}

CFourPhoneButton::CFourPhoneButton()
	: variant(Secondary)
	, glyph(GlyphNone)
	, hot(false)
	, trackingMouse(false)
	, defaultButton(false)
{
}

BEGIN_MESSAGE_MAP(CFourPhoneButton, CButton)
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
	ON_MESSAGE(BM_SETSTYLE, OnSetButtonStyle)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

void CFourPhoneButton::SetVariant(Variant value)
{
	variant = value;
	if (GetSafeHwnd() != NULL) {
		Invalidate();
	}
}

void CFourPhoneButton::SetGlyph(Glyph value)
{
	glyph = value;
	if (GetSafeHwnd() != NULL) {
		Invalidate();
	}
}

void CFourPhoneButton::PreSubclassWindow()
{
	defaultButton =
		(GetStyle() & BS_TYPEMASK) == BS_DEFPUSHBUTTON;
	ModifyStyle(BS_TYPEMASK, BS_OWNERDRAW);
	CButton::PreSubclassWindow();
}

UINT CFourPhoneButton::OnGetDlgCode()
{
	UINT code = CButton::OnGetDlgCode();
	code &= ~(DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON);
	code |= defaultButton
		? DLGC_DEFPUSHBUTTON
		: DLGC_UNDEFPUSHBUTTON;
	return code;
}

void CFourPhoneButton::DrawItem(LPDRAWITEMSTRUCT drawItem)
{
	if (drawItem == NULL || drawItem->hDC == NULL) {
		return;
	}

	CDC target;
	target.Attach(drawItem->hDC);
	CRect bounds(drawItem->rcItem);

	CDC buffer;
	CBitmap bitmap;
	CBitmap* oldBitmap = NULL;
	CDC* dc = &target;
	if (buffer.CreateCompatibleDC(&target) &&
		bitmap.CreateCompatibleBitmap(
			&target,
			(std::max)(1, bounds.Width()),
			(std::max)(1, bounds.Height()))) {
		oldBitmap = buffer.SelectObject(&bitmap);
		bounds.OffsetRect(-bounds.left, -bounds.top);
		dc = &buffer;
	}

	const bool disabled = (drawItem->itemState & ODS_DISABLED) != 0;
	const bool pressed = (drawItem->itemState & ODS_SELECTED) != 0;
	const bool focused = (drawItem->itemState & ODS_FOCUS) != 0;

	COLORREF background = FourPhoneTheme::White();
	COLORREF border = FourPhoneTheme::Line();
	COLORREF text = FourPhoneTheme::Ink();

	switch (variant) {
	case Primary:
		background = pressed
			? FourPhoneTheme::BrandDark()
			: (hot ? RGB(8, 163, 113) : FourPhoneTheme::Brand());
		border = background;
		text = FourPhoneTheme::White();
		break;
	case Danger:
		background = pressed ? RGB(185, 45, 56) : FourPhoneTheme::Danger();
		border = background;
		text = FourPhoneTheme::White();
		break;
	case Ghost:
		background = hot ? FourPhoneTheme::Canvas() : FourPhoneTheme::White();
		border = background;
		text = FourPhoneTheme::InkSoft();
		break;
	case Caption:
		background = hot ? FourPhoneTheme::Canvas() : FourPhoneTheme::White();
		border = background;
		text = FourPhoneTheme::InkSoft();
		break;
	case CaptionClose:
		background = hot ? FourPhoneTheme::Danger() : FourPhoneTheme::White();
		border = background;
		text = hot ? FourPhoneTheme::White() : FourPhoneTheme::InkSoft();
		break;
	case Secondary:
	default:
		background = pressed
			? FourPhoneTheme::BrandSurface()
			: (hot ? RGB(248, 252, 250) : FourPhoneTheme::White());
		border = focused || defaultButton
			? FourPhoneTheme::Brand()
			: (hot ? RGB(183, 217, 202) : FourPhoneTheme::Line());
		text = hot ? FourPhoneTheme::BrandDark() : FourPhoneTheme::Ink();
		break;
	}

	if (disabled) {
		background = RGB(241, 243, 244);
		border = FourPhoneTheme::Line();
		text = FourPhoneTheme::Muted();
	}

	dc->FillSolidRect(bounds, FourPhoneTheme::White());
	CRect shape(bounds);
	shape.DeflateRect(1, 1);
	const int radius = FourPhoneTheme::Scale(
		GetSafeHwnd(),
		variant == Caption || variant == CaptionClose ? 6 : 8);
	DrawRoundRect(*dc, shape, background, border, radius);

	const int oldMode = dc->SetBkMode(TRANSPARENT);
	const COLORREF oldColor = dc->SetTextColor(text);
	CFont* currentFont = GetFont();
	CFont* oldFont = currentFont != NULL
		? dc->SelectObject(currentFont)
		: NULL;

	CString caption;
	GetWindowText(caption);
	CRect textBounds(shape);
	if (glyph == GlyphArrowRight && !caption.IsEmpty()) {
		textBounds.right -= FourPhoneTheme::Scale(GetSafeHwnd(), 22);
	}
	if (!caption.IsEmpty() && glyph != GlyphMenu &&
		glyph != GlyphMinimize && glyph != GlyphMaximize &&
		glyph != GlyphRestore && glyph != GlyphClose) {
		dc->DrawText(
			caption,
			textBounds,
			DT_CENTER | DT_VCENTER | DT_SINGLELINE |
			DT_END_ELLIPSIS | DT_NOPREFIX);
	}
	if (glyph != GlyphNone) {
		DrawGlyph(*dc, shape, text, (std::max)(1, FourPhoneTheme::Scale(
			GetSafeHwnd(),
			1)));
	}

	if (oldFont != NULL) {
		dc->SelectObject(oldFont);
	}
	dc->SetTextColor(oldColor);
	dc->SetBkMode(oldMode);

	if (dc == &buffer) {
		target.BitBlt(
			drawItem->rcItem.left,
			drawItem->rcItem.top,
			drawItem->rcItem.right - drawItem->rcItem.left,
			drawItem->rcItem.bottom - drawItem->rcItem.top,
			&buffer,
			0,
			0,
			SRCCOPY);
		buffer.SelectObject(oldBitmap);
	}
	target.Detach();
}

void CFourPhoneButton::DrawGlyph(
	CDC& dc,
	const CRect& bounds,
	COLORREF color,
	int penWidth)
{
	CPen pen(PS_SOLID, penWidth, color);
	CPen* oldPen = dc.SelectObject(&pen);
	const int centerX = bounds.CenterPoint().x;
	const int centerY = bounds.CenterPoint().y;
	const int half = (std::max)(3, FourPhoneTheme::Scale(GetSafeHwnd(), 6));

	switch (glyph) {
	case GlyphMenu:
		for (int row = -1; row <= 1; row++) {
			const int y = centerY + row * half / 2;
			dc.MoveTo(centerX - half, y);
			dc.LineTo(centerX + half + 1, y);
		}
		break;
	case GlyphMinimize:
		dc.MoveTo(centerX - half, centerY + half / 2);
		dc.LineTo(centerX + half + 1, centerY + half / 2);
		break;
	case GlyphMaximize:
		dc.MoveTo(centerX - half, centerY - half);
		dc.LineTo(centerX + half, centerY - half);
		dc.LineTo(centerX + half, centerY + half);
		dc.LineTo(centerX - half, centerY + half);
		dc.LineTo(centerX - half, centerY - half);
		break;
	case GlyphRestore:
		dc.MoveTo(centerX - half + 2, centerY - half);
		dc.LineTo(centerX + half, centerY - half);
		dc.LineTo(centerX + half, centerY + half - 2);
		dc.MoveTo(centerX - half, centerY - half + 2);
		dc.LineTo(centerX + half - 2, centerY - half + 2);
		dc.LineTo(centerX + half - 2, centerY + half);
		dc.LineTo(centerX - half, centerY + half);
		dc.LineTo(centerX - half, centerY - half + 2);
		break;
	case GlyphClose:
		dc.MoveTo(centerX - half, centerY - half);
		dc.LineTo(centerX + half + 1, centerY + half + 1);
		dc.MoveTo(centerX + half, centerY - half);
		dc.LineTo(centerX - half - 1, centerY + half + 1);
		break;
	case GlyphArrowRight:
	{
		const int x = bounds.right -
			FourPhoneTheme::Scale(GetSafeHwnd(), 14);
		dc.MoveTo(x - half / 2, centerY - half / 2);
		dc.LineTo(x, centerY);
		dc.LineTo(x - half / 2, centerY + half / 2);
		break;
	}
	case GlyphNone:
	default:
		break;
	}
	dc.SelectObject(oldPen);
}

void CFourPhoneButton::OnMouseMove(UINT flags, CPoint point)
{
	if (!trackingMouse) {
		TRACKMOUSEEVENT event = {};
		event.cbSize = sizeof(event);
		event.dwFlags = TME_LEAVE;
		event.hwndTrack = GetSafeHwnd();
		if (TrackMouseEvent(&event)) {
			trackingMouse = true;
		}
	}
	if (!hot) {
		hot = true;
		Invalidate();
	}
	CButton::OnMouseMove(flags, point);
}

LRESULT CFourPhoneButton::OnMouseLeave(WPARAM, LPARAM)
{
	trackingMouse = false;
	hot = false;
	Invalidate();
	return 0;
}

LRESULT CFourPhoneButton::OnSetButtonStyle(WPARAM style, LPARAM redraw)
{
	defaultButton =
		(static_cast<UINT>(style) & BS_TYPEMASK) ==
		BS_DEFPUSHBUTTON;
	if (redraw != FALSE) {
		Invalidate();
	}
	return 0;
}

void CFourPhoneButton::OnSetFocus(CWnd* oldWindow)
{
	CButton::OnSetFocus(oldWindow);
	Invalidate();
}

void CFourPhoneButton::OnKillFocus(CWnd* newWindow)
{
	CButton::OnKillFocus(newWindow);
	Invalidate();
}

CFourPhoneTabCtrl::CFourPhoneTabCtrl()
{
}

BEGIN_MESSAGE_MAP(CFourPhoneTabCtrl, CTabCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

void CFourPhoneTabCtrl::PreSubclassWindow()
{
	FourPhoneTheme::CreateFont(this, font, 9, FW_SEMIBOLD);
	FourPhoneTheme::CreateFont(this, fontSelected, 9, FW_BOLD);
	SetFont(&font);
	CTabCtrl::PreSubclassWindow();
}

void CFourPhoneTabCtrl::OnPaint()
{
	CPaintDC target(this);
	CRect client;
	GetClientRect(client);

	CDC buffer;
	CBitmap bitmap;
	CBitmap* oldBitmap = NULL;
	if (!buffer.CreateCompatibleDC(&target) ||
		!bitmap.CreateCompatibleBitmap(
			&target,
			(std::max)(1, client.Width()),
			(std::max)(1, client.Height()))) {
		return;
	}
	oldBitmap = buffer.SelectObject(&bitmap);
	buffer.FillSolidRect(client, FourPhoneTheme::White());

	CPen divider(PS_SOLID, 1, FourPhoneTheme::Line());
	CPen* oldPen = buffer.SelectObject(&divider);
	buffer.MoveTo(client.left, client.bottom - 1);
	buffer.LineTo(client.right, client.bottom - 1);
	buffer.SelectObject(oldPen);

	const int selected = GetCurSel();
	for (int index = 0; index < GetItemCount(); index++) {
		CRect item;
		if (!GetItemRect(index, item)) {
			continue;
		}
		item.DeflateRect(
			FourPhoneTheme::Scale(GetSafeHwnd(), 3),
			FourPhoneTheme::Scale(GetSafeHwnd(), 3));

		const bool active = index == selected;
		if (active) {
			DrawRoundRect(
				buffer,
				item,
				FourPhoneTheme::BrandSurface(),
				FourPhoneTheme::BrandSurface(),
				FourPhoneTheme::Scale(GetSafeHwnd(), 8));
		}

		TCHAR label[128] = {};
		TCITEM tab = {};
		tab.mask = TCIF_TEXT;
		tab.pszText = label;
		tab.cchTextMax = _countof(label);
		GetItem(index, &tab);

		CFont* oldFont = buffer.SelectObject(
			active ? &fontSelected : &font);
		const int oldMode = buffer.SetBkMode(TRANSPARENT);
		const COLORREF oldColor = buffer.SetTextColor(
			active
				? FourPhoneTheme::BrandDark()
				: FourPhoneTheme::Muted());
		buffer.DrawText(
			label,
			item,
			DT_CENTER | DT_VCENTER | DT_SINGLELINE |
			DT_END_ELLIPSIS | DT_NOPREFIX);
		buffer.SetTextColor(oldColor);
		buffer.SetBkMode(oldMode);
		buffer.SelectObject(oldFont);
	}

	target.BitBlt(
		0,
		0,
		client.Width(),
		client.Height(),
		&buffer,
		0,
		0,
		SRCCOPY);
	buffer.SelectObject(oldBitmap);
}

BOOL CFourPhoneTabCtrl::OnEraseBkgnd(CDC*)
{
	return TRUE;
}

void CFourPhoneTabCtrl::OnSetFocus(CWnd* oldWindow)
{
	CTabCtrl::OnSetFocus(oldWindow);
	Invalidate();
}

void CFourPhoneTabCtrl::OnKillFocus(CWnd* newWindow)
{
	CTabCtrl::OnKillFocus(newWindow);
	Invalidate();
}
