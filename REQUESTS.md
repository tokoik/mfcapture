# 作業指示および対応履歴

## 概要

本プロジェクトでは、Windows Media Foundation による低遅延キャプチャを導入し、CMake ベースの再現可能なビルド環境へ移行したうえで、機能を画像取得と GLSL 表示に整理した。その後、`calib`（旧名 `calib-wom-msmf`）が出力した内部パラメータを利用して OpenCV と OpenGL の二方式を比較できる歪み補正機能と、ArUco Marker の検出機能を追加し、macOS・Raspberry Pi・Android・OpenXR に対応している。現在の設計方針は [GEMINI.md](GEMINI.md) にまとめている。

## 作業履歴

### 1. `CamMf` のバックポート

- **指示**: `calib` の `CamMf` クラスを `mfcapture` へバックポートする。
- **対応**: Media Foundation Source Reader、MFT デコーダ、カラーコンバータによるカメラ入力を移植し、レイテンシ優先と全フレーム処理の切り替え、フォーマット列挙、遅延初期化、COM リソース管理を反映した。

### 2. CMake ビルドと依存ライブラリ管理

- **指示**: CMake でビルドできるようにし、ダウンロードしたライブラリを `libs` へ置く。既存の `libs` ジャンクションは削除する。
- **対応**: C++17 の `CMakeLists.txt` を整備し、OpenCV、GLFW、Dear ImGui、Native File Dialog Extended などを `libs` へ取得するようにした。実行に必要な DLL、構成ファイル、シェーダー、画像、フォントをビルド先へコピーする処理を追加した。

### 3. 較正機能と ChArUco Board 作成機能の削除

- **指示**: 較正機能を削除し、キャプチャした画像を GLSL で変形表示するだけにする。
- **対応**: マーカー検出、標本取得、較正計算、ChArUco Board 作成に関するクラス、UI、設定を削除し、入力、テクスチャ転送、GLSL 変換、画面表示へ責務を整理した。

### 4. ソーストップディレクトリの整理

- **対応**: CMake から生成できるプロジェクトファイルや古いビルド成果物をソース管理から外し、out-of-source build を前提に `.gitignore` を更新した。

### 5. 描画ループ内の `Framebuffer::resize()` の確認

- **質問**: 毎フレーム `framebuffer.resize(frame)` を呼ぶオーバーヘッドは大きくないか。
- **確認結果**: `resize()` はサイズが同じなら再確保しないため、通常のフレームではサイズの比較だけで済む。現在の呼び出し位置を維持した。

### 6. 較正ファイルによる歪み補正の追加

- **指示**: `calib` で作成した内部パラメータ JSON を「ファイル」メニューから読み込み、OpenCV と OpenGL の二方式で補正する。「なし」「OpenCV」「OpenGL」のラジオボタンを追加する。
- **対応**:
  - `Undistortion` クラスを追加し、必須項目と行列サイズを検証して成功時だけ現在値を置き換えるようにした。
  - OpenCV 方式はキャプチャ直後、GPU 転送前に `cv::remap()` を実行し、補正マップは入力サイズが変化した場合だけ再作成するようにした。
  - `undistortion.vert` と `undistortion.frag` による OpenGL 方式を実装した。
  - 較正値がない状態では OpenCV／OpenGL を選択できないようにした。

### 7. 較正ファイル読み込み時の例外修正

- **原因**: `cv::Mat loaded{ rows, columns, CV_64F };` が initializer-list コンストラクタと解釈され、3×1 行列に 3×3 の要素を書き込んでいた。
- **対応**: `cv::Mat loaded(rows, columns, CV_64F);` と丸括弧で構築するようにした。

### 8. 教材向けコメントと Doxygen の整備

- **対応**: 補正処理、シェーダー切り替え、uniform 設定、GLSL の座標変換へ目的を説明するコメントを追加し、`@param` の不一致や未知の Doxygen コマンドを修正した。`Doxyfile` が `libs`、`build`、`docs` を走査しないようにした。

### 9. プロジェクト文書の整備

- **対応**: `README.md`、`GEMINI.md`、`REQUESTS.md` を作成した。

### 10. メンバ変数の初期化位置の統一

- **対応**: 全クラスのメンバ初期値をクラス定義内のデフォルトメンバ初期化子へ集約した。

### 11. `const_cast` と `friend` の廃止

- **対応**: `Config` の `friend class Menu` を削除して `getSettings()`, `setSettings()`, `getPreferences()` 等の公開 API を追加し、`Menu` の `const_cast` を廃止した。

### 12. `calib` との命名規約・コメントの統一

- **対応**: 共通する変数名・関数名を `mfcapture` の命名に、コメントと Doxygen の表現を `calib` に統一し、教材資料（プレゼンテーション、ハンドブック）に C++ クラス設計・カプセル化方針を追記した。

### 13. GStreamer 関連コードの削除

- **対応**: `cv::CAP_GSTREAMER` と GStreamer パイプラインの処理を削除し、GStreamer を扱わない方針を明記した。

### 14. Raspberry Pi（`CamLibcam`）と Linux ARM（OpenGL ES 3.1）への対応

- **対応**: `CamLibcam` を導入し、`cv::CAP_V4L2` と `/sys/class/video4linux` によるデバイス列挙、CMake オプション `USE_GLES` / `USE_LIBCAMERA` を整備した。Linux 固有のビルドエラー（`pwd.h` 不足、GLES の `GL_BGR` 未定義等）を解消した。

### 15. `GgApp::OpenXR` への統合

- **対応**: `calib-openxr` と `GgApp` の OpenXR 実装を一致させ、MSVC Debug 構成での `openxr_loaderd.lib` のリンク不整合を解消した。

