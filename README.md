# mfcapture

## 概要

本プログラムは、Webカメラ、動画ファイル、静止画像から取得した映像をOpenGLテクスチャへ転送し、GLSLで展開しながらリアルタイムに表示するC++アプリケーションです。

画像処理プログラミングの勉強会等において、CPU（OpenCV）とGPU（OpenGL / GLSL）による画像処理モデルの違いや、`calib-wom-msmf` で得られたカメラの内部パラメータ（カメラ行列および歪み係数）を用いたレンズ歪み補正の仕組みを比較学習するためのサンプルとして使用します。

Windowsのカメラ入力には Microsoft Media Foundation（MSMF）を直接使用します。Raspberry Pi ではネイティブの `libcamera` バックエンド (`CamLibcam`) および OpenGL ES 3.1 をサポートします。macOSおよびLinux、ならびに動画・静止画像の入力にはOpenCVを使用し、描画とUIにはOpenGL / OpenGL ES、GLFW、Dear ImGuiを使用します。カメラ較正処理自体は行わず、`calib-wom-msmf` が出力したJSON形式の較正パラメータを読み込んで補正に利用します。

## 主な機能

- Webカメラ、動画ファイル、静止画像からの映像入力
- Windows Media FoundationによるH.264/MJPGの直接取得、デコード、RGB変換 (`CamMf`)
- Raspberry Pi ネイティブの `libcamera` による高速フレーム取得と色変換 (`CamLibcam`)
- 解像度、フレームレート、符号化方式の組み合わせ選択
- 全フレーム処理とレイテンシ優先（低遅延）処理の切り替え
- 各種投影方式（Orthographic、Equirectangular、Equidistance、Stereographic等）によるGLSL展開描画
- `calib-wom-msmf` が出力したJSON形式カメラ較正パラメータの安全な読み込み
- 「なし」「OpenCV (CPU)」「OpenGL (GPU / GLSL)」の3モードから選択可能なレンズ歪み補正
- JSON構成ファイルによる投影方式、シェーダー設定、表示設定の一元管理

## プログラムの処理の流れ

`mfcapture.cpp` のメインループでは、一フレームを次の順序で処理します。

1. `Menu` がUIを描画し、入力源、投影方式、歪み補正モードの設定変更を受け付ける。
2. `Capture` が現在の入力源から最新フレームを取得する。
3. **OpenCV 補正モード選択時**: キャプチャ直後の CPU メモリ上の `cv::Mat` に対して `Undistortion::apply()` を適用し、`cv::remap()` で補正する。
4. 補正済み（または未補正）のフレームを Pixel Buffer Object (PBO) 等を経由して OpenGL テクスチャへ転送する。
5. `Menu::setup()` が現在選択されている `Preference` と補正モードに応じた展開シェーダー（通常表示用 `normal.frag` または OpenGL 歪み補正用 `undistortion.frag`）を設定する。
6. `Framebuffer` が GLSL シェーダーによる変換・展開結果を描画バッファに生成する。
7. 最終画像と Dear ImGui の UI を画面へ描画する。

OpenCV 補正はキャプチャ直後の CPU メモリ上かつ GPU 転送前に実行します。この順序により、GPU から CPU への不要な読み戻しを一切発生させず、補正結果だけを一度アップロードする効率的なパイプラインを維持します。一方、OpenGL 補正モードでは CPU 側の補正処理を行わず、テクスチャ転送後に `undistortion.frag` シェーダー内で逆写像 (Backward Mapping) により参照 UV 座標を動的に変換します。

`Framebuffer::resize()` や `Undistortion` のマップ更新は毎フレーム呼び出せますが、入力解像度やパラメータが変化していない場合は再確保・再計算を行わないキャッシュ設計となっています。

## レンズ歪み補正

### 較正ファイルの読み込み

「ファイル」メニューの「較正ファイルを開く」から、Native File Dialog Extended によるダイアログで JSON ファイルを選択します。ファイルには次の行列データが必要です。

- `camera matrix`: 3 行 3 列のカメラ内部行列 $K$
- `distortion`: 5 行 1 列の歪み係数配列 $[k_1, k_2, p_1, p_2, k_3]$

読み込みは一時変数へ展開して全要素と形状を検証し、正常に取得できた場合のみ現在保持しているパラメータを置き換えます。読み込み失敗時は歪み補正モードを「なし」に戻し、不完全な値が使用されるのを防ぎます。

### OpenCV 方式 (CPU 補正)

