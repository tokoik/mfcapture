# プロジェクト開発方針と環境定義 (GEMINI.md)

本ドキュメントは、`mfcapture` の設計方針、実装上維持すべき条件、および
開発・動作環境を定義します。

## 1. プロジェクトの目的

- 画像処理プログラミングの勉強会で使用できる、読みやすいサンプルを作成します。
- 入力画像に対する処理の位置と、CPU（OpenCV）と GPU（OpenGL／GLSL）の役割の違いを
  コードから追える構成にします。
- 本プログラムは画像の取得、GLSL による変形、既存の較正結果による歪み補正に
  責務を限定します。
- カメラ較正、ChArUco Board の作成、標本取得、較正値の算出は実装しません。

## 2. 開発環境

- 統合開発環境: Visual Studio 2022 以降（Windows）、VS Code / GCC（Linux）
- 開発言語: C++17
- ターゲット: 64 bit（x64 / aarch64）
- ビルドシステム: CMake 3.13 以降
- C++ ソース（`.h`、`.cpp`）: UTF-8、BOM 付き
- GLSL ソース（`.vert`、`.frag`、`.comp`）: UTF-8、BOM なし

外部依存ライブラリは CMake がプロジェクト直下の `libs` へ取得します。
`libs` を別ディレクトリへのジャンクションにしてはいけません。インクルードパス、
ライブラリパス、DLL コピー、Visual Studio のデバッグ環境は `CMakeLists.txt` に
集約します。生成された Visual Studio プロジェクトファイルはソース直下へ置かず、
out-of-source build を使用します。

## 3. 入力とキャプチャ

- Windows のカメラ入力は Microsoft Media Foundation を使用します (`CamMf`)。
- Raspberry Pi のカメラ入力は libcamera ネイティブバックエンドを使用します (`CamLibcam`)。
- その他のプラットフォームのカメラ入力と動画入力は OpenCV を使用します (`CamCv`)。
- GStreamer パイプライン入力はサポート対象外とし、構成ファイルや UI に GStreamer 固有の設定を追加しません。
- 静止画像は `CamImage` を通して扱います。
- `Camera.h` は純粋なフレーム取得レイヤとし、OpenGL (`gg.h`)、OpenCV、GLFW への依存を完全に排除して標準 C++ ライブラリのみで構成します。
- NVI (Non-Virtual Interface) パターンを採用し、公開インターフェース `start()`, `stop()`, `close()` で排他制御、スレッド状態フラグ（`running`）、およびスレッド合流（`thr.join()`）などの共通ライフサイクルを一元管理します。派生クラスは保護フック関数 `onStart()`, `onStop()`, `onClose()` にハードウェア固有処理のみを実装します。
- 従来の二重バッファ（`frame` と `image`）を廃止し、単一バッファ（`std::vector<std::uint8_t> image`）へ集約してメモリ使用量と不要な内部コピーを排除します。
- 上位層へのフレーム転送はテンプレートメソッド `lockFrame(F&& func)` によるコールバック方式とし、非ブロッキング排他ロック（`try_to_lock`）成功時のみデータポインタを渡して直接 PBO 転送や `cv::Mat` へのコピーを行うゼロコピー設計とします。
- 上位層での `dynamic_cast` による具象クラス依存を排除し、`isStillImage()`, `getFormatList()`, `selectFormat()` 等の基底クラス仮想関数を介して疎結合に連携します。
- プラットフォーム固有処理は `CamMf`、`CamLibcam`、`CamCv`、`Capture` に閉じ込め、UI と
  描画ループへプラットフォーム固有型を露出させません。
- `CamMf` は MFT デコーダとカラーコンバータを使い、CPU メモリ上のフレームへ変換します。
- フォーマット列挙時には重いデコーダ初期化を行わず、開始時に選択フォーマットを
  適用する遅延初期化を維持します。
- レイテンシ優先時は古いフレームを破棄し、全フレーム処理時は取得前のフレームを
  上書きしません。
- スレッド間で共有する状態は atomic、mutex、condition variable を用途に応じて使い、
  データレースと待機漏れを防ぎます。

## 4. 画像処理パイプライン

一フレームの基本順序は、入力取得、第 1 パス（歪み補正）、第 2 パス（Preference 展開）、
最終表示の 2 パス構成とします。

