#include "pch.h"
#include "framework.h"
#include "Defender.h"
#include "DefenderDlg.h"
#include "afxdialogex.h"
#include <cmath>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static HHOOK g_hMouseHook = nullptr;
static CDefenderDlg* g_pMainDlg = nullptr;
static ULONGLONG g_lastClickTime = 0;

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        PMSLLHOOKSTRUCT p = (PMSLLHOOKSTRUCT)lParam;

        // ✅ 캡쳐 중일 때만 기록 (Start 안 눌렀는데도 찍히는 문제 방지)
        if (g_pMainDlg && g_pMainDlg->IsCapturing())
        {
            /*if (wParam == WM_LBUTTONDOWN)
            {
                ULONGLONG now = GetTickCount64();
                DWORD diff = (g_lastClickTime == 0) ? 0 : (DWORD)(now - g_lastClickTime);
                g_lastClickTime = now;

                g_pMainDlg->LogClick(p->pt, diff);
                g_pMainDlg->AddPointData(p->pt, true);
            }*/
            if (wParam == WM_MOUSEMOVE)
            {
                // 이동도 찍고 싶으면 활성화
                g_pMainDlg->AddPointData(p->pt, false);
            }
        }
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}


BEGIN_MESSAGE_MAP(CDefenderDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_ERASEBKGND()
    ON_WM_DESTROY()
    ON_WM_TIMER()   // ✅ 추가
    ON_BN_CLICKED(IDC_BUTTON_ON, &CDefenderDlg::OnBnClickedButtonOn)
    ON_BN_CLICKED(IDC_BUTTON_OFF, &CDefenderDlg::OnBnClickedButtonOff)
    ON_BN_CLICKED(IDC_BUTTON_RESET, &CDefenderDlg::OnBnClickedButtonReset)
    ON_BN_CLICKED(IDC_BUTTON_SAVE, &CDefenderDlg::OnBnClickedButtonSave)
END_MESSAGE_MAP()

// -------------------------------
// 생성자
// -------------------------------
CDefenderDlg::CDefenderDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_DEFENDERAPP_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_outputFile.open("mouse_data_log.txt", std::ios::out);
}

void CDefenderDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_BUTTON_ON, m_btnOn);
    DDX_Control(pDX, IDC_BUTTON_OFF, m_btnOff);
    DDX_Control(pDX, IDC_PIC_MOUSE, m_picDesktop);
    DDX_Control(pDX, IDC_LIST_LOG, m_listLog);
    DDX_Control(pDX, IDC_BUTTON_RESET, m_btnReset);
    DDX_Control(pDX, IDC_BUTTON_SAVE, m_btnSave);
}

void CDefenderDlg::OnBnClickedButtonOn()
{
    StartCapture();
}

void CDefenderDlg::StartRandomMouse()
{
    if (!m_randomMouseRunning)
    {
        m_randomMouseRunning = true;
        m_pRandomMouseThread = AfxBeginThread(RandomMouseThread, this);
    }
}

UINT CDefenderDlg::RandomMouseThread(LPVOID pParam)
{
    CDefenderDlg* pDlg = (CDefenderDlg*)pParam;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    while (pDlg->m_randomMouseRunning)
    {
        int randX = rand() % screenW;
        int randY = rand() % screenH;
        //SetCursorPos(randX, randY);
        Sleep(100);
    }
    return 0;
}

void CDefenderDlg::OnBnClickedButtonOff()
{
    StopRandomMouse();
    StopCapture();
}

void CDefenderDlg::StopRandomMouse()
{
    if (m_randomMouseRunning)
    {
        m_randomMouseRunning = false;
        if (m_pRandomMouseThread)
        {
            WaitForSingleObject(m_pRandomMouseThread->m_hThread, INFINITE);
            m_pRandomMouseThread = nullptr;
        }
    }
}

BOOL CDefenderDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        if (pMsg->wParam == VK_SPACE)
        {
            StopRandomMouse();
            return TRUE;
        }
    }
    return CDialogEx::PreTranslateMessage(pMsg);
}