`cv::initUndistortRectifyMap()` を用いて補正座標参照マップ `map1`, `map2` を事前生成し、`cv::remap()` でフレームを補正します。重い計算を伴うマップ生成は解像度変更時のみ実行され、毎フレームの再計算を回避します。

### OpenGL 方式 (GPU 補正)

`orthographic.vert`（または専用頂点シェーダー）と `undistortion.frag` を使用します。フラグメントシェーダー内で各出力ピクセルの正規化座標から放射歪み・接線歪みモデルを適用して逆写像し、入力テクスチャの参照 UV 座標を計算します。CPU 負荷を増やすことなく高速に処理されます。

## 主要クラスと責務

### `Camera` と入力実装

`Camera` はキャプチャスレッド、共有フレーム、排他制御、レイテンシ優先フラグを管理する基底クラスです。

- `CamMf`: Windows Media Foundation によるカメラ入力
- `CamCv`: OpenCV によるカメラ、動画、ネットワーク入力
- `CamImage`: 静止画像入力
- `Capture`: 上記入力実装の所有、切り替え、開始・停止、フレーム取得の窓口

### `Config`、`Preference`、`Intrinsics`

- `Config`: JSON 構成ファイルの読み込みと表示設定・投影方式一覧の管理
- `Preference`: 投影方式ごとの説明、通常シェーダー (`shader`)、補正用シェーダー (`undistortion`)、固有 `Intrinsics` の保持
- `Intrinsics`: 画角、中心位置、解像度、フレームレートの保持

### `Undistortion`

較正ファイルから読み込んだカメラ行列 $K$ と歪み係数 $d$ を保持します。OpenCV 方式では参照マップの構築と `cv::remap()` の実行を担当し、OpenGL 方式では GLSL の uniform に渡す焦点距離、主点、歪み係数ベクトルを提供します。

### `Menu`

`Menu` は Dear ImGui による操作画面と、UI 操作を各機能へ伝える処理を担当します。描画処理は次の単位に分離されています。

- `drawMainMenuBar()`: ファイル操作とパネル表示
- `drawInputPanel()`: 投影方式、入力デバイス、歪み補正方式、開始・停止
- `drawErrorDialog()`: エラーメッセージ表示

投影方式の同期は `selectPreference()`、キャプチャ開始は `startCapture()` に集約し、UI 内に同じ状態遷移を重複して実装しない方針です。

## Windowsでの低遅延キャプチャ

`CamMf` は Media Foundation Source Reader から圧縮フレームを取得し、MFT デコーダとカラーコンバータを用いて CPU メモリ上の RGB 画像へ変換します。

- フォーマット列挙時には重いデコーダ初期化を行わず、ユーザーが「開始」を指示したタイミングで選択フォーマットを適用する遅延初期化を行う。
- `CODECAPI_AVLowLatencyMode` を設定し、MFT デコーダ内部のバッファリングを抑制する。
- レイテンシ優先時 (`prioritizeLatency == true`) は古いフレームを廃棄し、常に最新のフレームを共有バッファへ反映する。
- 全フレーム処理時は、メインスレッドがフレームを取得するまで次のフレームで上書きしない。
- COM オブジェクトおよび MFT バッファは、RAII パターン等を用いて早期 return 時にも解放漏れが発生しないよう安全に管理する。

## 基本操作

1. 「ファイル」メニューから画像、動画、または較正ファイル (`calib-wom-msmf` の出力 JSON) を開く。
2. カメラを使用する場合は「入力」パネルでカメラ装置を選択する。
3. Windows では解像度、フレームレート、符号化方式を選択する。
4. 必要に応じて「レイテンシ優先」を有効にする。
5. 「開始」を押してキャプチャを開始する。
6. 「歪み補正」ラジオボタンで「なし」「OpenCV」「OpenGL」のいずれかを選択する。

※ 較正ファイルを読み込むまでは、OpenCV および OpenGL による歪み補正機能を選択できません。

## 構成ファイル

既定の構成ファイルは `mfcapture_config.json` です。主な項目は次のとおりです。

- ウィンドウサイズと背景色
- 展開メッシュのサンプル数
- カメラ姿勢、焦点距離、焦点距離範囲
- 初期表示画像
- 投影方式ごとの説明、画角、中心、解像度、fps
- `shader`: 通常表示用の頂点・フラグメントシェーダー
- `undistortion`: OpenGL 歪み補正用の頂点・フラグメントシェーダー

