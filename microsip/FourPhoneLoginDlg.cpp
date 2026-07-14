/*
 * 4phone provisioning support for MicroSIP.
 *
 * This file is distributed under the GNU General Public License,
 * version 2 or any later version.
 */

#include "stdafx.h"
#include "FourPhoneLoginDlg.h"

#include "FourPhoneCredentialStore.h"
#include "settings.h"
#include "langpack.h"

namespace
{
	const TCHAR* kProvisioningSection = _T("4phone");
	const TCHAR* kEmailKey = _T("email");
	const TCHAR* kExtensionIdKey = _T("extensionId");

	void SecureClear(CString& value)
	{
		if (value.IsEmpty()) {
			return;
		}
		const int length = value.GetLength();
		LPTSTR buffer = value.GetBuffer();
		SecureZeroMemory(buffer, length * sizeof(TCHAR));
		value.ReleaseBuffer(0);
	}
}

CFourPhoneLoginDlg::CFourPhoneLoginDlg(CWnd* parent)
	: CDialog(IDD, parent)
	, step(StepCredentials)
	, requestInProgress(false)
	, statusIsError(false)
{
}

CFourPhoneLoginDlg::~CFourPhoneLoginDlg()
{
	SecureClear(challengeToken);
	SecureClear(refreshToken);
	SecureClear(sipConfig.password);
}

const FourPhoneSipConfig& CFourPhoneLoginDlg::GetSipConfig() const
{
	return sipConfig;
}

CString CFourPhoneLoginDlg::GetSelectedExtensionId() const
{
	return selectedExtensionId;
}

void CFourPhoneLoginDlg::DoDataExchange(CDataExchange* dataExchange)
{
	CDialog::DoDataExchange(dataExchange);
	DDX_Control(dataExchange, IDOK, signInButton);
	DDX_Control(dataExchange, IDCANCEL, cancelButton);
}

BEGIN_MESSAGE_MAP(CFourPhoneLoginDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CFourPhoneLoginDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(_T("4phone"));
	FourPhoneTheme::ApplyWindowChrome(GetSafeHwnd());
	FourPhoneTheme::CreateFont(this, uiFont, 9);
	FourPhoneTheme::CreateFont(this, headingFont, 17, FW_SEMIBOLD);
	FourPhoneTheme::CreateFont(this, eyebrowFont, 8, FW_BOLD);
	FourPhoneTheme::ApplyFontToChildren(this, &uiFont);
	FourPhoneTheme::PrepareControls(this);
	if (langPack.rtl) {
		ModifyStyleEx(0, WS_EX_LAYOUTRTL);
	}
	if (whiteBrush.GetSafeHandle() == NULL) {
		whiteBrush.CreateSolidBrush(FourPhoneTheme::White());
	}
	GetDlgItem(IDC_4PHONE_HEADING)->SetFont(&headingFont);
	GetDlgItem(IDC_4PHONE_EYEBROW)->SetFont(&eyebrowFont);

	signInButton.SetVariant(CFourPhoneButton::Primary);
	signInButton.SetGlyph(CFourPhoneButton::GlyphArrowRight);
	cancelButton.SetVariant(CFourPhoneButton::Secondary);
	GetDlgItem(IDC_4PHONE_HEADING)->SetWindowText(
		Translate(_T("Sign in to 4phone")));
	GetDlgItem(IDC_4PHONE_EYEBROW)->SetWindowText(
		Translate(_T("Secure cloud telephony")));
	GetDlgItem(IDC_4PHONE_EMAIL_LABEL)->SetWindowText(_T("Email"));
	GetDlgItem(IDC_4PHONE_PASSWORD_LABEL)->SetWindowText(
		Translate(_T("Password")));
	GetDlgItem(IDC_4PHONE_CODE_LABEL)->SetWindowText(
		Translate(_T("2FA code or recovery code")));
	GetDlgItem(IDC_4PHONE_EXTENSION_LABEL)->SetWindowText(
		Translate(_T("Extension")));
	GetDlgItem(IDCANCEL)->SetWindowText(Translate(_T("Cancel")));

	CEdit* password = static_cast<CEdit*>(
		GetDlgItem(IDC_4PHONE_PASSWORD));
	password->SetPasswordChar(0x25CF);

	const CString savedEmail = LoadSavedEmail();
	GetDlgItem(IDC_4PHONE_EMAIL)->SetWindowText(savedEmail);

	ShowStep(StepCredentials);
	SetStatus(
		Translate(_T("Enter your 4phone email and password")),
		false);
	CenterWindow();

	if (!savedEmail.IsEmpty()) {
		GetDlgItem(IDC_4PHONE_PASSWORD)->SetFocus();
		return FALSE;
	}
	return TRUE;
}