// -------------------------------
// OnInitDialog
// -------------------------------
BOOL CDefenderDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    InitializeCriticalSection(&m_csData);

    m_listLog.SetExtendedStyle(
        m_listLog.GetExtendedStyle()
        | LVS_EX_FULLROWSELECT
        | LVS_EX_GRIDLINES
    );

    m_listLog.InsertColumn(0, _T("X"), LVCFMT_LEFT, 60);
    m_listLog.InsertColumn(1, _T("Y"), LVCFMT_LEFT, 60);
    m_listLog.InsertColumn(2, _T("Time"), LVCFMT_LEFT, 120);

    CRect rcList;
    m_listLog.GetClientRect(rcList);
    int totalWidth = rcList.Width();
    m_listLog.SetColumnWidth(0, (int)(totalWidth * 0.3));
    m_listLog.SetColumnWidth(1, (int)(totalWidth * 0.3));
    m_listLog.SetColumnWidth(2, (int)(totalWidth * 0.4));

    InitDuplication();
    return TRUE;
}

void CDefenderDlg::OnDestroy()
{
    StopCapture();
    UninitDuplication();

    if (m_outputFile.is_open())
        m_outputFile.close();

    DeleteCriticalSection(&m_csData);

    CDialogEx::OnDestroy();
}


BOOL CDefenderDlg::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

void CDefenderDlg::OnPaint()
{
    CPaintDC dc(this);

    if (IsIconic())
    {
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;
        dc.DrawIcon(x, y, m_hIcon);
        return;
    }

    CRect rcClient;
    GetClientRect(&rcClient);
    dc.FillSolidRect(rcClient, RGB(240, 240, 240));
}

HCURSOR CDefenderDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

// -------------------------------
// StartCapture / StopCapture
// -------------------------------
void CDefenderDlg::StartCapture()
{
    if (m_running) return;

    m_running = true;
    m_lastFakeTick = 0;

    // ✅ UI 스레드 타이머로 캡쳐 루프 돌림
    m_timerId = SetTimer(1, m_captureInterval, nullptr);

    InstallHook(); // 클릭만 받고, move는 타이머에서 찍자
}

void CDefenderDlg::StopCapture()
{
    m_running = false;

    if (m_timerId)
    {
        KillTimer(m_timerId);
        m_timerId = 0;
    }

    UninstallHook();
}

void CDefenderDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent != 1 || !m_running)
    {
        CDialogEx::OnTimer(nIDEvent);
        return;
    }

    POINT pt{};
    if (::GetCursorPos(&pt))
    {
        // ✅ move 기록 (UI 스레드라 안전)
        AddPointData(pt, false);
    }

    // ✅ fake 주기 생성
    ULONGLONG nowTick = GetTickCount64();
    const ULONGLONG FAKE_INTERVAL_MS = 120;

    if (nowTick - m_lastFakeTick >= FAKE_INTERVAL_MS)
    {
        m_lastFakeTick = nowTick;

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);

        const int margin = 10;
        const int R_exclude = 180;
        const int D_min = 70;
        const int R_min = R_exclude + 30;
        const int R_max = 520;

        POINT fake = { -1, -1 };

        // ✅ 스냅샷 복사 없이도 UI 스레드라 직접 접근 OK
        for (int k = 0; k < 80; ++k)
        {
            POINT cand = SampleDonut(pt.x, pt.y, R_min, R_max, screenW, screenH, margin);
            if (!FarFromClick(cand.x, cand.y, pt.x, pt.y, R_exclude)) continue;
            if (!FarFromExistingFake(cand.x, cand.y, m_mouseData, D_min)) continue;
            fake = cand;
            break;
        }

        if (fake.x >= 0)
        {
            MouseData mdFake{};
            mdFake.x = pt.x;
            mdFake.y = pt.y;
            mdFake.fakeX = fake.x;
            mdFake.fakeY = fake.y;
            mdFake.isClick = false;

            m_mouseData.push_back(mdFake);

            // pipe 전송
            HANDLE hPipe = CreateFile(_T("\\\\.\\pipe\\GANPipe"),
                GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

            if (hPipe != INVALID_HANDLE_VALUE)
            {
                CString pipeMsg;
                pipeMsg.Format(_T("%d,%d\n"), fake.x, fake.y);
                DWORD dwWritten = 0;
                WriteFile(hPipe, (LPCTSTR)pipeMsg,
                    pipeMsg.GetLength() * sizeof(TCHAR),
                    &dwWritten, NULL);
                CloseHandle(hPipe);
            }
        }
    }

    // ✅ RenderFrame도 UI 스레드에서만 실행 (강종 방지)
    RenderFrame();

    CDialogEx::OnTimer(nIDEvent);
}


// -------------------------------
// InstallHook / UninstallHook
// -------------------------------
void CDefenderDlg::InstallHook()
{
    if (m_hookInstalled) return;

    g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, AfxGetInstanceHandle(), 0);
    if (g_hMouseHook)
    {
        m_hookInstalled = true;
        g_pMainDlg = this;
        g_lastClickTime = 0;
    }
}

