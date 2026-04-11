#include "visual.hpp"
#include "config.hpp"
#include "offsets.hpp"
#include "client_dll.hpp"
#include <Windows.h>

static uint32_t ImVec4ToD3DColor(const ImVec4& col) {
    return (uint8_t(col.w * 255.f) << 24) | (uint8_t(col.z * 255.f) << 16) |
        (uint8_t(col.y * 255.f) << 8) | uint8_t(col.x * 255.f);
}

static void ApplyGlow(uint64_t playerPawn, bool enable, uint32_t color) {
    uint64_t glowBase = playerPawn + cs2_dumper::schemas::client_dll::C_BaseModelEntity::m_Glow;
    uint64_t colorOverride = glowBase + cs2_dumper::schemas::client_dll::CGlowProperty::m_glowColorOverride;
    uint64_t glowEnable = glowBase + cs2_dumper::schemas::client_dll::CGlowProperty::m_bGlowing;

    if (enable) {
        *(uint32_t*)colorOverride = color;
        *(bool*)glowEnable = true;
    }
    else {
        *(bool*)glowEnable = false;
    }
}

void Visuals::ApplyTeamGlow() {
    uint64_t client = (uint64_t)GetModuleHandle("client.dll");
    uint64_t entityList = *(uint64_t*)(client + cs2_dumper::offsets::client_dll::dwEntityList);
    uint64_t localPawn = *(uint64_t*)(client + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (!client || !entityList || !localPawn) return;

    uint8_t localTeam = *(uint8_t*)(localPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
    uint32_t teamColor = ImVec4ToD3DColor(g_config.team.glow_color);

    for (int i = 0; i < 64; ++i) {
        uint64_t listEntry = *(uint64_t*)(entityList + 0x10);
        if (!listEntry) continue;

        uint64_t controller = *(uint64_t*)(listEntry + 0x70 * (i & 0x1FF));
        if (!controller) continue;

        uint32_t pawnHandle = *(uint32_t*)(controller + cs2_dumper::schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
        if (!pawnHandle) continue;

        uint64_t chunk = *(uint64_t*)(entityList + 8 * ((pawnHandle & 0x7FFF) >> 9) + 0x10);
        if (!chunk) continue;

        uint64_t pawn = *(uint64_t*)(chunk + 0x70 * (pawnHandle & 0x1FF));
        if (!pawn || pawn == localPawn) continue;

        int lifeState = *(int*)(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_lifeState);
        if (lifeState != 256) continue;

        uint8_t team = *(uint8_t*)(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
        if (team == localTeam) {
            ApplyGlow(pawn, g_config.team.glow, teamColor);
        }
    }
}

void Visuals::ApplyEnemyGlow() {
    uint64_t client = (uint64_t)GetModuleHandle("client.dll");
    uint64_t entityList = *(uint64_t*)(client + cs2_dumper::offsets::client_dll::dwEntityList);
    uint64_t localPawn = *(uint64_t*)(client + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (!client || !entityList || !localPawn) return;

    uint8_t localTeam = *(uint8_t*)(localPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
    uint32_t enemyColor = ImVec4ToD3DColor(g_config.enemy.glow_color);

    for (int i = 0; i < 64; ++i) {
        uint64_t listEntry = *(uint64_t*)(entityList + 0x10);
        if (!listEntry) continue;

        uint64_t controller = *(uint64_t*)(listEntry + 0x70 * (i & 0x1FF));
        if (!controller) continue;

        uint32_t pawnHandle = *(uint32_t*)(controller + cs2_dumper::schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
        if (!pawnHandle) continue;

        uint64_t chunk = *(uint64_t*)(entityList + 8 * ((pawnHandle & 0x7FFF) >> 9) + 0x10);
        if (!chunk) continue;

        uint64_t pawn = *(uint64_t*)(chunk + 0x70 * (pawnHandle & 0x1FF));
        if (!pawn || pawn == localPawn) continue;

        int lifeState = *(int*)(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_lifeState);
        if (lifeState != 256) continue;

        uint8_t team = *(uint8_t*)(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
        if (team != localTeam) {
            ApplyGlow(pawn, g_config.enemy.glow, enemyColor);
        }
    }
}

void Visuals::FovChange() {
    uint64_t client = (uint64_t)GetModuleHandle("client.dll");
    if (!client) return;

    uint64_t localPawn = *(uint64_t*)(client + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;
    if (!g_config.local.fov_changer) return;

    bool isScoped = *(bool*)(localPawn + cs2_dumper::schemas::client_dll::C_CSPlayerPawn::m_bIsScoped);
    if (isScoped) return;

    uint64_t cameraServices = *(uint64_t*)(localPawn + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_pCameraServices);
    if (!cameraServices) return;

    uint32_t fovValue = (uint32_t)g_config.local.fov;

    static uint32_t lastFOV = 0;
    static bool fovRateSet = false;

    if (!fovRateSet) {
        *(float*)(cameraServices + cs2_dumper::schemas::client_dll::CCSPlayerBase_CameraServices::m_flFOVRate) = 0.f;
        fovRateSet = true;
    }

    if (lastFOV != fovValue) {
        *(uint32_t*)(cameraServices + cs2_dumper::schemas::client_dll::CBasePlayerController::m_iDesiredFOV) = fovValue;
        *(float*)(cameraServices + cs2_dumper::schemas::client_dll::C_PointCamera::m_FOV) = fovValue;
        lastFOV = fovValue;
    }
}

void Visuals::AntiFlash() {
    uint64_t client = (uint64_t)GetModuleHandle("client.dll");
    if (!client) return;

    uint64_t localPawn = *(uint64_t*)(client + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;

    *(float*)(localPawn + cs2_dumper::schemas::client_dll::C_CSPlayerPawnBase::m_flFlashDuration) = 0.f;
}