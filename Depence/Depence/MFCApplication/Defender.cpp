#include "pch.h"
#include "framework.h"
#include "Defender.h"
#include "DefenderDlg.h"
#include <afxrich.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CDefenderApp, CWinApp)
    ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CDefenderApp::CDefenderApp()
{
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
}

CDefenderApp theApp;

BOOL CDefenderApp::InitInstance()
{
    CWinApp::InitInstance();
    AfxEnableControlContainer();
    AfxInitRichEdit2();

    CDefenderDlg dlg;
    m_pMainWnd = &dlg;
    INT_PTR nResponse = dlg.DoModal();
    return FALSE;
}
