#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <useless-shit-lowkey/coustom.h>
#include "../settings/settings.h"
#include <imgui_impl_dx11.h>
#include <DrawMenu.h>
#include "../Game/Gameloop/Gameloop.h"
#include "../Driver/communication.h"
#include <thread>
#include <atomic>

extern bool ColorsApplied;

static std::atomic<bool> g_readThreadRunning{false};
static std::thread g_readThread;

void ReadThreadFunc() {
	int camWriteIdx = 0;
	int readCooldown = 0;
	constexpr int ENTITY_READ_INTERVAL = 1;

	auto publishCamera = [&]() {
		g_cameraBuffer[camWriteIdx] = read_camera;
		g_cameraFrontIdx.store(camWriteIdx, std::memory_order_release);
		camWriteIdx = 1 - camWriteIdx;
	};

	while (g_readThreadRunning) {
		try {
			if (base_address && g_driver && g_driver->is_attached()) {
				if (LocalPtrs::Gworld) {
					UpdateCamera(LocalPtrs::Gworld);
					publishCamera();
				}
				if (readCooldown <= 0) {
					ActorRead();
					publishCamera();
					readCooldown = ENTITY_READ_INTERVAL;
				} else {
					readCooldown--;
				}
			}
		} catch (...) {

		}
		Sleep(1);
	}
}

