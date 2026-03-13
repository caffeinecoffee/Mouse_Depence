#pragma once

#include <vector>
#include <fstream>
#include <afxwin.h>
#include <afxcmn.h>
#include <windows.h>
#include <cmath>

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// -------------------------------
// 마우스 데이터 구조체
// -------------------------------
struct MouseData {
    int x = 0;
    int y = 0;
    int fakeX = -1;
    int fakeY = -1;

    bool    isClick = false;
    int     clickIndex = 0;
    DWORD   diffMs = 0;
    CString timeStr;
};

class CDefenderDlg : public CDialogEx
{
public:
    CDefenderDlg(CWnd* pParent = nullptr);
    void AddPointData(POINT pt, bool bClick);
    void LogClick(POINT pt, DWORD diff);
    bool IsCapturing() const { return m_running; }

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DEFENDERAPP_DIALOG };
#endif

    // ✅ 훅에서 “캡쳐 중일 때만” 기록하도록 체크용
    bool IsCapturing() const { return m_running; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL OnInitDialog();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnDestroy();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    DECLARE_MESSAGE_MAP()

private:
    // UI
    CStatic   m_picDesktop;
    CListCtrl m_listLog;
    CButton   m_btnReset, m_btnSave, m_btnOn, m_btnOff;
    HICON     m_hIcon = nullptr;
    std::ofstream m_outputFile;

    // ✅ data (락 필요)
    CRITICAL_SECTION m_csData{};
    std::vector<MouseData> m_mouseData;
    int m_clickCounter = 0;
    ULONGLONG m_lastFileTime = 0;
    ULONGLONG m_lastCaptureTime = 0;

    // thread / hook
    bool m_running = false;
    int  m_captureInterval = 100;
    CWinThread* m_pThread = nullptr;
    bool m_hookInstalled = false;

    bool m_randomMouseRunning = false;
    CWinThread* m_pRandomMouseThread = nullptr;

    // ===== DXGI Desktop Duplication =====
    Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3dContext;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> m_duplication;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_stagingTex;
    DXGI_OUTDUPL_DESC m_duplDesc{};

    // mirror 제거용 prev frame
    std::vector<BYTE> m_prevFrameBGRA;
    int m_prevW = 0;
    int m_prevH = 0;

private:
    // handlers
    afx_msg void OnBnClickedButtonOn();
    afx_msg void OnBnClickedButtonOff();
    afx_msg void OnBnClickedButtonReset();
    afx_msg void OnBnClickedButtonSave();

    // random mouse thread
    static UINT RandomMouseThread(LPVOID pParam);
    void StartRandomMouse();
    void StopRandomMouse();

    UINT_PTR m_timerId = 0;          // ✅ 추가
    ULONGLONG m_lastFakeTick = 0;    // ✅ fake 주기용

    // capture thread
    void StartCapture();
    void StopCapture();
    static UINT CaptureThread(LPVOID pParam);

    void InstallHook();
    void UninstallHook();

    // data

    void DrawMousePath(CDC* pDC, int destW, int destH, const std::vector<MouseData>& snapshot);
    void InsertLogItem(int x, int y, unsigned diff, CString timeStr);

    // DXGI functions
    bool InitDuplication();
    void UninitDuplication();
    bool CaptureDesktopBGRA(std::vector<BYTE>& outBGRA, int& outW, int& outH);
    bool MaskSelfWindowFromFrame(std::vector<BYTE>& bgra, int w, int h);
    void RenderFrame();
};