起動時に必要なシェーダーを構築し、UI で選択された補正方式に応じて切り替えて使用します。

## 開発環境とビルド

- C++17 (`/std:c++17`)
- CMake 3.13 以降
- Visual Studio 2022 以降（Windows、x64）
- OpenCV 4.13.0
- GLFW 3.4
- Dear ImGui 1.92.8
- Native File Dialog Extended 1.3.0

Windows での基本的なビルド例（PowerShell）：

```powershell
cmake -S . -B build
cmake --build build --config Debug
cmake --build build --config Release
```

初回の CMake 構成時には、`CMakeLists.txt` が必要な依存ライブラリを `libs` 以下へ自動取得します。ビルド後は、シェーダー、構成ファイル、画像、フォント、OpenCV DLL が実行ファイルのディレクトリへコピーされます。

### Raspberry Pi (Linux ARM) でのビルド例

Raspberry Pi OS (Bookworm / Bullseye, 64-bit / 32-bit) では、標準のパッケージマネージャから必要な開発パッケージを導入してビルドします。

```bash
# 依存パッケージのインストール
sudo apt update
sudo apt install -y build-essential cmake libopencv-dev libglfw3-dev libgtk-3-dev libgles2-mesa-dev libegl1-mesa-dev libcamera-dev libcamera-tools

# ビルド (OpenGL ES 3.1 および libcamera ネイティブバックエンドを使用)
cmake -B build -DUSE_GLES=ON -DUSE_LIBCAMERA=ON
cmake --build build -j$(nproc)

# 実行
./build/mfcapture

# Raspberry Pi Camera Module を V4L2 経由で使用する場合 (libcamerify 経由)
libcamerify ./build/mfcapture
```

ビルド完了後、POST_BUILD コマンドによりシェーダーおよび JSON 構成ファイル、画像アセットが `build/` ディレクトリへ自動コピーされます。

## 開発時の確認事項

- OpenCV 補正は GPU 転送前、OpenGL 補正は GLSL 内だけで実行すること。
- 補正マップと Framebuffer Object は、入力サイズが変化した場合だけ再作成すること。
- 較正ファイルの読み込みに失敗した場合、以前の値を部分的に更新しないこと。
- 通常シェーダーと補正シェーダーは同じ投影設定から選択できること。
- Windows 固有処理は `CamMf` と `Capture` に閉じ込め、`Menu` に Media Foundation 固有型を露出させないこと。
- `const_cast` や `friend` による不変条件迂回を排出し、`getSettings()` / `setSettings()` 等の公開 API で状態連携すること。
- クラスメンバ変数の初期化はコンストラクタの初期化子リストではなくクラス定義（ヘッダ内）のデフォルトメンバ初期化構文（インクラス初期化）へ集約すること。
- `calib-wom-msmf` との共通処理で変数名・関数名は `mfcapture`、コメント・Doxygen 表現は `calib-wom-msmf` に統一すること。
- C++ ソースは `UTF-8 with BOM`、GLSL ソースは `UTF-8 without BOM` の文字コード規約を厳守すること。
- コメントと Doxygen を実装変更と同時に更新すること。

## ドキュメント・関連資料

本プロジェクトには、設計方針や勉強会用のプレゼンテーション・資料が用意されています。

### 開発・管理ドキュメント
- [GEMINI.md](GEMINI.md): プロジェクト開発方針と環境定義
- [REQUESTS.md](REQUESTS.md): 開発依頼および変更対応履歴

### モジュール解説ドキュメント
- [CamMf.md](CamMf.md): Windows Media Foundation ビデオキャプチャクラス `CamMf` の実装詳細および Win32 / MF API リファレンス
- [CamMf.html](CamMf.html): `CamMf` の構造とデータパイプラインを解説した勉強会用スライド (HTML)
- [CamLibcam.md](CamLibcam.md): Raspberry Pi ネイティブカメラキャプチャクラス `CamLibcam` の実装詳細および libcamera C++ API リファレンス

### 勉強会プレゼンテーション・ハンドブック
- [presentation.md](presentation.md): カメラキャリブレーション & レンズ歪み補正プレゼンテーション概要
- [presentation.html](presentation.html): カメラキャリブレーション & レンズ歪み補正勉強会用スライド (HTML)
- [workshop_handbook.md](workshop_handbook.md): 勉強会ハンズオン用ハンドブック
- [workshop_handbook.html](workshop_handbook.html): 勉強会ハンズオン用ハンドブック (HTML)

