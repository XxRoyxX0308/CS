#ifndef APP3D_HPP
#define APP3D_HPP

// ============================================================================
//  App3D.hpp — PTSD 3D 框架範例應用程式標頭檔
// ============================================================================
//
//  本檔案定義了 App3D 類別，示範如何使用 PTSD 3D 框架的主要功能：
//  - 3D 攝影機（FPS 風格，支援鍵盤 WASD + 滑鼠）
//  - 場景圖（SceneGraph）管理 3D 節點
//  - 前向渲染器（ForwardRenderer）內建 Phong / PBR 混合管線
//  - 光源系統（方向光、點光源、聚光燈）
//  - 陰影映射（Shadow Mapping + PCF 軟陰影）
//  - ImGui 即時除錯面板
//
//  ※ 狀態機說明：
//     START  → 初始化場景（攝影機、燈光、模型）
//     UPDATE → 每幀更新（輸入處理、動畫、渲染、ImGui）
//     END    → 清理資源（目前由解構子自動處理）
// ============================================================================

#include "pch.hpp" // IWYU pragma: export

// --- 3D 核心模組 ---
#include "Core3D/Camera.hpp"         // 攝影機（透視 / 正交）
#include "Core3D/Model.hpp"          // 3D 模型（Assimp 載入或程序化產生）

// --- 渲染模組 ---
#include "Render/ForwardRenderer.hpp" // 前向渲染器（Shadow → Geometry → Skybox）

// --- 場景模組 ---
#include "Scene/Light.hpp"           // 方向光、點光源、聚光燈
#include "Scene/SceneGraph.hpp"      // 場景圖（管理所有 3D 節點 + 燈光 + 攝影機）
#include "Scene/SceneNode.hpp"       // 場景節點（位置、旋轉、縮放、父子階層）
#include "Scene/Skybox.hpp"          // 天空盒（Cubemap）

/**
 * @class App3D
 * @brief PTSD 3D 框架的完整範例應用程式
 *
 * 使用方式：
 * 1. 建立 App3D 實例
 * 2. 在主迴圈中根據 GetCurrentState() 呼叫 Start() / Update() / End()
 * 3. Start()  只會執行一次 —— 初始化攝影機、燈光、模型
 * 4. Update() 每幀執行 —— 處理輸入、更新場景、呼叫渲染器
 * 5. End()    程式結束時執行
 *
 * 按鍵說明：
 *  W / A / S / D    前後左右移動攝影機
 *  Q / E            上下移動攝影機
 *  滑鼠右鍵 + 拖曳  旋轉攝影機視角
 *  滑鼠滾輪          縮放（調整 FOV）
 *  TAB              開關 ImGui 除錯面板
 *  ESC              離開程式
 */
class App3D {
public:
    // === 應用程式狀態機 ===
    enum class State {
        START,   ///< 初始化狀態（僅執行一次）
        UPDATE,  ///< 主更新迴圈（每幀執行）
        END,     ///< 結束狀態（清理資源）
    };

    /// 取得目前狀態，供 main() 判斷該呼叫哪個方法
    State GetCurrentState() const { return m_CurrentState; }

    /// 初始化場景（攝影機、燈光、模型）
    void Start();

    /// 每幀更新（輸入 → 動畫 → 渲染 → ImGui）
    void Update();

    /// 結束清理
    void End();

private:
    State m_CurrentState = State::START; ///< 目前狀態，初始為 START

    // ── 場景圖 ──
    //  SceneGraph 是 3D 場景的容器，管理：
    //  - 根節點（Root）及其所有子節點
    //  - 攝影機
    //  - 光源列表（方向光 / 點光源 / 聚光燈）
    //  - 天空盒
    Scene::SceneGraph m_Scene;

    // ── 前向渲染器 ──
    //  ForwardRenderer 負責多 Pass 渲染管線：
    //  Pass 1: ShadowPass  — 從光源角度產生深度貼圖
    //  Pass 2: GeometryPass — 正常渲染幾何體（Phong 或 PBR）
    //  Pass 3: SkyboxPass  — 最後繪製天空盒
    Render::ForwardRenderer m_Renderer;

    // ── 場景物件 ──
    //  m_CubeModel — 一個程序化產生的立方體模型（可替換為 .obj / .fbx 模型）
    //  m_CubeNode  — 場景節點，將模型附加到場景圖中
    std::shared_ptr<Core3D::Model> m_CubeModel;
    std::shared_ptr<Scene::SceneNode> m_CubeNode;

    // ── 攝影機滑鼠狀態 ──
    float m_LastMouseX = 0.0f;   ///< 上一幀滑鼠 X 座標（用於計算偏移量）
    float m_LastMouseY = 0.0f;   ///< 上一幀滑鼠 Y 座標
    bool m_FirstMouse = true;    ///< 是否為首次捕捉滑鼠（避免初始跳動）
    bool m_CameraMode = true;    ///< 是否啟用攝影機控制模式

    // ── ImGui 除錯面板 ──
    bool m_ShowDebugPanel = true; ///< 是否顯示 ImGui 除錯面板
    bool m_UsePBR = true;         ///< 是否使用 PBR 渲染（false = Blinn-Phong）
    float m_AmbientStrength = 0.1f; ///< 環境光強度（尚未接入 shader）

    // ── 燈光動畫 ──
    float m_LightAngle = 0.0f; ///< 點光源繞場景旋轉的角度
};

#endif