### 16. 歪み補正の 2 パス描画パイプライン

- **原因**: OpenCV 方式では歪み補正後に展開シェーダーを適用していたが、OpenGL 方式では展開シェーダーを使わず、画角やアスペクト比が一致していなかった。
- **対応**: 「第 1 パス: 歪み補正（なし／OpenCV／OpenGL）」と「第 2 パス: 共通の展開シェーダー」の 2 パス構成とし、中間フレームバッファ `undistortedFramebuffer` と `Menu::setupUndistortion()` を追加した。最終表示を実 Framebuffer サイズによる `framebuffer.draw()` に統一した。

### 17. `Camera` クラスの再設計と解説文書の移行

- **対応**:
  - `CamMf.md` と `CamLibcam.md` を `docs/` へ移し、参照を更新した（現在は各 `Cam*` クラスの解説を両プロジェクトの `docs/` に置いている）。
  - `Camera.h` を外部ライブラリに依存しない抽象インターフェースとし、NVI パターン、単一バッファ、`lockFrame()` によるゼロコピー転送、仮想関数による `dynamic_cast` の排除を導入した（`calib` と共通）。

### 18. 終了時の純粋仮想関数呼び出し例外の解消

- **原因**: `~Camera()` から `close()` を介して純粋仮想関数 `onClose()` を呼んでいた。
- **対応**: `~Camera()` を `default` とし、各派生クラスのデストラクタで `close()` を呼ぶようにした。

### 19. ArUco Marker 検出の追加

- **対応**: `Aruco` クラスを追加し、歪み補正パイプラインへ統合した。補正なしでは生画像に対して歪み係数を用いて姿勢を推定し、補正後の画像では歪み係数を空行列として座標軸を描画する。OpenGL 補正時は `undistortedFramebuffer` を読み出して検出する。

### 20. Android スマートフォン対応

- **対応**: Camera2 NDK による `CamAndroid`、APK の `assets/` からの自動展開、Gradle プロジェクト（`android/`）と OpenCV Android SDK 4.11.0 の自動取得を実装した。UI 部分は第 23 項で Jetpack Compose に置き換えた。

### 21. macOS の AV Foundation（`CamAvf`）対応

- **対応**: `calib` と共通の `CamAvf` を導入し、デバイス・フォーマットの列挙とフォーマット選択 UI を Windows / Android と共通化した。

### 22. macOS ビルドエラーの解消と Homebrew 非依存化、ドキュメントの整理

- **対応**:
  - `openMovie()` に残っていた未定義変数の参照を削除した。
  - OpenCV のビルド設定で Homebrew 由来の外部依存の探索を無効化し、自己完結ビルドできるようにした。
  - ImGui のグリフ範囲の追加とデバイス名のサニタイズでカメラ名の文字化けに対処した。
  - pdfLaTeX でエラーになる特殊 Unicode 文字をコメントから除き、Doxygen の HTML と `docs/pdf/refman.pdf` を生成した。

### 23. Android 版の Jetpack Compose 移行と OpenGL / ImGui 依存の排除

- **対応**: `NativeBridge.cpp` を `ANativeWindow` への CPU 直接転送による描画に置き換え、Android ビルドから EGL、GLESv3、ImGui、OpenGL ラッパー群と不要なアセット・Gradle 設定を除いた。`Aruco.h` から不要な `gg.h` の参照を削除し、デスクトップ専用コードを `#if !defined(__ANDROID__)` で分離した。

### 24. Android 版の機能追加と較正時の解像度への自動切り替え

- **対応**:
  - Android 版でキャプチャ画像を縦横比を保って中央に表示するようにした。
  - Android 版でカメラの解像度を選択できるようにした（`Menu::selectResolution()`）。
  - 較正ファイルに記録された較正時の解像度（`size`）を読み取り、最も近いカメラ解像度へ自動で切り替えるようにした（`Menu::selectBestResolution()`）。

### 25. 点検と最適化

- **指示**: `GEMINI.md` の方針と `REQUESTS.md` の経緯にもとづいてプロジェクトを点検・最適化し、文書を整理する。
- **対応**（入力部は `calib` と共通の変更）:
  - `Capture` のプラットフォーム別に重複していた `openDevice()` / `updateFormatList()` の処理を `NativeCamera` 型で共通化し、未使用の `Capture::emptyFormatList` を削除した。
  - `Camera::stop()` で `running.exchange(false)` を使い、停止処理とスレッド合流が重複しないようにした。`close()` が `stop()` を含むため、`Menu` の連続呼び出しを整理した。
  - 既定フォーマットの選択を `findDefaultFormat()` に、3 つのフォーマット選択ドロップダウンの同期処理を `Menu::selectFormatItem()` に集約した。`selectBestResolution()` を重複のない解像度リストに対して評価するようにした。
  - 非 Windows の `Menu::openDevice()` でも初期画角を設定するようにし、`startCapture()` での重複した計算を削除した。
  - 到達しない `_MSC_VER` 分岐、未使用の `fileHistory` と `<pwd.h>` などのインクルード、Android でビルドされない `mfcapture.cpp` 内の Android 用分岐を削除した。
  - `Config::initialImage` の定義を `Menu.cpp` から `Config.cpp` へ移した。
  - BOM が欠けていた C++ ソース（`Aruco.h`, `CamImage.h`, `Capture.h`, `Capture.cpp`, `Config.h`, `Config.cpp`）に BOM を付与した。
  - `GEMINI.md`、`REQUESTS.md`、`README.md` から古い記述や重複を整理した。
- **検証**: Windows の Debug / Release、Android の Debug APK のビルドが成功し、`git diff --check` とソースコードに関する Doxygen 警告がないことを確認した。