void CDefenderDlg::UninstallHook()
{
    if (g_hMouseHook)
    {
        UnhookWindowsHookEx(g_hMouseHook);
        g_hMouseHook = nullptr;
    }
    m_hookInstalled = false;
    g_pMainDlg = nullptr;
}


// ===== helpers =====
static inline int ClampI(int v, int lo, int hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}
static inline double Dist2(int x1, int y1, int x2, int y2) {
    double dx = (double)x1 - x2;
    double dy = (double)y1 - y2;
    return dx * dx + dy * dy;
}

static bool FarFromExistingFake(int x, int y, const std::vector<MouseData>& data, int dMin)
{
    const double d2 = (double)dMin * dMin;
    for (const auto& md : data)
    {
        if (md.fakeX < 0 || md.fakeY < 0) continue;
        if (Dist2(x, y, md.fakeX, md.fakeY) < d2) return false;
    }
    return true;
}

static bool FarFromClick(int x, int y, int cx, int cy, int rExclude)
{
    return Dist2(x, y, cx, cy) >= (double)rExclude * rExclude;
}

static POINT SampleDonut(int cx, int cy, int rMin, int rMax, int screenW, int screenH, int margin)
{
    double t = ((double)rand() / RAND_MAX) * 6.283185307179586; // 2pi
    double u = (double)rand() / RAND_MAX;
    double r = sqrt((double)rMin * rMin + u * ((double)rMax * rMax - (double)rMin * rMin));

    int x = (int)lround(cx + r * cos(t));
    int y = (int)lround(cy + r * sin(t));

    x = ClampI(x, margin, screenW - margin);
    y = ClampI(y, margin, screenH - margin);

    return POINT{ x, y };
}

// -------------------------------
// ✅ CaptureThread (중괄호/흐름 완전 수정본)
// -------------------------------
// -------------------------------
// ✅ CaptureThread (방어자) - 전체 코드 (그대로 교체)
// - 매 루프: (1) 현재 커서 move 기록, (2) 주기적으로 fake 생성+전송, (3) RenderFrame
// -------------------------------
UINT CDefenderDlg::CaptureThread(LPVOID pParam)
{
    CDefenderDlg* pDlg = (CDefenderDlg*)pParam;

    // fake 생성 주기 제어용
    static ULONGLONG s_lastFakeTick = 0;

    while (pDlg->m_running)
    {
        // 1) 현재 커서 move 기록 (원하면 끌 수 있음)
        POINT pt{};
        if (::GetCursorPos(&pt))
        {
            pDlg->AddPointData(pt, false);  // move
        }

        // 2) 일정 주기마다 fake 좌표 생성 + pipe 전송
        ULONGLONG nowTick = GetTickCount64();
        const ULONGLONG FAKE_INTERVAL_MS = 120;   // 원하는 값으로 조절

        if (nowTick - s_lastFakeTick >= FAKE_INTERVAL_MS)
        {
            s_lastFakeTick = nowTick;

            int screenW = GetSystemMetrics(SM_CXSCREEN);
            int screenH = GetSystemMetrics(SM_CYSCREEN);

            const int margin = 10;
            const int R_exclude = 180;  // 실제 클릭/커서 주변 제외 반경
            const int D_min = 70;    // fake끼리 최소 간격
            const int R_min = R_exclude + 30;
            const int R_max = 520;

            POINT fake = { -1, -1 };

            // 후보를 여러 번 뽑아서 조건 만족하는 것 선택
            for (int k = 0; k < 80; ++k)
            {
                POINT cand = SampleDonut(pt.x, pt.y, R_min, R_max, screenW, screenH, margin);
                if (!FarFromClick(cand.x, cand.y, pt.x, pt.y, R_exclude)) continue;

                // ⚠️ pDlg->m_mouseData를 그대로 접근(동시성 민감하면 CS 추가 권장)
                if (!FarFromExistingFake(cand.x, cand.y, pDlg->m_mouseData, D_min)) continue;

                fake = cand;
                break;
            }

            if (fake.x >= 0)
            {
                // fake도 기록(선택)
                MouseData mdFake{};
                mdFake.x = pt.x;
                mdFake.y = pt.y;
                mdFake.isClick = false;
                mdFake.fakeX = fake.x;
                mdFake.fakeY = fake.y;

                pDlg->m_mouseData.push_back(mdFake);

                // pipe 전송
                HANDLE hPipe = CreateFile(
                    _T("\\\\.\\pipe\\GANPipe"),
                    GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL
                );

                if (hPipe != INVALID_HANDLE_VALUE)
                {
                    CString pipeMsg;
                    pipeMsg.Format(_T("%d,%d\n"), fake.x, fake.y);

                    DWORD dwWritten = 0;
                    WriteFile(hPipe,
                        (LPCTSTR)pipeMsg,
                        pipeMsg.GetLength() * sizeof(TCHAR),
                        &dwWritten,
                        NULL);

                    CloseHandle(hPipe);
                }
            }
        }

        // 3) ✅ 프레임 렌더는 항상 호출 (조건문 밖)
        pDlg->RenderFrame();

        // 4) 주기
        Sleep(pDlg->m_captureInterval);
    }

    return 0;
}