void CFourPhoneLoginDlg::OnPaint()
{
	CPaintDC dc(this);
	CRect client;
	GetClientRect(client);
	dc.FillSolidRect(client, FourPhoneTheme::White());

	CRect top(client);
	CRect topUnits(0, 0, 0, 43);
	MapDialogRect(topUnits);
	top.bottom = topUnits.bottom;
	dc.FillSolidRect(top, RGB(248, 250, 249));

	CRect logoUnits(28, 14, 126, 37);
	MapDialogRect(logoUnits);
	FourPhoneTheme::DrawBrandLogo(dc, logoUnits);

	CPen divider(PS_SOLID, 1, FourPhoneTheme::Line());
	CPen* oldPen = dc.SelectObject(&divider);
	dc.MoveTo(top.left, top.bottom - 1);
	dc.LineTo(top.right, top.bottom - 1);
	dc.SelectObject(oldPen);

	CRect accentUnits(22, 48, 24, 82);
	MapDialogRect(accentUnits);
	dc.FillSolidRect(accentUnits, FourPhoneTheme::Brand());
}

BOOL CFourPhoneLoginDlg::OnEraseBkgnd(CDC*)
{
	return TRUE;
}

HBRUSH CFourPhoneLoginDlg::OnCtlColor(
	CDC* dc,
	CWnd* window,
	UINT controlColor)
{
	HBRUSH brush = CDialog::OnCtlColor(
		dc,
		window,
		controlColor);
	if (dc == NULL || window == NULL) {
		return brush;
	}

	if (controlColor == CTLCOLOR_EDIT ||
		controlColor == CTLCOLOR_LISTBOX) {
		dc->SetTextColor(FourPhoneTheme::Ink());
		dc->SetBkColor(FourPhoneTheme::White());
		return static_cast<HBRUSH>(whiteBrush.GetSafeHandle());
	}

	if (controlColor == CTLCOLOR_STATIC ||
		controlColor == CTLCOLOR_BTN ||
		controlColor == CTLCOLOR_DLG) {
		const int id = window->GetDlgCtrlID();
		if (id == IDC_4PHONE_HEADING) {
			dc->SetTextColor(FourPhoneTheme::Ink());
		}
		else if (id == IDC_4PHONE_EYEBROW) {
			dc->SetTextColor(FourPhoneTheme::BrandDark());
		}
		else if (id == IDC_4PHONE_STATUS) {
			dc->SetTextColor(
				statusIsError
					? FourPhoneTheme::Danger()
					: FourPhoneTheme::Muted());
		}
		else {
			dc->SetTextColor(FourPhoneTheme::InkSoft());
		}
		dc->SetBkMode(TRANSPARENT);
		return static_cast<HBRUSH>(whiteBrush.GetSafeHandle());
	}
	return brush;
}

void CFourPhoneLoginDlg::OnCancel()
{
	if (!requestInProgress) {
		CDialog::OnCancel();
	}
}

