# CS — FPS 射擊遊戲

基於 [PTSD](PTSD/) 3D 框架開發的第一人稱射擊遊戲，靈感來自 Counter-Strike。

## 功能

- **FPS 攝影機** — WASD 走路、空白鍵跳躍、滑鼠鎖定視角旋轉
- **棋盤格地板** — 程序化產生的 Checkerboard Pattern（100×100 m，50×50 格）
- **Phong 光照** — 方向光（太陽光）+ 陰影投射
- **Debug 面板** — 按 TAB 顯示攝影機資訊、FPS 等

## 建置需求

| 項目 | 版本 |
|------|------|
| CMake | ≥ 3.16 |
| C++ 標準 | C++17 |
| OpenGL | ≥ 4.1 |
| 作業系統 | Windows 10/11 |

> 所有第三方依賴（SDL2、GLEW、GLM、ImGui、Assimp 等）由 PTSD 的 CMake 自動下載。

## 建置指令

```bash
# 產生建置檔（首次會下載依賴，需要一些時間）
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

# 編譯
cmake --build build --target CS --config Debug
```

編譯完成後，執行檔位於 `build/` 目錄。

## 操作說明

| 按鍵 | 功能 |
|------|------|
| W / A / S / D | 前 / 左 / 後 / 右 移動 |
| Space | 跳躍 |
| 滑鼠移動 | 視角旋轉 |
| 滾輪 | 調整 FOV |
| TAB | 切換游標鎖定 / Debug 面板 |
| ESC | 退出遊戲 |

## 專案結構

```
CS/
├── CMakeLists.txt          # 根建置設定
├── config.json             # 視窗 & 框架設定
├── .gitignore
├── README.md
├── include/
│   └── App.hpp             # 遊戲主類別
├── src/
│   ├── main.cpp            # 進入點
│   └── App.cpp             # 遊戲邏輯
├── Resources/              # 遊戲資源（材質、模型等）
└── PTSD/                   # 3D 框架（子模組）
    ├── assets/             # Shader、字型、音效
    ├── include/            # 框架標頭檔
    ├── src/                # 框架原始碼
    └── cmake/              # 依賴管理
```

## License

見 [LICENSE](LICENSE)。

[Gun model source](https://sketchfab.com/TastyTony/models)
[Character model source](https://sketchfab.com/erhanmatur/collections/csgo-character-229341bc3fb84b4d931318208d9634e2)