# 作業指示および対応履歴

## 概要

本プロジェクトでは、Windows Media Foundation による低遅延キャプチャを導入し、
CMake ベースの再現可能なビルド環境へ移行したうえで、機能を画像取得と GLSL 表示に
整理しました。その後、`calib-wom-msmf` が出力した内部パラメータを利用し、
OpenCV と OpenGL の二方式を比較できる歪み補正機能を追加しています。

## 作業履歴

### 1. `CamMf` のバックポート

- **指示**: `calib-wom-msmf` の `CamMf` クラスを `mfcapture` へバックポートする。
- **対応**:
  - Microsoft Media Foundation Source Reader、MFT デコーダ、カラーコンバータを
    使用するカメラ入力を移植した。
  - H.264／MJPG などの入力を CPU メモリ上の表示可能なフレームへ変換した。
  - レイテンシ優先と全フレーム処理の切り替え、フォーマット列挙、遅延初期化、
    COM リソース管理を反映した。
  - `Camera` と `Capture` のフレーム転送経路を移植後のインターフェースへ合わせた。

### 2. CMake ビルドと依存ライブラリ管理

- **指示**: `calib-wom-mf` と同様に CMake でビルドできるようにし、ダウンロードした
  ライブラリを `libs` へ置く。既存の `libs` ジャンクションは削除する。
- **対応**:
  - C++17 の `CMakeLists.txt` を整備した。
  - OpenCV、GLFW、Dear ImGui、Native File Dialog Extended などを `libs` 以下へ
    取得して参照する構成にした。
  - `libs` ジャンクションを削除し、通常のディレクトリへ置き換えた。
  - 実行時に必要な DLL、構成ファイル、シェーダー、画像、フォントをビルド先へ
    コピーする処理を追加した。
  - Visual Studio の Debug／Release 構成でビルドできることを確認した。

### 3. 較正機能と ChArUco Board 作成機能の削除

- **指示**: `mfcapture` から ChArUco Board 作成を含む較正機能を削除し、
  キャプチャした画像を GLSL で変形表示するだけにする。
- **対応**:
  - マーカー検出、標本取得、較正計算、ChArUco Board 作成に関するクラス、UI、
    設定を削除した。
  - 入力、テクスチャ転送、GLSL 変換、画面表示という処理へ責務を整理した。
  - OpenCV は動画入力や後に追加する画像補正に必要な範囲だけ残した。

### 4. ソーストップディレクトリの整理

- **指示**: 不要になったプロジェクトファイルなどを削除し、`.gitignore` も修正する。
- **対応**:
  - CMake から生成できる Visual Studio プロジェクトファイルや古いビルド成果物を
    ソース管理対象から削除した。
  - out-of-source build を前提に `.gitignore` を更新した。
  - ダウンロード依存物、生成文書、各構成のビルド成果物を無視するよう整理した。

### 5. 描画ループ内の `Framebuffer::resize()` の確認

- **質問**: 毎フレーム `framebuffer.resize(frame)` を呼ぶことによるオーバーヘッドは
  大きくないか。
- **確認結果**:
  - `resize()` は現在サイズと入力フレームサイズを比較し、同じ場合は再確保しない。
  - 通常フレームで行われる処理はサイズ確認だけであり、大きなオーバーヘッドには
    ならない。
  - 入力源や解像度が変化した場合だけ、Framebuffer Object と付随リソースを
    作り直す現在の呼び出し位置を維持した。

### 6. 較正ファイルによる歪み補正の追加

- **指示**:
  - `calib-wom-msmf` で作成した内部パラメータ JSON を「ファイル」メニューから
    Native File Dialog Extended で読み込む。
  - OpenCV と OpenGL／GLSL の二方式で補正する。
  - `mfcapture_config.json` の `Default` に `undistortion` ノードを追加する。
  - 「なし」「OpenCV」「OpenGL」のラジオボタンを追加する。
- **対応**:
  - `Undistortion` クラスを追加し、`camera matrix` と `distortion` を保持するようにした。
  - JSON の必須項目と行列サイズを検証し、成功時だけ現在値を置き換えるようにした。
  - OpenCV 方式はキャプチャ直後かつ GPU 転送前に `cv::remap()` を実行するようにした。
    この位置により GPU から CPU への読み戻しを避けられる。
  - 補正マップは入力サイズが変化した場合だけ
    `cv::initUndistortRectifyMap()` で再作成するようにした。
  - `undistortion.vert` と `undistortion.frag` を追加し、GLSL で歪み座標を計算する
    OpenGL 方式を実装した。
  - `Preference` が通常シェーダーと補正シェーダーを起動時に構築し、
    `Menu::setup()` が選択方式に応じて切り替えるようにした。
  - 較正値がない状態では OpenCV／OpenGL を選択できないようにした。

