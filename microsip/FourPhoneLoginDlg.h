/*
 * 4phone provisioning support for MicroSIP.
 *
 * This file is distributed under the GNU General Public License,
 * version 2 or any later version.
 */

#pragma once

#include "FourPhoneApi.h"
#include "resource.h"

class CFourPhoneLoginDlg : public CDialog
{
public:
	enum { IDD = IDD_4PHONE_LOGIN };

	explicit CFourPhoneLoginDlg(CWnd* parent = NULL);
	virtual ~CFourPhoneLoginDlg();

	const FourPhoneSipConfig& GetSipConfig() const;
	CString GetSelectedExtensionId() const;

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void DoDataExchange(CDataExchange* dataExchange);

	DECLARE_MESSAGE_MAP()

private:
	enum Step
	{
		StepCredentials,
		StepTwoFactor,
		StepExtension
	};

	bool CompleteAuthentication(FourPhoneAuthTokens& tokens);
	bool LoadExtensions();
	bool LoadSelectedSipConfig();
	void ShowStep(Step nextStep);
	void SetBusy(bool busy);
	void SetStatus(const CString& message, bool isError);
	void SaveEmail(const CString& email);
	void SaveSelectedExtensionId(const CString& extensionId);
	CString LoadSavedEmail() const;
	void ClearPasswordControl();

	CFourPhoneApi api;
	Step step;
	bool requestInProgress;
	CString challengeToken;
	CString refreshToken;
	CString selectedExtensionId;
	FourPhoneSipConfig sipConfig;
	std::vector<FourPhoneExtension> extensions;
};
