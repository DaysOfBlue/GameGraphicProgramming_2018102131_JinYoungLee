#include "Game/Game.h"

namespace library
{
	/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
	  Method:   Game::Game
	  Summary:  Constructor
	  Args:     PCWSTR pszGameName
				  Name of the game
	  Modifies: [m_pszGameName, m_mainWindow, m_renderer].
	M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
	Game::Game(_In_ PCWSTR pszGameName) :
	
		m_pszGameName(pszGameName),
		m_mainWindow(std::make_unique<MainWindow>()),
		m_renderer(std::make_unique<Renderer>())
	{}

	/* M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
	  Method:   Game::Initialize
	  Summary:  Initializes the components of the game
	  Args:     HINSTANCE hInstance
				  Handle to the instance
				INT nCmdShow
				  Is a flag that says whether the main application window
				  will be minimized, maximized, or shown normally
	  Modifies: [m_mainWindow, m_renderer].
	  Returns:  HRESULT
				Status code
	M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M----M*/
	HRESULT Game::Initialize(_In_ HINSTANCE hInstance, _In_ INT nCmdShow) 
	{
		if (FAILED(m_mainWindow->Initialize(hInstance, nCmdShow, m_pszGameName)))
		{
			return 0;
		}

		HWND hWnd = m_mainWindow->GetWindow();

		if (FAILED(m_renderer->Initialize(hWnd)))
		{
			return 0;
		}
		

		return S_OK;
	}

	/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
	  Method:   Game::Run
	  Summary:  Runs the game loop
	  Returns:  INT
				  Status code to return to the operating system
	M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/

	INT Game::Run() 
	{
		LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
		LARGE_INTEGER Frequency;
		FLOAT ElapsedSeconds;
		MSG msg = {0};

		QueryPerformanceCounter(&StartingTime);

		while (WM_QUIT != msg.message) {
			
			
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				
			}
			else
			{
				QueryPerformanceFrequency(&Frequency);
				QueryPerformanceCounter(&EndingTime);
				ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
				ElapsedSeconds = ElapsedMicroseconds.QuadPart / (FLOAT)Frequency.QuadPart;
				// HandleInput
				m_renderer->HandleInput(m_mainWindow->GetDirections(), m_mainWindow->GetMouseRelativeMovement(),ElapsedSeconds);
				m_mainWindow->ResetMouseMovement();
				//Update
				m_renderer->Update(ElapsedSeconds);
				QueryPerformanceCounter(&StartingTime);
				//Draw
				m_renderer -> Render();
				
			}
			
			
		}

		return static_cast<INT>(msg.wParam);
	}

	/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
	  Method:   Game::GetGameName
	  Summary:  Returns the name of the game
	  Returns:  PCWSTR
				  Name of the game
	M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
	PCWSTR Game::GetGameName() const 
	{
		return m_pszGameName;
	}

	/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
	 Method:   Game::GetWindow
	 Summary:  Returns the main window
	 Returns:  std::unique_ptr<MainWindow>&
				 The main window
   M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
   
	std::unique_ptr<MainWindow>& Game::GetWindow() 
	{
		return m_mainWindow;
	}

   /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
	 Method:   Game::GetRenderer
	 Summary:  Returns the renderer
	 Returns:  std::unique_ptr<Renderer>&
				 The renderer
   M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
	std::unique_ptr<Renderer>& Game::GetRenderer()
	{
		return m_renderer;
	}
}