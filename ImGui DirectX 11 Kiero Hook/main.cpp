#include "includes.h"
#include "config.hpp"
#include "visual.hpp"
#include <chrono>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Config g_config;
Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;
int visuals_subtab = 0;
bool g_menu_open = false;

void InitImGui() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (GetAsyncKeyState(VK_INSERT) & 1) {
        g_menu_open = !g_menu_open;
        ImGui::GetIO().MouseDrawCursor = g_menu_open;
        ShowCursor(g_menu_open);
        if (!g_menu_open) ClipCursor(NULL);
    }

    if (g_menu_open) {
        ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
        if (msg == WM_INPUT || msg == WM_MOUSEMOVE || msg == WM_LBUTTONDOWN ||
            msg == WM_RBUTTONDOWN || msg == WM_MOUSEWHEEL)
            return TRUE;
    }
    return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

bool init = false;
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (g_config.team.glow)
        Visuals::ApplyTeamGlow();

    if (g_config.enemy.glow)
        Visuals::ApplyEnemyGlow();
    
    if (g_config.anti_flash)
        Visuals::AntiFlash();

    if (!init) {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice))) {
            pDevice->GetImmediateContext(&pContext);
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            window = sd.OutputWindow;
            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
            pBackBuffer->Release();
            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
            InitImGui();
            init = true;
        }
        else
            return oPresent(pSwapChain, SyncInterval, Flags);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (g_menu_open) {
        ImGui::SetNextWindowSize(ImVec2(600, 400));
        ImGui::Begin("Simple Menu", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        static int current_tab = 0;
        ImGui::BeginChild("##tabs", ImVec2(120, 0), true);
        if (ImGui::Button("Visuals", ImVec2(110, 30))) current_tab = 0;
        if (ImGui::Button("Movement", ImVec2(110, 30))) current_tab = 1;
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("##content", ImVec2(0, 0), true);

        if (current_tab == 0) {
            ImGui::BeginChild("##visuals_subtabs", ImVec2(0, 30), false, ImGuiWindowFlags_NoScrollbar);
            if (ImGui::Button("Team")) visuals_subtab = 0;
            ImGui::SameLine();
            if (ImGui::Button("Enemy")) visuals_subtab = 1;
            ImGui::SameLine();
            if (ImGui::Button("Local")) visuals_subtab = 2;
            ImGui::EndChild();

            ImGui::Separator();

            if (visuals_subtab == 0) {
                ImGui::Checkbox("Glow", &g_config.team.glow);
                if (ImGui::IsItemClicked(1)) ImGui::OpenPopup("TeamGlowColor");
                if (ImGui::BeginPopup("TeamGlowColor")) {
                    ImGui::ColorPicker4("##picker", (float*)&g_config.team.glow_color,
                        ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel);
                    ImGui::EndPopup();
                }
            }
            else if (visuals_subtab == 1) {
                ImGui::Checkbox("Glow", &g_config.enemy.glow);
                if (ImGui::IsItemClicked(1)) ImGui::OpenPopup("EnemyGlowColor");
                if (ImGui::BeginPopup("EnemyGlowColor")) {
                    ImGui::ColorPicker4("##picker", (float*)&g_config.enemy.glow_color,
                        ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel);
                    ImGui::EndPopup();
                }
            }
            else if (visuals_subtab == 2) {
                ImGui::Checkbox("FOV Changer", &g_config.local.fov_changer);
                if (ImGui::IsItemClicked(1)) ImGui::OpenPopup("FOVSlider");
                if (ImGui::BeginPopup("FOVSlider")) {
                    ImGui::SliderInt("FOV", &g_config.local.fov, 0, 180);
                    ImGui::EndPopup();
                }
            }
        }
        else if (current_tab == 1) {
            ImGui::Checkbox("Bhop", &g_config.bhop);
            ImGui::Checkbox("Anti-flash", &g_config.anti_flash);
        }

        ImGui::EndChild();
        ImGui::End();
    }

    ImGui::Render();
    pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (g_config.local.fov_changer)
        Visuals::FovChange();

    return oPresent(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI MainThread(LPVOID lpReserved) {
    bool init_hook = false;
    do {
        if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success) {
            kiero::bind(8, (void**)&oPresent, hkPresent);
            init_hook = true;
        }
    } while (!init_hook);
    return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hMod);
        CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        kiero::shutdown();
        break;
    }
    return TRUE;
}