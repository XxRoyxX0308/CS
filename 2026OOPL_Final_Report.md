# 2026 OOPL Final Report

## 組別資訊

組別：19

組員：資工二 113590006 鄞永力

復刻遊戲：Counter-Strike (CS)

## 專案簡介

### 遊戲簡介

本專案以經典第一人稱射擊遊戲 Counter-Strike 為目標進行復刻，並在既有 PTSD Engine 的基礎上擴充為支援完整 3D 場景之 FPS 遊戲框架。遊戲包含玩家移動、武器射擊、碰撞偵測、多人連線、AI Bot、音效系統、地圖載入、導航網格以及伺服器同步機制等功能。

專案採用 Client-Server 架構，玩家可透過網路進行多人對戰，系統同時支援 AI Bot 自動尋路與戰鬥行為，使遊戲具備完整 FPS 的核心玩法。

本專案不僅重現 CS 的基本玩法，也實作了許多現代遊戲引擎常見的技術，例如：

- Host Authoritative Network
- Remote Player Interpolation
- NavMesh + A* Pathfinding
- Capsule Physics
- Raycast Hit Detection
- Scene Graph Rendering

### 組別分工

由於本組僅有一位成員，因此所有工作皆由本人負責。

負責內容包含：

- 專案開發與維護
- 程式架構設計
- 網路同步系統
- 遊戲邏輯實作
- AI Bot 系統
- 武器系統
- 場景管理
- 地圖載入
- 音效系統
- 玩家控制系統
- 碰撞偵測系統
- 除錯與效能優化
- 文件整理與報告撰寫

其中投入最多時間的部分為玩家與地圖碰撞系統的修正與調整，以解決玩家穿牆、卡牆、樓梯移動異常及地形邊界碰撞等問題。

## 遊戲介紹

### 遊戲規則

本遊戲以 Counter-Strike 為參考對象，玩家透過第一人稱視角進行戰鬥。

主要規則如下：

1. 玩家可於地圖中自由移動。
2. 玩家可使用武器進行攻擊。
3. 命中敵人後造成傷害。
4. 血量歸零時角色死亡。
5. 擊殺敵人後獲得 1 分與 500 遊戲幣。
6. 可用遊戲幣購買不同槍械。
7. 任一隊伍獲得獲得總分 20 分即獲勝並結束遊戲。

### 遊戲畫面