void CFourPhoneLoginDlg::OnOK()
{
	if (requestInProgress) {
		return;
	}

	if (step == StepCredentials) {
		CString email;
		CString password;
		GetDlgItem(IDC_4PHONE_EMAIL)->GetWindowText(email);
		GetDlgItem(IDC_4PHONE_PASSWORD)->GetWindowText(password);
		email.Trim();

		if (email.IsEmpty() || password.IsEmpty()) {
			SetStatus(Translate(_T("Enter email and password")), true);
			return;
		}

		SetBusy(true);
		SetStatus(Translate(_T("Checking your account...")), false);
		CWaitCursor waitCursor;

		FourPhoneLoginResponse response;
		const bool success = api.Login(email, password, response);
		SecureClear(password);
		ClearPasswordControl();

		if (!success) {
			SetStatus(api.GetLastError(), true);
			SetBusy(false);
			return;
		}

		SaveEmail(email);
		if (response.requiresTwoFactor) {
			SecureClear(challengeToken);
			challengeToken = response.challengeToken;
			SecureClear(response.challengeToken);
			ShowStep(StepTwoFactor);
			SetStatus(
				Translate(_T(
					"Enter the code from your authenticator app")),
				false);
			SetBusy(false);
			return;
		}

		if (!CompleteAuthentication(response.tokens)) {
			SetBusy(false);
			return;
		}
		if (::IsWindow(GetSafeHwnd())) {
			SetBusy(false);
		}
		return;
	}

	if (step == StepTwoFactor) {
		CString verificationValue;
		GetDlgItem(IDC_4PHONE_CODE)->GetWindowText(
			verificationValue);
		verificationValue.Trim();
		if (verificationValue.IsEmpty()) {
			SetStatus(Translate(_T("Enter verification code")), true);
			return;
		}

		SetBusy(true);
		SetStatus(Translate(_T("Checking code...")), false);
		CWaitCursor waitCursor;

		FourPhoneAuthTokens tokens;
		const bool success = api.VerifyTwoFactor(
			challengeToken,
			verificationValue,
			tokens);
		SecureClear(verificationValue);
		GetDlgItem(IDC_4PHONE_CODE)->SetWindowText(_T(""));

		if (!success) {
			SetStatus(api.GetLastError(), true);
			SetBusy(false);
			return;
		}
		SecureClear(challengeToken);
		if (!CompleteAuthentication(tokens)) {
			SetBusy(false);
			return;
		}
		if (::IsWindow(GetSafeHwnd())) {
			SetBusy(false);
		}
		return;
	}

	if (step == StepExtension) {
		SetBusy(true);
		SetStatus(Translate(_T("Loading telephony settings...")), false);
		CWaitCursor waitCursor;
		if (!LoadSelectedSipConfig()) {
			SetBusy(false);
		}
	}
}

bool CFourPhoneLoginDlg::CompleteAuthentication(
	FourPhoneAuthTokens& tokens)
{
	api.SetAccessToken(tokens.accessToken);
	SecureClear(refreshToken);
	refreshToken = tokens.refreshToken;
	SecureClear(tokens.accessToken);
	SecureClear(tokens.refreshToken);

	SetStatus(Translate(_T("Loading available extensions...")), false);
	return LoadExtensions();
}

bool CFourPhoneLoginDlg::LoadExtensions()
{
	if (!api.GetExtensions(extensions)) {
		SetStatus(api.GetLastError(), true);
		return false;
	}
	if (extensions.empty()) {
		SetStatus(
			Translate(_T(
				"No active extension is assigned to your user")),
			true);
		return false;
	}

	if (extensions.size() == 1) {
		selectedExtensionId = extensions[0].id;
		SetStatus(
			Translate(_T("Applying telephony settings...")),
			false);
		return LoadSelectedSipConfig();
	}

	CComboBox* extensionBox = static_cast<CComboBox*>(
		GetDlgItem(IDC_4PHONE_EXTENSION));
	extensionBox->ResetContent();
	for (size_t index = 0; index < extensions.size(); index++) {
		CString label = extensions[index].number;
		if (!extensions[index].displayName.IsEmpty()) {
			label.AppendFormat(
				_T(" - %s"),
				extensions[index].displayName.GetString());
		}
		extensionBox->AddString(label);
	}
	extensionBox->SetCurSel(0);

	ShowStep(StepExtension);
	SetStatus(
		Translate(_T("Select an extension for this computer")),
		false);
	return true;
}

bool CFourPhoneLoginDlg::LoadSelectedSipConfig()
{
	if (step == StepExtension) {
		CComboBox* extensionBox = static_cast<CComboBox*>(
			GetDlgItem(IDC_4PHONE_EXTENSION));
		const int selectedIndex = extensionBox->GetCurSel();
		if (selectedIndex < 0 ||
			selectedIndex >= static_cast<int>(extensions.size())) {
			SetStatus(Translate(_T("Select an extension")), true);
			return false;
		}
		selectedExtensionId = extensions[selectedIndex].id;
	}

	if (selectedExtensionId.IsEmpty()) {
		SetStatus(Translate(_T("No extension selected")), true);
		return false;
	}
	if (!api.GetSipConfig(selectedExtensionId, sipConfig)) {
		SetStatus(api.GetLastError(), true);
		return false;
	}

	CString storeError;
	if (!CFourPhoneCredentialStore::SaveRefreshToken(
		refreshToken,
		storeError)) {
		SetStatus(storeError, true);
		return false;
	}

	SaveSelectedExtensionId(selectedExtensionId);
	EndDialog(IDOK);
	return true;
}

