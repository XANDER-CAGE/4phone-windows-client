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
}

BEGIN_MESSAGE_MAP(CFourPhoneLoginDlg, CDialog)
END_MESSAGE_MAP()

BOOL CFourPhoneLoginDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(_T("4phone"));
	GetDlgItem(IDC_4PHONE_HEADING)->SetWindowText(
		_T("Вход в онлайн-АТС 4phone"));
	GetDlgItem(IDC_4PHONE_EMAIL_LABEL)->SetWindowText(_T("Email"));
	GetDlgItem(IDC_4PHONE_PASSWORD_LABEL)->SetWindowText(_T("Пароль"));
	GetDlgItem(IDC_4PHONE_CODE_LABEL)->SetWindowText(
		_T("Код 2FA или резервный код"));
	GetDlgItem(IDC_4PHONE_EXTENSION_LABEL)->SetWindowText(
		_T("Добавочный номер"));
	GetDlgItem(IDCANCEL)->SetWindowText(_T("Отмена"));

	CEdit* password = static_cast<CEdit*>(
		GetDlgItem(IDC_4PHONE_PASSWORD));
	password->SetPasswordChar(0x25CF);

	const CString savedEmail = LoadSavedEmail();
	GetDlgItem(IDC_4PHONE_EMAIL)->SetWindowText(savedEmail);

	ShowStep(StepCredentials);
	SetStatus(
		_T("Введите email и пароль от личного кабинета 4phone"),
		false);
	CenterWindow();

	if (!savedEmail.IsEmpty()) {
		GetDlgItem(IDC_4PHONE_PASSWORD)->SetFocus();
		return FALSE;
	}
	return TRUE;
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
			SetStatus(_T("Введите email и пароль"), true);
			return;
		}

		SetBusy(true);
		SetStatus(_T("Проверяем учетную запись..."), false);
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
				_T("Введите код из приложения-аутентификатора"),
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
			SetStatus(_T("Введите код подтверждения"), true);
			return;
		}

		SetBusy(true);
		SetStatus(_T("Проверяем код..."), false);
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
		SetStatus(_T("Получаем настройки телефонии..."), false);
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

	SetStatus(_T("Получаем доступные добавочные..."), false);
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
			_T("К пользователю не привязан активный добавочный номер"),
			true);
		return false;
	}

	if (extensions.size() == 1) {
		selectedExtensionId = extensions[0].id;
		SetStatus(_T("Применяем настройки телефонии..."), false);
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
	SetStatus(_T("Выберите добавочный для этого компьютера"), false);
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
			SetStatus(_T("Выберите добавочный номер"), true);
			return false;
		}
		selectedExtensionId = extensions[selectedIndex].id;
	}

	if (selectedExtensionId.IsEmpty()) {
		SetStatus(_T("Добавочный номер не выбран"), true);
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
		step == StepExtension ? _T("Подключить") : _T("Войти"));

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
			? _T("Подождите...")
			: (step == StepExtension ? _T("Подключить") : _T("Войти")));
	UpdateWindow();
}

void CFourPhoneLoginDlg::SetStatus(
	const CString& message,
	bool isError)
{
	UNREFERENCED_PARAMETER(isError);
	CWnd* status = GetDlgItem(IDC_4PHONE_STATUS);
	status->SetWindowText(message);
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