### 7. 較正ファイル読み込み時の例外修正

- **指示**: `mfcapture` では `Undistortion.cpp` の行列読み込みで例外が発生するが、
  同じファイルを `calib-wom-msmf` では正常に読めるため修正する。
- **原因**:
  - `cv::Mat loaded{ rows, columns, CV_64F };` が行列サイズを指定する
    コンストラクタではなく initializer-list と解釈され、意図しない 3×1 行列が
    作成されていた。
  - その行列へ 3×3 の要素を書き込んだため範囲外アクセスの例外が発生した。
- **対応**:
  - `cv::Mat loaded(rows, columns, CV_64F);` と丸括弧による構築へ変更した。
  - 読み込み失敗時に現在の較正値を部分更新しない処理を維持した。

### 8. 教材向けコメントと Doxygen の整備

- **指示**: 追加コードの各ブロックへ「何のために、どういう処理を行うか」を説明する
  コメントと Doxygen コメントを追加し、既存コメントも点検する。
- **対応**:
  - `Undistortion`、較正ファイル読み込み、OpenCV 補正経路、シェーダー切り替え、
    uniform 設定へ目的を説明する日本語コメントを追加した。
  - GLSL へカメラ座標、正規化座標、放射・接線歪み、範囲外判定の説明を追加した。
  - 公開型、公開関数、引数、戻り値の Doxygen コメントを追加・更新した。
  - 既存コードの `@param` 引数名不一致、誤記、未知の Doxygen コマンドを修正した。
  - `Doxyfile` が `libs`、`build`、`docs` を走査しないよう更新した。
- **検証**:
  - Doxygen でプロジェクトコードの引数・コメント不一致がないことを確認した。
  - Debug／Release の両構成でビルドが成功した。
  - `git diff --check` が成功した。

### 9. プロジェクト文書の整備

- **指示**: `calib-wom-msmf` に倣い、プログラム解説、開発方針、依頼と処理内容の
  履歴を作成する。
- **対応**:
  - 利用方法、処理の流れ、クラスの責務、補正方式、ビルド方法を `README.md` に
    まとめた。
  - 教材としての目的、クラス境界、画像処理パイプライン、安全性、コメント方針、
    検証方針を `GEMINI.md` にまとめた。
  - これまでの指示、原因、対応、検証結果を本 `REQUESTS.md` に時系列で記録した。

### 10. クラスメンバ変数の初期化の集約

- **指示**: クラスメンバ変数の初期化を、コンストラクタからクラス定義（ヘッダ内）へ移行する。
- **対応**:
  - `Buffer`, `Camera`, `CamCv`, `CamImage`, `CamMf`, `Capture`, `Config`, `Expand`, `Framebuffer`, `Intrinsics`, `Menu`, `Preference`, `Texture`, `Undistortion` 等の全クラスで、初期値をヘッダ内（インクラス初期化構文 `int x{ 0 };`, `Framebuffer() = default;` 等）へ集約した。
  - コンストラクタ初期化子リストをシンプル化し、メンバーの初期化漏れを防ぐ構造へリファクタリングした。

### 11. `const_cast` および `friend` の完全廃止と公開 API の採用

- **指示**: `mfcapture` で使っている `const_cast` や `friend` を、`calib-wom-msmf` に倣って getter / setter に置き換える。
- **対応**:
  - `Config.h` から `friend class Menu;` 宣言を削除し、`getSettings()`, `setSettings()`, `getPreferences()` 等の公開 API を追加した。
  - `Menu` が保持する `Config` への参照を非 `const` 参照 (`Config& config`) へ変更し、`Menu::loadConfig()` や `saveConfig()` での `const_cast` を全廃した。
  - 直接的なプライベートメンバ参照を公開 API 経由に統一し、カプセル化と安全性を向上させた。

### 12. 共通処理における命名規約・コメントの統一とドキュメント同期

- **指示**:
  - `calib-wom-msmf` と `mfcapture` で共通する変数名・関数名は `mfcapture` のものに合わせる。
  - コメント表現は `calib-wom-msmf` に合わせる。
  - 修正内容を両プロジェクトのドキュメント (Markdown, HTML) に反映する。