// -------------------------------
// AddPointData
// -------------------------------
void CDefenderDlg::AddPointData(POINT pt, bool bClick)
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);
    ULONGLONG currentFT = (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;

    DWORD diff = 0;
    if (m_lastFileTime != 0)
        diff = (DWORD)((currentFT - m_lastFileTime) / 10000ULL);
    m_lastFileTime = currentFT;

    CString timeStr;
    timeStr.Format(_T("%02d:%02d:%02d.%03d"),
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    MouseData md{};
    md.x = pt.x;
    md.y = pt.y;
    md.isClick = bClick;
    md.diffMs = diff;
    md.timeStr = timeStr;

    if (bClick)
        md.clickIndex = ++m_clickCounter;

    // ✅ 락
    EnterCriticalSection(&m_csData);
    m_mouseData.push_back(md);
    LeaveCriticalSection(&m_csData);

    InsertLogItem(pt.x, pt.y, diff, timeStr);

    if (m_outputFile.is_open())
    {
        m_outputFile
            << md.x << "\t"
            << md.y << "\t"
            << (LPCTSTR)timeStr << "\t"
            << diff << "\n";
    }
}


void CDefenderDlg::LogClick(POINT pt, DWORD diff)
{
}

// -------------------------------
// DrawMousePath
// -------------------------------
void CDefenderDlg::DrawMousePath(CDC* pDC, int destW, int destH, const std::vector<MouseData>& snapshot)
{
    if (snapshot.empty()) return;

    int minX = 0;
    int minY = 0;
    int maxX = GetSystemMetrics(SM_CXSCREEN);
    int maxY = GetSystemMetrics(SM_CYSCREEN);

    int w = maxX - minX; if (w <= 0) w = 1;
    int h = maxY - minY; if (h <= 0) h = 1;

    float scaleX = (float)destW / w;
    float scaleY = (float)destH / h;
    float scale = min(scaleX, scaleY);

    CPen penLine(PS_SOLID, 2, RGB(0, 0, 0));
    CPen penCircle(PS_SOLID, 1, RGB(255, 0, 0));
    CBrush brushCircle(RGB(255, 0, 0));

    auto transform = [&](int x, int y) {
        float tx = (x - minX) * scale;
        float ty = (y - minY) * scale;
        return CPoint((int)tx, (int)ty);
        };

    for (size_t i = 0; i < snapshot.size(); i++)
    {
        CPoint curPt = transform(snapshot[i].x, snapshot[i].y);

        if (i > 0)
        {
            CPoint prevPt = transform(snapshot[i - 1].x, snapshot[i - 1].y);
            CPen* pOldPen = pDC->SelectObject(&penLine);
            pDC->MoveTo(prevPt);
            pDC->LineTo(curPt);
            pDC->SelectObject(pOldPen);
        }

        if (snapshot[i].isClick)
        {
            CPen* pOldPen = pDC->SelectObject(&penCircle);
            CBrush* pOldBr = pDC->SelectObject(&brushCircle);

            int r = 4;
            pDC->Ellipse(curPt.x - r, curPt.y - r, curPt.x + r, curPt.y + r);

            pDC->SelectObject(pOldBr);
            pDC->SelectObject(pOldPen);

            CString label;
            label.Format(_T("%d"), snapshot[i].clickIndex);

            COLORREF oldColor = pDC->SetTextColor(RGB(255, 255, 255));
            int oldBkMode = pDC->SetBkMode(TRANSPARENT);
            pDC->TextOut(curPt.x + 6, curPt.y - 6, label);
            pDC->SetTextColor(oldColor);
            pDC->SetBkMode(oldBkMode);
        }
    }
}


void CDefenderDlg::InsertLogItem(int x, int y, unsigned diff, CString timeStr)
{
    int rowCount = m_listLog.GetItemCount();
    CString sx; sx.Format(_T("%d"), x);
    int row = m_listLog.InsertItem(rowCount, sx);

    CString sy; sy.Format(_T("%d"), y);
    m_listLog.SetItemText(row, 1, sy);

    m_listLog.SetItemText(row, 2, timeStr);
}

// -------------------------------
// Reset
// -------------------------------
void CDefenderDlg::OnBnClickedButtonReset()
{
    m_listLog.DeleteAllItems();
    m_mouseData.clear();
    m_clickCounter = 0;
    m_lastCaptureTime = 0;
    m_lastFileTime = 0;

    CDC* pDC = m_picDesktop.GetDC();
    if (pDC)
    {
        CRect rcPic;
        m_picDesktop.GetClientRect(&rcPic);
        pDC->FillSolidRect(&rcPic, RGB(255, 255, 255));
        m_picDesktop.ReleaseDC(pDC);
    }
}

// -------------------------------
// Save
// -------------------------------
void CDefenderDlg::OnBnClickedButtonSave()
{
    CStdioFile file;
    if (file.Open(_T("mouse_log_saved.csv"),
        CFile::modeCreate | CFile::modeWrite | CFile::typeText))
    {
        file.WriteString(_T("X,Y,Time,ms\n"));

        for (size_t i = 0; i < m_mouseData.size(); i++)
        {
            const MouseData& md = m_mouseData[i];
            CString line;
            CString fakeTimeStr = _T("=") + CString("\"") + md.timeStr + CString("\"");
            line.Format(_T("%d,%d,%s,%u\n"), md.x, md.y, (LPCTSTR)fakeTimeStr, md.diffMs);
            file.WriteString(line);
        }

        file.Close();
        AfxMessageBox(_T("CSV 파일로 저장되었습니다."));
    }
    else
    {
        AfxMessageBox(_T("CSV 파일 열기 실패"));
    }
}

// =======================================================
// ✅ DXGI Desktop Duplication (ComPtr 주소 전달 방식 수정)
// =======================================================
bool CDefenderDlg::InitDuplication()
{
    UninitDuplication();

    D3D_FEATURE_LEVEL fl;
    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr, 0,
        D3D11_SDK_VERSION,
        m_d3dDevice.GetAddressOf(),      // ✅ FIX
        &fl,
        m_d3dContext.GetAddressOf()      // ✅ FIX
    );
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    hr = m_d3dDevice.As(&dxgiDevice);
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    hr = dxgiDevice->GetAdapter(&adapter);
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDXGIOutput> output;
    hr = adapter->EnumOutputs(0, &output);
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDXGIOutput1> output1;
    hr = output.As(&output1);
    if (FAILED(hr)) return false;

    hr = output1->DuplicateOutput(m_d3dDevice.Get(), m_duplication.GetAddressOf()); // ✅ FIX
    if (FAILED(hr)) return false;

    m_duplication->GetDesc(&m_duplDesc);

    m_prevFrameBGRA.clear();
    m_prevW = m_prevH = 0;
    m_stagingTex.Reset();

    return true;
}

