# mfcapture

## 概要

本プログラムは、Webカメラ、動画ファイル、静止画像から取得した映像をOpenGLテクスチャへ転送し、GLSLで展開しながらリアルタイムに表示するC++アプリケーションです。

画像処理プログラミングの勉強会等において、CPU（OpenCV）とGPU（OpenGL / GLSL）による画像処理の違いや、`calib` で得られたカメラの内部パラメータ（カメラ行列および歪み係数）を用いたレンズ歪み補正の仕組みを比較学習するためのサンプルとして使用します。カメラ較正処理自体は行わず、`calib` が出力したJSON形式の較正パラメータを読み込んで補正とArUco Markerの姿勢推定に利用します。

カメラ入力には、Windowsでは Microsoft Media Foundation (`CamMf`)、macOSでは AV Foundation (`CamAvf`)、Raspberry Pi では `libcamera` (`CamLibcam`)、Android では Camera2 NDK (`CamAndroid`) を直接使用します。その他の動画・静止画像の入力にはOpenCVを使用します。デスクトップおよびRaspberry Piの描画とUIにはOpenGL / OpenGL ES 3.1、GLFW、Dear ImGuiを使用し、Android版は Jetpack Compose のUIと `ANativeWindow` へのCPU直接描画により、OpenGLやImGuiに依存しない構成としています。

## 主な機能

- Webカメラ、動画ファイル、静止画像からの映像入力
- 各プラットフォームのネイティブAPIによるカメラ入力と、解像度・フレームレート・符号化方式の組み合わせ選択
- 全フレーム処理とレイテンシ優先（低遅延）処理の切り替え
- 各種投影方式（Orthographic、Equirectangular、Equidistance、Stereographic等）によるGLSL展開描画
- `calib` が出力したJSON形式カメラ較正パラメータの安全な読み込みと、較正時の解像度への自動切り替え
- 「なし」「OpenCV (CPU)」「OpenGL (GPU / GLSL)」の3モードから選択可能なレンズ歪み補正
- ArUco Markerの検出と、較正パラメータを用いた姿勢推定・座標軸表示
- JSON構成ファイルによる投影方式、シェーダー設定、表示設定の一元管理

## プログラムの処理の流れ

デスクトップ版の `mfcapture.cpp` のメインループでは、一フレームを「第 1 パス：歪み補正」→「第 2 パス：Preference 展開」→「最終画面表示」の順に処理します（Android版では `NativeBridge.cpp` がフレーム取得、補正、検出、描画を行います）。

1. `Menu` が UI を描画し、入力源、投影方式、歪み補正方式、ArUco Marker 検出の設定変更を受け付ける。
2. `Capture` が現在の入力源から新しいフレームを取得する。
3. **第 1 パス (歪み補正)**:
   - **OpenCV 方式**: キャプチャ直後の `cv::Mat` を `Undistortion::apply()` で補正し、テクスチャ `frame` へ転送する。
   - **OpenGL 方式**: 生画像をテクスチャ `frame` へ転送し、`Menu::setupUndistortion()` で設定した `undistortion.vert` + `undistortion.frag` で中間フレームバッファ `undistortedFramebuffer` へ補正描画する。
   - **なし**: 生画像をそのままテクスチャ `frame` へ転送する。
   - ArUco Marker を検出する場合は、この段階の画像（補正なしでは生画像、補正ありでは補正後の画像）に対して検出と描画を行う。
4. **第 2 パス (Preference 展開)**: 補正方式に関わらず、第 1 パスの結果を `Menu::setup()` で設定した投影方式の展開シェーダー（`orthographic.vert` + `normal.frag` など）で最終フレームバッファ `framebuffer` へ展開する。これにより、CPU 補正と GPU 補正で同一の画角、焦点距離、姿勢、アスペクト比が適用され、表示結果が一致します。
5. **最終画面表示**: `framebuffer.draw(window.getFboWidth(), window.getFboHeight())` により、実 Framebuffer サイズに基づいて縦横比を保ったまま画面中央へ表示する。