- **対応**:
  - `calib-wom-msmf` と `mfcapture` 間で共通する変数名・関数名を `mfcapture` の命名規則へ統一し、Doxygen および実装コメント記述を `calib-wom-msmf` の解説表現へ統一した。
  - C++ ソースは `UTF-8 with BOM`、GLSL ソースは `UTF-8 without BOM` の保存形式を再検証し、Debug / Release 両構成での正常ビルドを確認した。
  - `presentation.html`, `presentation.md`, `workshop_handbook.html`, `workshop_handbook.md`, `images/` 内のプレゼンテーション・ハンドブック教材資産に C++ クラス設計・カプセル化方針を追記し、`calib-wom-msmf` および `mfcapture` の両ワークツリーへ反映・同期した。

### 13. GStreamer 関連コードの削除

- **指示**: GStreamer は使用しないため、`mfcapture` の関連コードを削除し、`GEMINI.md`, `README.md`, `REQUESTS.md` も更新する。
- **対応**:
  - `Menu.cpp` から `cv::CAP_GSTREAMER` バックエンド定義および `openDevice()` 内の GStreamer パイプライン処理分岐を完全に削除した。
  - `GEMINI.md` に GStreamer 非対応・構成非追加の基本方針を明記した。
  - Debug / Release 両構成での完全ビルドが成功することを確認した。

### 14. Raspberry Pi (`CamLibcam`) および Linux ARM (GLES 3.1) のサポート

- **指示**: `calib-rpi` に追加された `CamLibcam` クラスを `mfcapture` へ導入し、技術解説ドキュメント `CamLibcam.md` を作成する。また Raspberry Pi (Linux ARM) 上でビルド・動作可能にする。
- **対応**:
  - `CamLibcam.h`, `CamLibcam.cpp`, `CamLibcam.md` を導入し、libcamera ネイティブバックエンドによる高効率なフレーム取得・色変換処理を実装した。
  - Linux 環境における `cv::CAP_V4L2` カメラ選択と `/sys/class/video4linux` デバイス列挙を整備した。
  - CMake オプション `USE_GLES` および `USE_LIBCAMERA` を整備し、OpenGL ES 3.1 / EGL 環境に対応した。
  - Linux 環境特有のビルドエラー（`pwd.h`, `sys/types.h` 不足や GLES での `GL_BGR` 未定義等）を解消した。

### 15. `GgApp::OpenXR` への統合と同期

- **指示**: `calib-rpi` と `mfcapture` の `GgApp` クラスの OpenXR 対応を `calib-openxr` と一致させ、リベースおよび同期を可能にする。
- **対応**:
  - `GgApp` 内部に公式 OpenXR 実装をカプセル化し、MSVC Debug 構成での `openxr_loaderd.lib` リンク不整合を解消した。
  - 3 つのリポジトリ間で OpenXR 実装の足並みを揃え、VR / MR ヘッドセットへのステレオ出力パスを確立した。

### 16. レンズ歪み補正の 2 パス描画パイプライン統合 (OpenCV / OpenGL 結果・アスペクト比の完全一致)

- **指示**: OpenCV による歪み補正と OpenGL による歪み補正で結果（画角、アスペクト比、拡大縮小率）が一致しない問題を解消する。
- **原因**:
  - OpenCV 方式では歪み補正後に Preference の展開シェーダー（`orthographic.vert` + `normal.frag`）が二重に適用されていたのに対し、OpenGL 方式では展開シェーダーがスキップされ、歪み補正シェーダー（`undistortion.vert` + `undistortion.frag`）のみが実行されていたため、画角やアスペクト比が一致していなかった。
- **対応**:
  - レンダリングパイプラインを「第 1 パス：歪み補正（OpenCV / OpenGL / なし）」$\rightarrow$「第 2 パス：共通の Preference 展開シェーダー」の 2 パス構成に統合した。
  - 中間フレームバッファ `undistortedFramebuffer` を導入し、OpenGL 方式では第 1 パスで GPU 歪み補正を行い、その結果を展開パスへ渡すようにした。
  - `Menu::setupUndistortion()` を新設して歪み補正シェーダーの設定を分離し、`Menu::setup()` は常に展開シェーダーを設定するように整理した。
  - 最終表示を `framebuffer.draw(window.getFboWidth(), window.getFboHeight())` に統一し、HiDPI 環境でのアスペクト比歪みを防止した。
- **検証**:
  - OpenCV 方式と OpenGL 方式を切り替えても、画角・アスペクト比・拡大縮小率が完全に一致することを確認した。
  - MSVC Debug / Release 両構成で警告・エラーなくビルドが成功することを確認した。