![遊戲畫面](https://github.com/XxRoyxX0308/CS/blob/main/assets/ingame.png?raw=true)

## 程式設計

## 程式架構

本專案採用模組化設計，將不同功能拆分為獨立子系統，降低耦合度並提升維護性。

```text
Application
│
├── App
│   ├── App
│   ├── AudioManager
│   ├── CombatManager
│   ├── GameManager
│   ├── InputManager
│   ├── NetworkController
│   ├── StateManager
│   ├── UIManager
│   └── VoiceChatManager
│
├── Core
│   ├── Constants
│   └── Types
│
├── Effects
│   └── BulletHole
│
├── Entity
│   ├── Character
│   ├── CharacterModel
│   ├── Player
│   ├── RemotePlayer
│   └── BotPlayer
│
├── Navigation
│   ├── NavMesh
│   └── PathFinder
│
├── Network
│   ├── Server
│   │   └── GameServer
│   ├── Client
│   │   ├── GameClient
│   │   └── Interpolation
│   ├── Discovery
│   │   └── LANDiscovery
│   ├── NetworkManager
│   ├── Packet
│   ├── Socket
│   └── Types
│
├── Physics
│   ├── CapsuleCast
│   ├── CollisionMesh
│   └── CollisionTypes
│
└── Weapon
    ├── Knives
    │   ├── CombatKnife
    │   └── VictorinoxKnife
    ├── Pistols
    │   ├── M1895
    │   └── SigScorpion
    ├── Rifles
    │   ├── AXL47
    │   └── HoneyBadger
    ├── Shotguns
    │   ├── DoubleDeuce
    │   └── MP153
    ├── SMGs
    │   ├── MP5K
    │   └── UZI
    ├── Snipers
    │   ├── AR30
    │   └── Axis2
    ├── RayCast
    ├── Weapon
    ├── WeaponDefs
    └── WeaponSpread
```

### Application Layer

Application 為整個系統的入口。

主要負責：

- 初始化引擎
- 建立遊戲視窗
- 載入資源
- 啟動各模組
- 控制主迴圈

所有系統皆由 Application 啟動與管理。

### Game Loop

遊戲核心採用固定更新流程：

```text
Input
 ↓
Game Logic
 ↓
Physics
 ↓
Network Sync
 ↓
Rendering
```

透過此方式維持所有模組同步運作。

### GameManager

GameManager 為整體遊戲控制中心。

負責：

- 場景切換
- 遊戲狀態管理
- 玩家生成
- Bot 管理
- 資源調度

GameManager 扮演協調者角色，負責串接各個系統。

### Entity System

Entity System 採用物件導向架構。

所有角色皆繼承自共同基底類別。

優點：

- 易於擴充
- 減少重複程式碼
- 提高維護性

### State Machine

角色與 AI 採用 FSM（Finite State Machine）。

常見狀態包含：

- Idle
- Move
- Attack
- Dead
- Search Target

FSM 能有效降低複雜邏輯造成的程式混亂。

### Network Architecture

採用 Host Authoritative 架構。

```text
Client
   │
   ▼
Server
   │
   ▼
All Clients
```

Server 負責：

- 權威判定
- 狀態同步
- 傷害計算
- 玩家資訊管理

Client 主要負責：

- 玩家輸入
- 畫面顯示
- 插值處理

此架構可避免作弊與狀態不同步問題。

### Remote Interpolation

由於網路封包存在延遲問題，因此 Remote Player 並非直接跳到同步位置，而是透過插值計算平滑移動。

優點：

- 降低瞬移感
- 提升遊戲體驗
- 改善網路延遲造成的畫面抖動

## 程式技術

### 1. PTSD Engine 3D 化

原始 PTSD Engine 為 2D Framework。

本專案將其擴充成可支援 3D FPS 遊戲之框架。

新增內容包含：

- 3D Camera
- Model Loading
- Scene Graph
- Mesh Rendering
- Lighting System
- 3D Transform

使引擎具備建立部分 3D 遊戲的能力。

### 2. Physics System

實作 FPS 所需之物理系統。

包含：

- Capsule Collision
- Wall Collision
- Ground Detection
- Gravity
- Sliding Movement

其中玩家碰撞系統為開發過程中最重要的修正項目。

曾遭遇：

- 穿牆
- 卡牆
- 地形邊緣抖動
- 樓梯碰撞異常

透過多次調整碰撞檢測與位移修正後才成功解決。

### 3. Raycast Weapon System

武器命中判定採用 Raycast。

流程如下：

```text
Fire
 ↓
Raycast
 ↓
Hit Detection
 ↓
Damage
```

優點：

- 高效率
- 適合 FPS
- 判定精準

### 4. Networking System

多人連線系統包含：

- GameServer
- GameClient
- Packet Serialization
- State Synchronization

同步內容：

- 玩家位置
- 玩家旋轉
- 動作狀態
- 血量資訊
- 射擊事件

### 5. NavMesh

AI 行走區域使用 NavMesh。

流程：

```text
Map
 ↓
NavMesh
 ↓
Path Nodes
 ↓
A*
```

NavMesh 可避免 AI 穿牆或走入非法區域。

### 6. A* Pathfinding

Bot 尋路採用 A*。

評估函數：

f(n)=g(n)+h(n)

特性：

- 尋路效率高
- 可找到近似最佳路徑
- 廣泛應用於遊戲 AI

### 7. Bot AI

AI 採用有限狀態機。

主要行為：

- 巡邏
- 搜尋敵人
- 追蹤目標
- 攻擊
- 死亡

搭配 NavMesh 與 A* 完成完整 AI 系統。

### 8. Audio

音效系統支援：

- 槍聲
- 腳步聲
- 場景音效

使遊戲更接近實際 FPS 體驗。

### 9. Decal System

射擊命中牆面時建立 Bullet Hole Decal。

功能包含：

- 子彈痕跡
- 命中特效
- 視覺回饋

提升遊戲真實感。

### 10. Asset Management

資源系統負責管理：

- Model
- Texture
- Audio

避免重複載入造成效能浪費。

---

## 使用到 AI/AI Agent 的部分

本專案開發過程中約 99% 程式碼由 GitHub Copilot 協助生成。

主要用途包含：

- 類別設計
- 函式實作
- 系統架構建立
- 網路模組開發
- AI 系統撰寫
- Rendering 系統開發

本人主要負責：

- 強迫 Copilot 做事
- 程式驗證
- 小部分 Bug 修正
- 玩家碰撞問題修復

實際開發過程中，Copilot 雖然能快速產生大量程式碼，但仍需人工理解與修正才能整合至完整專案。

## 結語

### 問題與解決方法

### 問題一：玩家穿牆

問題：

玩家可能在狹窄空間中被兩面牆壁擠出地圖。

解決方式：

改良碰撞檢測流程，增加位移修正與碰撞回推機制。

### 問題二：Bot 尋路失敗

問題：

AI 可能卡在障礙物附近。

解決方式：

重新調整 NavMesh 與路徑節點連結。

### 自評

| 項次 | 項目 | 完成 |
|------|------|------|
| 1 | 完成專案權限改為 public | V |
| 2 | 具有 debug 的功能 | V |
| 3 | 解決專案上所有 Memory Leak 的問題 | V |
| 4 | 報告中沒有任何錯字，以及沒有任何一項遺漏 | V |
| 5 | 報告至少保持基本的美感，人類可讀 | V |
| 6 | 成功奴役 Copilot | V |

### 心得

本次專案是我目前規模最大且最複雜的遊戲開發專案之一。

透過實作 FPS 遊戲，我接觸到許多過去較少深入研究的技術，例如多人連線同步、NavMesh、A* 尋路以及 3D 渲染架構。

雖然大部分程式碼由 Copilot 協助生成，但在實際整合時仍然需要大量理解與除錯工作。尤其是碰撞系統的修正耗費最多時間，許多問題無法單純依靠 AI 解決，必須透過反覆測試與分析才能找到真正原因。本專案讓我更加理解大型軟體系統如何拆分模組，以及遊戲引擎各子系統之間的互動方式。

### 貢獻比例

| 組員 | 貢獻比例 |
|------|---------|
| 鄞永力 | 100% |