void CDefenderDlg::UninitDuplication()
{
    m_stagingTex.Reset();
    m_duplication.Reset();
    m_d3dContext.Reset();
    m_d3dDevice.Reset();

    m_prevFrameBGRA.clear();
    m_prevW = m_prevH = 0;
}

bool CDefenderDlg::CaptureDesktopBGRA(std::vector<BYTE>& outBGRA, int& outW, int& outH)
{
    if (!m_duplication) return false;

    DXGI_OUTDUPL_FRAME_INFO fi{};
    Microsoft::WRL::ComPtr<IDXGIResource> res;

    HRESULT hr = m_duplication->AcquireNextFrame(16, &fi, &res);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) return false;

    if (hr == DXGI_ERROR_ACCESS_LOST)
    {
        InitDuplication();
        return false;
    }
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    hr = res.As(&tex);
    if (FAILED(hr))
    {
        m_duplication->ReleaseFrame();
        return false;
    }

    D3D11_TEXTURE2D_DESC desc{};
    tex->GetDesc(&desc);

    outW = (int)desc.Width;
    outH = (int)desc.Height;

    // ✅ 크기 변경 시 staging 재생성 (현재는 없거나 크기가 다르면 다시 만든다)
    if (!m_stagingTex ||
        m_prevW != outW || m_prevH != outH)
    {
        D3D11_TEXTURE2D_DESC sd = desc;
        sd.BindFlags = 0;
        sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        sd.Usage = D3D11_USAGE_STAGING;
        sd.MiscFlags = 0;

        m_stagingTex.Reset();
        hr = m_d3dDevice->CreateTexture2D(&sd, nullptr, m_stagingTex.GetAddressOf());
        if (FAILED(hr))
        {
            m_duplication->ReleaseFrame();
            return false;
        }
    }

    m_d3dContext->CopyResource(m_stagingTex.Get(), tex.Get());

    D3D11_MAPPED_SUBRESOURCE map{};
    hr = m_d3dContext->Map(m_stagingTex.Get(), 0, D3D11_MAP_READ, 0, &map);
    if (FAILED(hr))
    {
        m_duplication->ReleaseFrame();
        return false;
    }

    outBGRA.resize((size_t)outW * outH * 4);

    BYTE* dst = outBGRA.data();
    BYTE* src = (BYTE*)map.pData;

    for (int y = 0; y < outH; ++y)
    {
        memcpy(dst + (size_t)y * outW * 4,
            src + (size_t)y * map.RowPitch,
            (size_t)outW * 4);
    }

    m_d3dContext->Unmap(m_stagingTex.Get(), 0);
    m_duplication->ReleaseFrame();

    return true;
}

