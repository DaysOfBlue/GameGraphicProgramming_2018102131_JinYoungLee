#include "Window/MainWindow.h"

namespace library
{
    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   MainWindow::Initialize

      Summary:  Initializes main window

      Args:     HINSTANCE hInstance
                  Handle to the instance
                INT nCmdShow
                    Is a flag that says whether the main application window
                    will be minimized, maximized, or shown normally
                PCWSTR pszWindowName
                    The window name

      Returns:  HRESULT
                  Status code
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    HRESULT MainWindow::Initialize(_In_ HINSTANCE hInstance, _In_ INT nCmdShow, _In_ PCWSTR pszWindowName) 
    {
        RECT rc = { 0, 0, 800, 600 };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;

        HRESULT hr = BaseWindow::initialize(hInstance, nCmdShow, pszWindowName, dwStyle);

        if (FAILED(hr))
            return hr;

        static bool didInitRawInput = false;
        if (!didInitRawInput)
        {
            RAWINPUTDEVICE rid =
            {
                .usUsagePage = 0x01,
                .usUsage = 0x02,
                .dwFlags = 0,
                .hwndTarget = nullptr
            };

            if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) return E_FAIL;
            didInitRawInput = true;
        }

        POINT p1, p2;

        if (!GetClientRect(m_hWnd, &rc))
            return HRESULT_FROM_WIN32(GetLastError());
        p1.x = rc.left;
        p1.y = rc.top;
        p2.x = rc.right;
        p2.y = rc.bottom;

        if (!ClientToScreen(m_hWnd, &p1)) return E_FAIL;
        if (!ClientToScreen(m_hWnd, &p2)) return E_FAIL;

        rc.left = p1.x;
        rc.top = p1.y;
        rc.right = p2.x;
        rc.bottom = p2.y;

        if (!ClipCursor(&rc)) 
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
            

        return S_OK;
    }
    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   MainWindow::GetWindowClassName

      Summary:  Returns the name of the window class

      Returns:  PCWSTR
                  Name of the window class
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/

    PCWSTR MainWindow::GetWindowClassName() const 
    {
        return m_pszWindowName;
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   MainWindow::HandleMessage

      Summary:  Handles the messages

      Args:     UINT uMessage
                  Message code
                WPARAM wParam
                    Additional data the pertains to the message
                LPARAM lParam
                    Additional data the pertains to the message

      Returns:  LRESULT
                  Integer value that your program returns to Windows
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    LRESULT MainWindow::HandleMessage(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
    {
        
        switch (uMsg)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_INPUT:

            UINT dataSize;

            GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));

            if (dataSize > 0) 
            {
                std::unique_ptr<BYTE[]> rawdata = std::make_unique<BYTE[]>(dataSize);
                if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, rawdata.get(), &dataSize, sizeof(RAWINPUTHEADER)) == dataSize) 
                {
                    RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(rawdata.get());
                    if (raw->header.dwType == RIM_TYPEMOUSE)
                    {
                        m_mouseRelativeMovement.X += raw->data.mouse.lLastX;
                        m_mouseRelativeMovement.Y += raw->data.mouse.lLastY;
                    }
                }
            }

            return DefWindowProc(m_hWnd, uMsg, wParam, lParam);

        case WM_KEYDOWN:

            switch (wParam) {

            case 0x57:
                m_directions.bFront = true;
                break;
            case 0x53:
                m_directions.bBack = true;
                break;
            case 0x41:
                m_directions.bLeft = true;
                break;
            case 0x44:
                m_directions.bRight = true;
                break;
            case VK_SHIFT:
                m_directions.bDown = true;
                break;
            case VK_SPACE:
                m_directions.bUp = true;
                break;
            }

            return 0;

        case WM_KEYUP:
            switch (wParam) {

            case 0x57:
                m_directions.bFront = false;
                break;
            case 0x53:
                m_directions.bBack = false;
                break;
            case 0x41:
                m_directions.bLeft = false;
                break;
            case 0x44:
                m_directions.bRight = false;
                break;
            case VK_SHIFT:
                m_directions.bDown = false;
                break;
            case VK_SPACE:
                m_directions.bUp = false;
            }
            return 0;

        default:
            
            return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
        }
        return TRUE;
    }
    
    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   MainWindow::GetDirections

      Summary:  Returns the keyboard direction input

      Returns:  const DirectionsInput&
                  Keyboard direction input
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/

    const DirectionsInput& MainWindow::GetDirections() const
    {
        return m_directions;
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   MainWindow::GetMouseRelativeMovement

      Summary:  Returns the mouse relative movement

      Returns:  const MouseRelativeMovement&
                  Mouse relative movement
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    const MouseRelativeMovement& MainWindow::GetMouseRelativeMovement() const
    {
        return m_mouseRelativeMovement;
    }
    
    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   MainWindow::ResetMouseMovement

      Summary:  Reset the mouse relative movement to zero
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/

    void MainWindow::ResetMouseMovement()
    {
        m_mouseRelativeMovement = {0, 0};
    }
    
}