`Framebuffer::resize()` や `Undistortion` のマップ更新は毎フレーム呼び出せますが、入力解像度やパラメータが変化していない場合は再確保・再計算を行いません。

## レンズ歪み補正

### 較正ファイルの読み込み

「ファイル」メニューの「較正ファイルを開く」から JSON ファイルを選択します。ファイルには次の行列データが必要です。

- `camera matrix`: 3 行 3 列のカメラ内部行列 $K$
- `distortion`: 5 行 1 列の歪み係数配列 $[k_1, k_2, p_1, p_2, k_3]$

読み込みは一時変数へ展開して全要素と形状を検証し、正常に取得できた場合のみ現在のパラメータを置き換えます。読み込みに失敗した場合は歪み補正方式を「なし」に戻します。較正時の解像度 (`size`) が記録されていれば、最も近いカメラ解像度へ自動で切り替えます。

### OpenCV 方式 (CPU 補正)

キャプチャ直後の CPU フレームに対し、`cv::initUndistortRectifyMap()` で事前生成した参照マップを用いて `cv::remap()` で補正したのち、テクスチャへアップロードします。GPU から CPU への読み戻しは発生しません。

### OpenGL 方式 (GPU 補正)

生画像テクスチャをそのままアップロードし、中間フレームバッファへ `undistortion.vert` と `undistortion.frag` で描画します。フラグメントシェーダーで各出力画素の正規化座標に放射歪み・接線歪みモデルを適用し、逆写像 (Backward Mapping) でサンプリングします。

## ArUco Marker の検出

「ArUco」パネルで辞書とマーカー長を設定し、「ArUco Marker 検出」を有効にすると、`Aruco` クラスがマーカーを検出して枠と ID を描画します。較正ファイルを読み込んでいれば、`cv::solvePnP()` で姿勢を推定して座標軸を描画します。補正なしでは歪み係数を用い、補正後の画像では歪みが除去済みのため歪み係数を使いません。

## 主要クラスと責務

### `Camera` と入力実装

`Camera` はキャプチャスレッド、フレームバッファ、排他制御、レイテンシ優先フラグを管理する基底クラスです。OpenGLやOpenCVに依存せず、標準C++ライブラリだけで構成されています。入力部は `calib` と共通のファイルです。

- **NVI (Non-Virtual Interface) パターン**: 公開関数 `start()`, `stop()`, `close()` がスレッドの状態管理、排他制御、スレッド合流を一元管理し、派生クラスは保護フック `onStart()`, `onStop()`, `onClose()` にハードウェア固有の処理だけを実装します。
- **単一バッファとゼロコピー転送**: フレームはCPUメモリ上の単一バッファに保持し、`lockFrame(F&& func)` が非ブロッキングロックに成功したときだけデータをコールバックへ渡して、PBOへの直接転送や`cv::Mat`へのコピーを行います。
- **仮想関数による疎結合**: 上位層は `isStillImage()`, `getFormatList()`, `selectFormat()` などの仮想関数を通じて入力を操作し、派生クラスへのダウンキャストを行いません。

| クラス | 役割 |
| --- | --- |
| `CamMf` | Windows Media Foundation によるカメラ入力 |
| `CamAvf` | macOS AV Foundation によるカメラ入力 |
| `CamAndroid` | Android Camera2 NDK によるカメラ入力 |
| `CamLibcam` | Raspberry Pi の libcamera によるカメラ入力 |
| `CamCv` | OpenCV によるカメラ、動画、ネットワーク入力 |
| `CamImage` | 静止画像入力 |
| `Capture` | 上記入力実装の所有、切り替え、開始・停止、フレーム取得の窓口 |

### `Config`、`Preference`、`Intrinsics`

- `Config`: JSON 構成ファイルの読み込みと表示設定・投影方式一覧の管理
- `Preference`: 投影方式ごとの説明、通常シェーダー (`shader`)、補正用シェーダー (`undistortion`)、固有 `Intrinsics` の保持
- `Intrinsics`: 画角、中心位置、解像度、フレームレートの保持

### `Undistortion`