bool CDefenderDlg::MaskSelfWindowFromFrame(std::vector<BYTE>& bgra, int w, int h)
{
    if (bgra.empty() || w <= 0 || h <= 0) return false;

    if (m_prevFrameBGRA.empty() || m_prevW != w || m_prevH != h)
    {
        m_prevFrameBGRA = bgra;
        m_prevW = w; m_prevH = h;
        return false; // 첫 프레임 스킵
    }

    RECT rcWnd{};
    if (!::GetWindowRect(GetSafeHwnd(), &rcWnd))
    {
        m_prevFrameBGRA = bgra;
        return true;
    }

    int left = max(0, rcWnd.left);
    int top = max(0, rcWnd.top);
    int right = min(w, rcWnd.right);
    int bottom = min(h, rcWnd.bottom);

    if (left < right && top < bottom)
    {
        for (int y = top; y < bottom; ++y)
        {
            BYTE* row = bgra.data() + (size_t)y * w * 4;
            BYTE* prevRow = m_prevFrameBGRA.data() + (size_t)y * w * 4;

            memcpy(row + (size_t)left * 4,
                prevRow + (size_t)left * 4,
                (size_t)(right - left) * 4);
        }
    }

    m_prevFrameBGRA = bgra;
    return true;
}

void CDefenderDlg::RenderFrame()
{
    HWND hPic = m_picDesktop.GetSafeHwnd();
    if (!hPic) return;

    CRect rcPic;
    ::GetClientRect(hPic, &rcPic);
    int picW = rcPic.Width();
    int picH = rcPic.Height();
    if (picW <= 0 || picH <= 0) return;

    std::vector<BYTE> bgra;
    int capW = 0, capH = 0;

    if (!CaptureDesktopBGRA(bgra, capW, capH))
        return;

    // ✅ 내 창(방어자 창) 영역을 이전 프레임으로 덮어 “거울 제거”
    if (!MaskSelfWindowFromFrame(bgra, capW, capH))
        return;

    // ✅ snapshot
    std::vector<MouseData> snapshot;
    EnterCriticalSection(&m_csData);
    snapshot = m_mouseData;
    LeaveCriticalSection(&m_csData);

    HDC hdcPic = ::GetDC(hPic);
    if (!hdcPic) return;

    HDC hdcMem = CreateCompatibleDC(hdcPic);
    HBITMAP hbm = CreateCompatibleBitmap(hdcPic, picW, picH);
    HGDIOBJ old = SelectObject(hdcMem, hbm);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = capW;
    bmi.bmiHeader.biHeight = -capH; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetStretchBltMode(hdcMem, HALFTONE);
    StretchDIBits(
        hdcMem,
        0, 0, picW, picH,
        0, 0, capW, capH,
        bgra.data(),
        &bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );

    CDC dcMem;
    dcMem.Attach(hdcMem);
    DrawMousePath(&dcMem, picW, picH, snapshot);
    dcMem.Detach();

    BitBlt(hdcPic, 0, 0, picW, picH, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, old);
    DeleteObject(hbm);
    DeleteDC(hdcMem);
    ::ReleaseDC(hPic, hdcPic);
}