void CFourPhoneLoginDlg::ShowStep(Step nextStep)
{
	step = nextStep;
	CString heading = Translate(_T("Sign in to 4phone"));
	if (step == StepTwoFactor) {
		heading = Translate(_T("Verify your identity"));
	}
	else if (step == StepExtension) {
		heading = Translate(_T("Choose an extension"));
	}
	GetDlgItem(IDC_4PHONE_HEADING)->SetWindowText(heading);

	const int credentialsVisibility =
		step == StepCredentials ? SW_SHOW : SW_HIDE;
	GetDlgItem(IDC_4PHONE_EMAIL_LABEL)->ShowWindow(
		credentialsVisibility);
	GetDlgItem(IDC_4PHONE_EMAIL)->ShowWindow(credentialsVisibility);
	GetDlgItem(IDC_4PHONE_PASSWORD_LABEL)->ShowWindow(
		credentialsVisibility);
	GetDlgItem(IDC_4PHONE_PASSWORD)->ShowWindow(
		credentialsVisibility);

	const int codeVisibility =
		step == StepTwoFactor ? SW_SHOW : SW_HIDE;
	GetDlgItem(IDC_4PHONE_CODE_LABEL)->ShowWindow(codeVisibility);
	GetDlgItem(IDC_4PHONE_CODE)->ShowWindow(codeVisibility);

	const int extensionVisibility =
		step == StepExtension ? SW_SHOW : SW_HIDE;
	GetDlgItem(IDC_4PHONE_EXTENSION_LABEL)->ShowWindow(
		extensionVisibility);
	GetDlgItem(IDC_4PHONE_EXTENSION)->ShowWindow(
		extensionVisibility);

	GetDlgItem(IDOK)->SetWindowText(
		step == StepExtension
			? Translate(_T("Connect"))
			: Translate(_T("Sign in")));

	if (step == StepTwoFactor) {
		GetDlgItem(IDC_4PHONE_CODE)->SetFocus();
	}
	else if (step == StepExtension) {
		GetDlgItem(IDC_4PHONE_EXTENSION)->SetFocus();
	}
}

void CFourPhoneLoginDlg::SetBusy(bool busy)
{
	requestInProgress = busy;
	GetDlgItem(IDOK)->EnableWindow(!busy);
	GetDlgItem(IDCANCEL)->EnableWindow(!busy);
	GetDlgItem(IDC_4PHONE_EMAIL)->EnableWindow(!busy);
	GetDlgItem(IDC_4PHONE_PASSWORD)->EnableWindow(!busy);
	GetDlgItem(IDC_4PHONE_CODE)->EnableWindow(!busy);
	GetDlgItem(IDC_4PHONE_EXTENSION)->EnableWindow(!busy);
	GetDlgItem(IDOK)->SetWindowText(
		busy
			? Translate(_T("Please wait..."))
			: (step == StepExtension
				? Translate(_T("Connect"))
				: Translate(_T("Sign in"))));
	UpdateWindow();
}

void CFourPhoneLoginDlg::SetStatus(
	const CString& message,
	bool isError)
{
	statusIsError = isError;
	CWnd* status = GetDlgItem(IDC_4PHONE_STATUS);
	status->SetWindowText(message);
	status->Invalidate();
	status->UpdateWindow();
}

void CFourPhoneLoginDlg::SaveEmail(const CString& email)
{
	WritePrivateProfileString(
		kProvisioningSection,
		kEmailKey,
		email,
		accountSettings.iniFile);
}

void CFourPhoneLoginDlg::SaveSelectedExtensionId(
	const CString& extensionId)
{
	WritePrivateProfileString(
		kProvisioningSection,
		kExtensionIdKey,
		extensionId,
		accountSettings.iniFile);
}

CString CFourPhoneLoginDlg::LoadSavedEmail() const
{
	CString email;
	LPTSTR buffer = email.GetBuffer(320);
	GetPrivateProfileString(
		kProvisioningSection,
		kEmailKey,
		_T(""),
		buffer,
		320,
		accountSettings.iniFile);
	email.ReleaseBuffer();
	return email;
}

void CFourPhoneLoginDlg::ClearPasswordControl()
{
	CEdit* password = static_cast<CEdit*>(
		GetDlgItem(IDC_4PHONE_PASSWORD));
	const int length = password->GetWindowTextLength();
	if (length > 0) {
		CString value;
		password->GetWindowText(value);
		SecureClear(value);
	}
	password->SetWindowText(_T(""));
}