較正ファイルから読み込んだカメラ行列 $K$ と歪み係数 $d$ を保持します。OpenCV 方式では参照マップの構築と `cv::remap()` の実行を担当し、OpenGL 方式では GLSL の uniform に渡す焦点距離、主点、歪み係数を提供します。

### `Aruco`

ArUco Marker の辞書の管理、マーカー検出、姿勢推定と座標軸の描画を担当します。

### `Menu`

`Menu` は Dear ImGui による操作画面と、UI 操作を各機能へ伝える処理を担当します。描画処理は次の単位に分離されています。

- `drawMainMenuBar()`: ファイル操作とパネル表示
- `drawInputPanel()`: 歪み補正方式、投影方式、姿勢、入力デバイス、フォーマット、開始・停止
- `drawArucoPanel()`: 辞書、ArUco Marker 検出、マーカー長
- `drawErrorDialog()`: エラーメッセージ表示

投影方式の同期は `selectPreference()`、キャプチャ開始は `startCapture()`、フォーマットの選択肢の同期は `updateFormatDropdowns()` と `selectFormatItem()` に集約しています。

## 低遅延キャプチャ

- **Windows (`CamMf`)**: Media Foundation Source Reader から圧縮フレームを取得し、MFT デコーダとカラーコンバータで CPU メモリ上の画像へ変換します。フォーマット列挙時にはデコーダを作らず「開始」時に適用し、`CODECAPI_AVLowLatencyMode` でデコーダ内部のバッファリングを抑制します。
- **macOS (`CamAvf`)**: `AVCaptureDeviceDiscoverySession` でカメラを列挙し、`kCVPixelFormatType_32BGRA` を指定して OS 側で BGRA へ変換します。`alwaysDiscardsLateVideoFrames` でレイテンシ優先を切り替えます。
- レイテンシ優先時は古いフレームを破棄して常に最新のフレームを使い、全フレーム処理時はメインスレッドが取得するまで次のフレームで上書きしません。

詳細は [docs/CamMf.md](docs/CamMf.md)、[docs/CamAvf.md](docs/CamAvf.md) を参照してください。

## 基本操作

1. 「ファイル」メニューから画像、動画、または較正ファイル (`calib` の出力 JSON) を開く。
2. カメラを使用する場合は「入力」パネルでカメラ装置を選択する。
3. Windows および macOS では解像度、フレームレート、符号化方式を選択する（未選択時は 1280×720 に近いものが選ばれる）。
4. 必要に応じて「レイテンシ優先」を有効にする。
5. 「開始」を押してキャプチャを開始する。
6. 「歪み補正」ラジオボタンで「なし」「OpenCV」「OpenGL」のいずれかを選択する。
7. 必要に応じて「ArUco」パネルで ArUco Marker 検出を有効にする。

※ 較正ファイルを読み込むまでは、OpenCV および OpenGL による歪み補正を選択できません。

## 構成ファイル

既定の構成ファイルは `mfcapture_config.json` です。主な項目は次のとおりです。

- ウィンドウサイズと背景色
- 展開メッシュのサンプル数
- カメラ姿勢、焦点距離、焦点距離範囲
- ArUco 辞書とマーカー長
- 初期表示画像
- 投影方式ごとの説明、画角、中心、解像度、fps
- `shader`: 通常表示用の頂点・フラグメントシェーダー
- `undistortion`: OpenGL 歪み補正用の頂点・フラグメントシェーダー

## 開発環境とビルド

- C++17
- CMake 3.13 以降
- Visual Studio 2022 以降（Windows、x64）
- Clang / Xcode Command Line Tools（macOS、arm64 / x64）
- OpenCV 4.13.0（Android は OpenCV Android SDK 4.11.0）
- GLFW 3.4
- Dear ImGui 1.92.8
- Native File Dialog Extended 1.3.0

### Windows

```powershell
cmake -S . -B build
cmake --build build --config Debug
cmake --build build --config Release
```

初回の CMake 構成時には、`CMakeLists.txt` が必要な依存ライブラリを `libs` 以下へ自動取得します。ビルド後は、シェーダー、構成ファイル、画像、フォント、OpenCV DLL が実行ファイルのディレクトリへコピーされます。