DWORD GetProcessIdByName(const std::wstring& exeName) {
    std::wstring target = exeName;
    std::transform(target.begin(), target.end(), target.begin(), ::towlower);

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            std::wstring current = pe.szExeFile;
            std::transform(current.begin(), current.end(), current.begin(), ::towlower);
            if (current == target) {
                DWORD pid = pe.th32ProcessID;
                CloseHandle(snap);
                return pid;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return 0;
}

bool IsProcessRunning(const std::wstring& exeName) {
    return GetProcessIdByName(exeName) != 0;
}

namespace Overlay {
    bool bExecuteOnce = true;
    bool bIsGameHijacked = false;
    struct DirectX11Interface {
        ID3D11Device* pDevice = nullptr;
        ID3D11DeviceContext* pDeviceContext = nullptr;
        IDXGISwapChain* pSwapChain = nullptr;
        ID3D11RenderTargetView* pRenderTargetView = nullptr;
        MSG Message = { NULL };
    }DirectX11;

    void BeginOverlay()
    {
		HWND hWnd = nullptr;
		int attempts = 0;
		while (!hWnd && attempts < 200) {
			hWnd = FindWindowA("Chrome_WidgetWin_1", "Discord Overlay");
			if (!hWnd) { Sleep(100); }
			attempts++;
		}
		if (!hWnd) {
			MessageBoxA(nullptr,
				"Discord Overlay window not found.\n"
				"Enable Discord in-game overlay and disable legacy overlay.",
				"hydra", MB_ICONERROR);
			return;
		}

		UINT bufW = (UINT)(CenterX > 0 ? CenterX * 2 : GetSystemMetrics(SM_CXSCREEN));
		UINT bufH = (UINT)(CenterY > 0 ? CenterY * 2 : GetSystemMetrics(SM_CYSCREEN));
		{
			RECT rc{};
			if (GetClientRect(hWnd, &rc)) {
				const UINT w = (UINT)(rc.right - rc.left);
				const UINT h = (UINT)(rc.bottom - rc.top);

				if (w >= 64 && h >= 64) {
					bufW = w;
					bufH = h;
				}
			}
		}
		if (bufW < 64) bufW = 64;
		if (bufH < 64) bufH = 64;
		CenterX = (int)(bufW / 2);
		CenterY = (int)(bufH / 2);
		ScreenCenterX = (float)CenterX;
		ScreenCenterY = (float)CenterY;

        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 2;
        sd.BufferDesc.Width = bufW;
        sd.BufferDesc.Height = bufH;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 0;
        sd.BufferDesc.RefreshRate.Denominator = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        sd.Flags = 0;

        D3D_FEATURE_LEVEL featureLevel;
		const D3D_FEATURE_LEVEL levels[] = {
			D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
		};
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            levels,
            3,
            D3D11_SDK_VERSION,
            &sd,
            &DirectX11.pSwapChain,
            &DirectX11.pDevice,
            &featureLevel,
            &DirectX11.pDeviceContext);

        if (FAILED(hr) || !DirectX11.pSwapChain || !DirectX11.pDevice) {
			char msg[256];
			sprintf_s(msg, "D3D11CreateDeviceAndSwapChain failed (hr=0x%08lX).\n"
				"Is Discord Overlay visible? Try focus Fortnite first.",
				(unsigned long)hr);
			MessageBoxA(nullptr, msg, "hydra", MB_ICONERROR);
			return;
        }

        ID3D11Texture2D* pBackBuffer = nullptr;
        if (FAILED(DirectX11.pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer))) || !pBackBuffer) {
			MessageBoxA(nullptr, "Failed to get swapchain back buffer.", "hydra", MB_ICONERROR);
			return;
		}
        DirectX11.pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &DirectX11.pRenderTargetView);
        pBackBuffer->Release();
		if (!DirectX11.pRenderTargetView) {
			MessageBoxA(nullptr, "Failed to create render target view.", "hydra", MB_ICONERROR);
			return;
		}
        DirectX11.pDeviceContext->OMSetRenderTargets(1, &DirectX11.pRenderTargetView, nullptr);

        D3D11_VIEWPORT vp{};
        vp.Width = (FLOAT)bufW;
        vp.Height = (FLOAT)bufH;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        DirectX11.pDeviceContext->RSSetViewports(1, &vp);

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(hWnd);
        ImGui_ImplDX11_Init(DirectX11.pDevice, DirectX11.pDeviceContext);

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
        io.WantCaptureKeyboard = false;

		{
			auto loadOrDefault = [&](const char* path, float size) -> ImFont* {
				ImFont* f = io.Fonts->AddFontFromFileTTF(path, size);
				return f ? f : io.Fonts->AddFontDefault();
			};
			Fonts::StandardFont = io.Fonts->AddFontDefault();
			Fonts::TextureFont  = loadOrDefault("C:\\Windows\\Fonts\\segoeuib.ttf", 40.0f);
			Fonts::Verdana      = loadOrDefault("C:\\Windows\\Fonts\\verdana.ttf", 14.0f);
			Fonts::FortniteFont = loadOrDefault("C:\\Windows\\Fonts\\arial.ttf", 14.0f);
			Fonts::GTAFont      = loadOrDefault("C:\\Windows\\Fonts\\impact.ttf", 14.0f);
			Fonts::Arial        = loadOrDefault("C:\\Windows\\Fonts\\arial.ttf", 14.0f);
			Fonts::Rajdhani     = loadOrDefault("C:\\Windows\\Fonts\\segoeui.ttf", 15.5f);
			Fonts::TahomaBold   = loadOrDefault("C:\\Windows\\Fonts\\tahomabd.ttf", 15.0f);
			Fonts::SegoeRegular = loadOrDefault("C:\\Windows\\Fonts\\segoeui.ttf", 13.0f);
			Fonts::SegoeSemi    = loadOrDefault("C:\\Windows\\Fonts\\seguisb.ttf", 15.0f);
			Fonts::SegoeBold    = loadOrDefault("C:\\Windows\\Fonts\\segoeuib.ttf", 16.0f);
			Fonts::DisplayBold  = loadOrDefault("C:\\Windows\\Fonts\\segoeuib.ttf", 28.0f);
			Fonts::NormalFont   = Fonts::SegoeRegular ? Fonts::SegoeRegular : Fonts::StandardFont;
			Fonts::HydraFont    = Fonts::SegoeBold ? Fonts::SegoeBold : Fonts::StandardFont;

			ImFont* fb = Fonts::StandardFont;
			auto nz = [&](ImFont*& p) { if (!p) p = fb; };
			nz(Fonts::TextureFont); nz(Fonts::Verdana); nz(Fonts::FortniteFont);
			nz(Fonts::GTAFont); nz(Fonts::Arial); nz(Fonts::Rajdhani);
			nz(Fonts::TahomaBold); nz(Fonts::SegoeRegular); nz(Fonts::SegoeSemi);
			nz(Fonts::SegoeBold); nz(Fonts::DisplayBold); nz(Fonts::NormalFont); nz(Fonts::HydraFont);
		}
		bExecuteOnce = false;

        {
            std::string saved = Config::LoadSelectedConfig();
            if (!saved.empty() && Config::Exists(saved)) {
                Config::Load(saved);
            } else {
                Config::EnsureDirectory();
                if (!Config::Exists("default")) {
                    Config::Save("default");
                }
                Config::Load("default");
            }
            ColorsApplied = true;
        }

        ZeroMemory(&DirectX11.Message, sizeof(MSG));

        bool deviceLost = false;
        bool fortniteAlive = true;
        DWORD lastFortCheck = 0;
        DWORD fortnitePid = GetProcessIdByName(L"FortniteClient-Win64-Shipping.exe");

        g_readThreadRunning = true;
        g_readThread = std::thread(ReadThreadFunc);

        while (DirectX11.Message.message != WM_QUIT) {

            if (PeekMessage(&DirectX11.Message, hWnd, 0, 0, PM_REMOVE))
            {
                if (DirectX11.Message.message == WM_KEYDOWN &&
                    (DirectX11.Message.wParam == VK_ESCAPE || DirectX11.Message.wParam == VK_RETURN)) {
                    continue;
                }
                TranslateMessage(&DirectX11.Message);
                DispatchMessage(&DirectX11.Message);
            }

            DWORD currentTime = GetTickCount();
            if (currentTime - lastFortCheck >= 1000) {
                lastFortCheck = currentTime;
                DWORD currentPid = GetProcessIdByName(L"FortniteClient-Win64-Shipping.exe");
                if (currentPid == 0 || currentPid != fortnitePid) {
                    base_address = 0;
                    fortniteAlive = false;
                    if (currentPid != 0) fortnitePid = currentPid;
                }
                if (!fortniteAlive) {
                    if (ReacquireGame()) {
                        fortniteAlive = true;
                        fortnitePid = GetProcessIdByName(L"FortniteClient-Win64-Shipping.exe");
                    }
                }
            }

            if (!deviceLost) {
                RECT rect;
                if (GetClientRect(hWnd, &rect)) {
                    UINT w = rect.right - rect.left;
                    UINT h = rect.bottom - rect.top;
                    if (w > 0 && h > 0 && (w != (UINT)(CenterX * 2) || h != (UINT)(CenterY * 2))) {
						CenterX = (int)(w / 2);
						CenterY = (int)(h / 2);
						ScreenCenterX = (float)CenterX;
						ScreenCenterY = (float)CenterY;
                        if (DirectX11.pRenderTargetView) {
                            DirectX11.pRenderTargetView->Release();
                            DirectX11.pRenderTargetView = nullptr;
                        }
                        HRESULT rhr = DirectX11.pSwapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
                        if (SUCCEEDED(rhr)) {
                            ID3D11Texture2D* pBB = nullptr;
                            if (SUCCEEDED(DirectX11.pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBB)))) {
                                DirectX11.pDevice->CreateRenderTargetView(pBB, nullptr, &DirectX11.pRenderTargetView);
                                pBB->Release();
                            }
                        }
                    }
                }
            }

            if (!deviceLost) {
                {
                    ImGuiIO& io = ImGui::GetIO();
                    io.MouseDown[0] = GetAsyncKeyState(VK_LBUTTON) ? true : false;
                }

                ImGui_ImplDX11_NewFrame();
                ImGui_ImplWin32_NewFrame();

                {
                    ImGuiIO& io = ImGui::GetIO();
                    POINT point;
                    GetCursorPos(&point);
                    io.MousePos.x = (float)point.x;
                    io.MousePos.y = (float)point.y;
                }

                ImGui::NewFrame();
                pDevice = DirectX11.pDevice;

                if (Options::RGBMode::bEnabled) {
                    Options::RGBMode::rgbTime += ImGui::GetIO().DeltaTime * Options::RGBMode::fSpeed;
                }

                if (LoadingScreen_IsActive()) {
                    DrawLoadingScreen();
                } else {
                    if (fortniteAlive) ActorRender();
                    Menu::RenderMenu(hWnd);
                }

                float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
                if (DirectX11.pRenderTargetView)
                    DirectX11.pDeviceContext->ClearRenderTargetView(DirectX11.pRenderTargetView, clearColor);

                ImGui::Render();
                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
                HRESULT presentResult = DirectX11.pSwapChain->Present(0, 0);
                if (presentResult == DXGI_ERROR_DEVICE_REMOVED || presentResult == DXGI_ERROR_DEVICE_RESET) {
                    deviceLost = true;
                }
            }

            if (deviceLost) {
                Sleep(100);

                ImGui_ImplDX11_InvalidateDeviceObjects();

                if (DirectX11.pRenderTargetView) {
                    DirectX11.pRenderTargetView->Release();
                    DirectX11.pRenderTargetView = nullptr;
                }
                if (DirectX11.pDeviceContext) {
                    DirectX11.pDeviceContext->Release();
                    DirectX11.pDeviceContext = nullptr;
                }
                if (DirectX11.pSwapChain) {
                    DirectX11.pSwapChain->Release();
                    DirectX11.pSwapChain = nullptr;
                }
                if (DirectX11.pDevice) {
                    DirectX11.pDevice->Release();
                    DirectX11.pDevice = nullptr;
                }

                DXGI_SWAP_CHAIN_DESC sd = {};
                sd.BufferCount = 2;
                sd.BufferDesc.Width = CenterX * 2;
                sd.BufferDesc.Height = CenterY * 2;
                sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                sd.BufferDesc.RefreshRate.Numerator = 0;
                sd.BufferDesc.RefreshRate.Denominator = 0;
                sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                sd.OutputWindow = hWnd;
                sd.SampleDesc.Count = 1;
                sd.SampleDesc.Quality = 0;
                sd.Windowed = TRUE;
                sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
                sd.Flags = 0;

                D3D_FEATURE_LEVEL featureLevel;
                HRESULT hr = D3D11CreateDeviceAndSwapChain(
                    nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
                    D3D11_SDK_VERSION, &sd,
                    &DirectX11.pSwapChain, &DirectX11.pDevice,
                    &featureLevel, &DirectX11.pDeviceContext);

                if (SUCCEEDED(hr)) {
                    ID3D11Texture2D* pBackBuffer = nullptr;
                    if (SUCCEEDED(DirectX11.pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)))) {
                        DirectX11.pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &DirectX11.pRenderTargetView);
                        pBackBuffer->Release();
                    }
                    DirectX11.pDeviceContext->OMSetRenderTargets(1, &DirectX11.pRenderTargetView, nullptr);

                    D3D11_VIEWPORT vp;
                    vp.Width = (FLOAT)(CenterX * 2);
                    vp.Height = (FLOAT)(CenterY * 2);
                    vp.MinDepth = 0.0f;
                    vp.MaxDepth = 1.0f;
                    vp.TopLeftX = 0;
                    vp.TopLeftY = 0;
                    DirectX11.pDeviceContext->RSSetViewports(1, &vp);

                    ImGui_ImplDX11_CreateDeviceObjects();
                    deviceLost = false;
                }
            }
        }
        g_readThreadRunning = false;
        if (g_readThread.joinable()) g_readThread.join();

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        if (DirectX11.pRenderTargetView != nullptr) {
            DirectX11.pRenderTargetView->Release();
        }
        if (DirectX11.pSwapChain != nullptr) {
            DirectX11.pSwapChain->Release();
        }
        if (DirectX11.pDeviceContext != nullptr) {
            DirectX11.pDeviceContext->Release();
        }
        if (DirectX11.pDevice != nullptr) {
            DirectX11.pDevice->Release();
        }
    }
}
