#pragma once

#include <afxcmn.h>
#include <afxwin.h>

namespace FourPhoneTheme
{
	COLORREF Brand();
	COLORREF BrandDark();
	COLORREF BrandLight();
	COLORREF BrandSurface();
	COLORREF Ink();
	COLORREF InkSoft();
	COLORREF Muted();
	COLORREF Line();
	COLORREF Canvas();
	COLORREF White();
	COLORREF Danger();

	int Scale(HWND window, int value);
	bool CreateFont(
		CWnd* window,
		CFont& font,
		int pointSize,
		int weight = FW_NORMAL);
	void ApplyFontToChildren(CWnd* window, CFont* font);
	void PrepareControls(CWnd* window);
	void DrawBrandLogo(
		CDC& dc,
		const CRect& bounds,
		CFont* wordmarkFont = NULL);
	void ApplyWindowChrome(HWND window);
	void ApplyListColors(CListCtrl* list);
}

class CFourPhoneButton : public CButton
{
public:
	enum Variant
	{
		Primary,
		Secondary,
		Ghost,
		Danger,
		Caption,
		CaptionClose
	};

	enum Glyph
	{
		GlyphNone,
		GlyphMenu,
		GlyphMinimize,
		GlyphMaximize,
		GlyphRestore,
		GlyphClose,
		GlyphArrowRight
	};

	CFourPhoneButton();

	void SetVariant(Variant value);
	void SetGlyph(Glyph value);

protected:
	virtual void PreSubclassWindow();
	virtual void DrawItem(LPDRAWITEMSTRUCT drawItem);

	afx_msg void OnMouseMove(UINT flags, CPoint point);
	afx_msg LRESULT OnMouseLeave(WPARAM, LPARAM);
	afx_msg void OnSetFocus(CWnd* oldWindow);
	afx_msg void OnKillFocus(CWnd* newWindow);

	DECLARE_MESSAGE_MAP()

private:
	void DrawGlyph(
		CDC& dc,
		const CRect& bounds,
		COLORREF color,
		int penWidth);

	Variant variant;
	Glyph glyph;
	bool hot;
	bool trackingMouse;
};

class CFourPhoneTabCtrl : public CTabCtrl
{
public:
	CFourPhoneTabCtrl();

protected:
	virtual void PreSubclassWindow();

	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* dc);
	afx_msg void OnSetFocus(CWnd* oldWindow);
	afx_msg void OnKillFocus(CWnd* newWindow);

	DECLARE_MESSAGE_MAP()

private:
	CFont font;
	CFont fontSelected;
};