### macOS

Xcode Command Line Tools（`xcode-select --install`）と CMake があれば、Homebrew などの外部パッケージマネージャに依存せずビルドできます。依存ライブラリは CMake が `libs` へ取得・ビルドし、`AVFoundation` と `CoreMedia` フレームワークを自動的にリンクします。

```bash
cmake -B build
cmake --build build -j$(sysctl -n hw.ncpu)
./build/mfcapture
```

### Raspberry Pi (Linux ARM)

Raspberry Pi OS (Bookworm / Bullseye) では、標準のパッケージマネージャから開発パッケージを導入してビルドします。ARM環境では OpenGL ES 3.1 (`USE_GLES`) と libcamera バックエンド (`USE_LIBCAMERA`) が既定で有効になります。

```bash
sudo apt update
sudo apt install -y build-essential cmake libopencv-dev libglfw3-dev libgtk-3-dev libgles2-mesa-dev libegl1-mesa-dev libcamera-dev
cmake -B build
cmake --build build -j$(nproc)
./build/mfcapture
```

Raspberry Pi Camera Module を V4L2 経由で使用する場合は `libcamerify ./build/mfcapture` で起動します。macOS と Linux でも、ビルド後にシェーダー、構成ファイル、画像が `build/` へコピーされます。

### Android

Android Studio で `android` フォルダを開いてビルド・実行します。実機の事前設定、カメラ権限、Logcat によるデバッグ手順は [Android 実機テストガイド](docs/Android.md) を参照してください。コマンドラインでは次のようにビルドします。

```powershell
cd android
.\gradlew.bat assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

## 開発時の確認事項

設計方針と検証方針の詳細は [GEMINI.md](GEMINI.md) にまとめています。特に次の点に注意してください。

- OpenCV 補正は GPU 転送前、OpenGL 補正は GLSL 内だけで実行すること。
- 補正マップと Framebuffer Object は、入力サイズが変化した場合だけ再作成すること。
- 較正ファイルの読み込みに失敗した場合、以前の値を部分的に更新しないこと。
- プラットフォーム固有処理を `Capture` と各 `Cam*` クラスに閉じ込め、`Menu` に固有型を露出させないこと。
- `calib` と共通のファイルは同一内容に保つこと。
- C++ ソースは BOM 付き UTF-8、GLSL ソースは BOM なし UTF-8 とすること。
- コメントと Doxygen を実装変更と同時に更新すること。

## ドキュメント・関連資料

### 開発・管理ドキュメント

- [GEMINI.md](GEMINI.md): プロジェクト開発方針と環境定義
- [REQUESTS.md](REQUESTS.md): 開発依頼および変更対応履歴

### プラットフォーム・機能別ガイド

- [docs/OpenXR.md](docs/OpenXR.md): OpenXR バックエンド実装マニュアル
- [docs/Android.md](docs/Android.md): Android 実機テストとビルドガイド
- [docs/CamMf.md](docs/CamMf.md): Windows Media Foundation ビデオキャプチャクラス `CamMf` の解説
- [docs/CamAvf.md](docs/CamAvf.md): macOS AV Foundation ビデオキャプチャクラス `CamAvf` の解説
- [docs/CamAndroid.md](docs/CamAndroid.md): Android Camera2 NDK ビデオキャプチャクラス `CamAndroid` の解説
- [docs/CamLibcam.md](docs/CamLibcam.md): Raspberry Pi ネイティブカメラキャプチャクラス `CamLibcam` の解説

### 勉強会プレゼンテーション・ハンドブック

- [presentation.md](presentation.md) / [presentation.html](presentation.html): カメラキャリブレーション & レンズ歪み補正の勉強会用スライド
- [workshop_handbook.md](workshop_handbook.md) / [workshop_handbook.html](workshop_handbook.html): 勉強会ハンズオン用ハンドブック
- [CamMf.html](CamMf.html): `CamMf` の構造とデータパイプラインを解説した勉強会用スライド