- **第 1 パス (歪み補正)**:
  - OpenCV 歪み補正はキャプチャ直後、OpenGL テクスチャへ転送する前に行います。GPU から CPU への読み戻しを追加してはいけません。
  - OpenGL 歪み補正は生画像テクスチャを入力とし、中間フレームバッファ (`undistortedFramebuffer`) へ `undistortion.vert` と `undistortion.frag` で GPU 補正描画を行います。
  - 補正なしの場合は、生画像テクスチャをそのまま第 2 パスへ渡します。
  - 補正なしと OpenGL 補正では Pixel Buffer Object を使用する高速な転送経路を維持します。
- **第 2 パス (Preference 展開)**:
  - 歪み補正モード（なし／OpenCV／OpenGL）に関わらず、補正済み（または未補正）のフレームテクスチャを入力として、共通の Preference 展開シェーダー（`orthographic.vert` + `normal.frag` など）で最終フレームバッファ (`framebuffer`) へ展開します。
  - これにより、CPU 補正と GPU 補正で同一の画角、焦点距離、回転、アスペクト比が保証され、補正結果が完全に一致します。
- **最終表示**:
  - `framebuffer.draw(window.getFboWidth(), window.getFboHeight())` を用い、実 Framebuffer サイズに基づいて縦横比を維持した中央表示（contain 方式）を行います。
- `Undistortion` の補正マップは入力サイズが変化した場合だけ再構築します。
- `Framebuffer::resize()` は毎フレーム呼び出せますが、同じサイズでは GPU リソースを再確保しないキャッシュ実装を維持します。
- CPU 側のフレーム形式と GLSL が期待する色成分の対応を変更する場合は、最終表示シェーダーまで含めて確認します。

## 5. 較正ファイルと歪み補正

- 入力は `calib-wom-msmf` が出力する JSON 形式とし、`camera matrix`（3×3）と
  `distortion`（5×1）を必須とします。
- 行列は一時領域へ読み込み、形状と全要素を検証してから現在値を置き換えます。
- 読み込み失敗時は補正方式を「なし」に戻し、不完全なパラメータを使用しません。
- JSON 配列から `cv::Mat` を作成するときは、行列サイズを指定する通常の
  コンストラクタを使用します。波括弧による初期化で initializer-list
  コンストラクタを誤選択しないよう注意します。
- OpenCV と GLSL は同じカメラ行列および 5 係数 `k1, k2, p1, p2, k3` を使用します。
- OpenGL 用 uniform へ渡すときは、画素単位の焦点距離と主点、および実際の
  入力解像度の関係を崩さないようにします。

## 6. シェーダーと構成

- 各 `Preference` は通常展開用の `shader` と、歪み補正用の `undistortion` を
  構成ファイルから読み込みます。
- `Default` を含む投影方式は、起動時に必要なシェーダーを構築します。
- 歪み補正パス（第 1 パス）の設定は `Menu::setupUndistortion()` を使用し、`undistortion.vert` と `undistortion.frag` を設定します。
- 展開パス（第 2 パス）の設定は `Menu::setup()` を使用し、現在選択されている投影方式の展開シェーダー（`preference.getShader()`）を設定します。
- 両シェーダーで共通の設定処理を使用します。存在しない uniform の location が
  `-1` の場合に OpenGL が更新を無視する性質を利用し、不要な分岐を増やしません。
- 構成の読み込みに失敗した場合は、使用可能な既定シェーダーを維持します。

## 7. UI と状態管理

- ファイル選択には Native File Dialog Extended を使用します。
- 較正ファイルの読み込みは「ファイル」メニューから行います。
- 歪み補正方式は「なし」「OpenCV」「OpenGL」のラジオボタンで選択します。
- 較正値が未読み込みの場合は OpenCV／OpenGL 補正へ遷移させません。
- `Menu::draw()` は UI の描画と入力受付を担当し、実際の画像処理は
  `mfcapture.cpp`、`Undistortion`、シェーダーへ委譲します。
- 同じ状態遷移を複数の UI ブロックへ重複実装せず、補助関数へ集約します。
- `const_cast` や `friend` によるクラスの不変条件迂回を一切禁止し、`getSettings()` や `setSettings()` などの明示的な公開 API を介して状態変更と連携を行います。
- メンバ変数の初期化はコンストラクタの初期化子リストではなくクラス定義（ヘッダ内）のデフォルトメンバ初期化構文（インクラス初期化）へ集約します。

## 8. リソース管理と安全性

- COM オブジェクト、OpenGL オブジェクト、MFT バッファ、libcamera 要求オブジェクトは RAII または明示的な対称処理で解放し、早期 return や `continue` でも漏らしません。
- コピー長は入力と出力の実容量から決め、バッファ境界を越えないようにします。
- 画像サイズが 0、較正値が未設定、シェーダー構築失敗などの状態を公開関数の
  入口で検査します。
- ファイル読み込みは、成功した部分だけが現在状態へ残る更新を行いません。

## 9. コメントと Doxygen

- コメントにはコードの逐語的な説明ではなく、「何のために」「どの状態を保つために」
  その処理を行うかを記述します。
- 教材として処理単位を追えるよう、ファイル読み込み、検証、キャッシュ更新、
  CPU／GPU 転送、座標変換の各ブロックに説明を付けます。
- `calib-wom-msmf` と `mfcapture` 間で共通する変数名・関数名は `mfcapture` の命名に統一し、コメントおよび Doxygen の表現スタイルは `calib-wom-msmf` に統一します。
- 公開型と公開関数、重要な非公開関数には Doxygen コメントを付けます。
- `@param` は宣言の引数名と一致させ、戻り値がある関数には `@return` を記述します。
- 実装を変更したときは、コメント、Doxygen、README、必要なら REQUESTS を同時に
  更新します。
- Doxygen の引数名不一致、未記載引数、未知のコマンドに関する警告を残しません。

## 10. 検証方針

- Windows の Debug と Release の両構成をビルドします。
- `git diff --check` で差分の空白エラーを確認します。
- Doxygen を実行し、プロジェクトコードのコメント警告を確認します。
- 較正ファイルの正常系、必須行列欠落、行列サイズ不正を確認します。
- 同じ入力について、補正なし、OpenCV、OpenGL の切り替えを確認します。
- 入力解像度変更時だけ補正マップと Framebuffer が再構築されることを確認します。

## 11. Raspberry Pi / 組み込み環境対応方針

- **グラフィックス API (OpenGL ES 3.1)**:
  - Raspberry Pi 4/5 の GPU (VideoCore VI/VII) および Mesa ドライバに最適化するため、OpenGL ES 3.1 (`GL_GLES_PROTOTYPES`, `IMGUI_IMPL_OPENGL_ES3`) をサポートします。
  - CMake オプション `USE_GLES` を提供し、ARM 環境 (`arm|aarch64`) では既定値を `ON` とします（デスクトップ OpenGL への切り替えも可能）。
- **シェーダーコードの統一と GLES 前処理**:
  - シェーダーファイル自体の宣言は Desktop OpenGL 3.3 準拠の `#version 330` に統一します。
  - `ggCreateShader` において、`GL_GLES_PROTOTYPES` 有効時は先頭のバージョン宣言を `#version 310 es` に置換し、精度修飾子（`precision highp float; precision highp int;`）を自動付与します。これにより、シェーダーファイルの複製・二重管理を防ぎます。
- **カメラ入力 (libcamera および V4L2 サポート)**:
  - Raspberry Pi のネイティブカメラスタックとして `CamLibcam` (libcamera バックエンド) を提供します。
  - Linux 環境における汎用 UVC カメラ入力バックエンドとして `cv::CAP_V4L2` を追加します。
  - `/sys/class/video4linux` を走査して接続されたカメラデバイスの一覧と実際のデバイス番号を取得し、SoC 内部処理ノード（bcm2835-codec, bcm2835-isp, pisp 等）を除外した上で、USB カメラおよび Raspberry Pi Camera Module を選択可能にします。
- **アセット・リソースの配置**:
  - Linux 環境でも POST_BUILD コマンドにより、シェーダー、構成ファイル、画像アセットを実行バイナリディレクトリへ自動配置します。

## 12. OpenXR サポート方針

- `GgApp::OpenXR` により、VR / MR ヘッドセットへのステレオ展開出力をサポートします。
- OpenXR API への依存は `GgApp` 内にカプセル化し、メインアプリケーションや画像パイプラインの独立性を保ちます。
