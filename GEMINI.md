# プロジェクト開発方針と環境定義 (GEMINI.md)

本ドキュメントは、`mfcapture` の設計方針、実装上維持すべき条件、および開発・動作環境を定義します。変更の経緯は [REQUESTS.md](REQUESTS.md) に記録します。

## 1. プロジェクトの目的

- 画像処理プログラミングの勉強会で使用できる、読みやすいサンプルを作成します。
- 入力画像に対する処理の位置と、CPU（OpenCV）と GPU（OpenGL／GLSL）の役割の違いをコードから追える構成にします。
- 本プログラムの責務は、画像の取得、GLSL による展開、既存の較正結果による歪み補正、ArUco Marker の検出と姿勢表示に限定します。
- カメラ較正、ChArUco Board の作成、標本取得、較正値の算出は実装しません（これらは `calib` が担当します）。

## 2. 開発環境

- **統合開発環境**: Visual Studio 2022 以降（Windows）、VS Code / Clang（macOS）、VS Code / GCC（Linux）、Android Studio（Android）
- **開発言語**: C++17
- **ターゲット**: 64 bit（x64 / arm64 / aarch64）
- **ビルドシステム**: CMake 3.13 以降。生成物はソース直下へ置かず、out-of-source build とします。
- **外部依存ライブラリ**: CMake がプロジェクト直下の `libs` へ自動取得します。`libs` を別ディレクトリへのジャンクションにしません。インクルードパス、ライブラリパス、DLL コピー、Visual Studio のデバッグ環境は `CMakeLists.txt` に集約します。
- **文字コード**:
  - C++ / Objective-C++ ソース（`.h`, `.cpp`, `.mm`）: UTF-8、**BOM 付き**
  - GLSL ソース（`.vert`, `.frag`, `.comp`）: UTF-8、**BOM なし**
  - pdfLaTeX（CJKutf8）で Doxygen の PDF を生成できるよう、コメントに丸数字・特殊引用符・商標記号などの特殊 Unicode 文字を使いません。

## 3. 入力とキャプチャ

入力部（`Camera`、`Capture`、各 `Cam*` クラス）は `calib` と同一のファイルを使用します。詳細な方針は `calib` の GEMINI.md 第 2 節と共通です。

| 入力 | クラス |
| --- | --- |
| Windows のカメラ | `CamMf`（Media Foundation + MFT デコーダ／カラーコンバータ） |
| macOS のカメラ | `CamAvf`（AV Foundation、BGRA 出力） |
| Android のカメラ | `CamAndroid`（Camera2 NDK、RGBA_8888） |
| Raspberry Pi のカメラ | `CamLibcam`（libcamera） |
| その他のカメラ・動画 | `CamCv`（OpenCV、Linux では `cv::CAP_V4L2`） |
| 静止画像 | `CamImage` |

- GStreamer パイプライン入力はサポート対象外とし、構成ファイルや UI に GStreamer 固有の設定を追加しません。
- `Camera.h` は OpenGL、OpenCV、GLFW に依存せず標準 C++ ライブラリだけで構成します。NVI パターンで `start()`, `stop()`, `close()` がライフサイクルを管理し、派生クラスは `onStart()`, `onStop()`, `onClose()` だけを実装します。`close()` は `stop()` を含むため、呼び出し側で続けて呼びません。
- フレームは単一バッファに保持し、`lockFrame()` の非ブロッキングロックでコールバックへ渡して、PBO への直接転送や `cv::Mat` へのコピーを行います。
- 上位層は `dynamic_cast` を使わず、`isStillImage()`, `getFormatList()`, `selectFormat()` 等の仮想関数で連携します。
- フォーマット列挙時には重い初期化を行わず、「開始」時に選択フォーマットを適用します。フォーマット情報は `CaptureFormat` で受け渡し、表示文字列を再解析しません。
- 未選択時の既定フォーマット（1280 x 720 優先）は `Menu::updateFormatDropdowns()`、ドロップダウン間の同期は `Menu::selectFormatItem()` に集約します。
- レイテンシ優先時は古いフレームを破棄し、全フレーム処理時は取得前のフレームを上書きしません。
- スレッド間で共有する状態は atomic、mutex、condition variable を用途に応じて使い、データレースと待機漏れを防ぎます。

## 4. 画像処理パイプライン

一フレームは、入力取得、第 1 パス（歪み補正）、第 2 パス（Preference 展開）、最終表示の順に処理します。

- **第 1 パス（歪み補正）**:
  - OpenCV 補正はキャプチャ直後、OpenGL テクスチャへ転送する前に `cv::remap()` で行います。GPU から CPU への読み戻しを追加しません。
  - OpenGL 補正は生画像テクスチャを入力とし、中間フレームバッファ `undistortedFramebuffer` へ `undistortion.vert` と `undistortion.frag` で描画します。
  - 補正なしの場合は、生画像テクスチャをそのまま第 2 パスへ渡します。
  - 補正なしと OpenGL 補正では PBO を使う高速な転送経路を維持します。
- **第 2 パス（Preference 展開）**: 補正方式に関わらず、第 1 パスの結果を共通の展開シェーダー（`orthographic.vert` + `normal.frag` など）で `framebuffer` へ展開します。これにより CPU 補正と GPU 補正で同じ画角、焦点距離、回転、アスペクト比が保証されます。
- **最終表示**: `framebuffer.draw(window.getFboWidth(), window.getFboHeight())` で実 Framebuffer サイズに基づいて縦横比を保った中央表示（contain 方式）を行います。
- `Undistortion` の補正マップは入力サイズが変化した場合だけ再構築します。`Framebuffer::resize()` は毎フレーム呼べますが、同じサイズでは GPU リソースを再確保しません。
- CPU 側のフレーム形式と GLSL が期待する色成分の対応を変更する場合は、最終表示シェーダーまで含めて確認します。

## 5. 較正ファイルと歪み補正

- 入力は `calib` が出力する JSON 形式とし、`camera matrix`（3×3）と `distortion`（5×1）を必須とします。較正時の入力解像度（`size`）が記録されていれば、読み込み後に最も近いカメラ解像度へ切り替えます（`Menu::selectBestResolution()`）。
- 行列は一時領域へ読み込み、形状と全要素を検証してから現在値を置き換えます。読み込みに失敗したときは補正方式を「なし」に戻し、不完全なパラメータを使いません。
- JSON 配列から `cv::Mat` を作るときは、行列サイズを指定するコンストラクタを丸括弧で呼びます。波括弧では initializer-list コンストラクタが選ばれるため使いません。
- OpenCV と GLSL は同じカメラ行列と 5 係数 `k1, k2, p1, p2, k3` を使います。uniform へ渡すときは、画素単位の焦点距離と主点、実際の入力解像度の関係を崩さないようにします。

## 6. ArUco Marker の検出

- `Aruco` クラスが辞書の管理、マーカー検出、姿勢推定と座標軸の描画を担当します。
- 歪み補正なしでは生画像に対して、較正値があればカメラ行列と歪み係数を渡して姿勢を推定します。
- OpenCV 補正・OpenGL 補正では補正後の画像に対して検出し、歪みは除去済みのため歪み係数を空行列として渡します。
- OpenGL 補正時は `undistortedFramebuffer` を PBO 経由で CPU へ読み出して検出し、描画結果を書き戻します。検出しないときは読み戻しを行いません。

## 7. シェーダーと構成

- 各 `Preference` は通常展開用の `shader` と歪み補正用の `undistortion` を構成ファイルから読み込み、起動時に構築します。
- 歪み補正パスは `Menu::setupUndistortion()`、展開パスは `Menu::setup()` で設定します。両者で共通の設定処理を使い、存在しない uniform の location が `-1` のとき OpenGL が更新を無視する性質を利用して分岐を増やしません。
- 構成の読み込みに失敗した場合は、使用可能な既定シェーダーを維持します。

## 8. UI と状態管理

- ファイル選択には Native File Dialog Extended を使用し、較正ファイルは「ファイル」メニューから読み込みます。
- 歪み補正方式は「なし」「OpenCV」「OpenGL」のラジオボタンで選択し、較正値が未読み込みの場合は OpenCV／OpenGL 補正へ遷移させません。
- `Menu::draw()` は UI の描画と入力受付を担当し、画像処理は `mfcapture.cpp`、`Undistortion`、`Aruco`、シェーダーへ委譲します。
- 同じ状態遷移を複数の UI ブロックへ重複実装せず、補助関数へ集約します。キャプチャ開始は `Menu::startCapture()`、デバイスのオープンと初期画角の設定は `Menu::openDevice()` に集約します。
- `const_cast` や `friend` でクラスの不変条件を迂回せず、`getSettings()` や `setSettings()` などの公開 API を使います。
- メンバ変数の初期値はクラス定義内のデフォルトメンバ初期化子に集約します。静的メンバの定義は、そのクラスの実装ファイルに置きます。
- `calib` と共通する変数名・関数名は `mfcapture` の命名に、コメントおよび Doxygen の表現は `calib` に統一します。共通ファイルは両プロジェクトで同一内容に保ちます。

## 9. リソース管理と安全性

- COM オブジェクト、OpenGL オブジェクト、MFT バッファ、libcamera の要求オブジェクトは RAII または明示的な対称処理で解放し、早期 return や `continue` でも漏らしません。
- コピー長は入力と出力の実容量から決め、バッファ境界を越えないようにします。
- 画像サイズが 0、較正値が未設定、シェーダー構築失敗などの状態を公開関数の入口で検査します。
- ファイル読み込みでは、成功した部分だけが現在状態へ残る更新を行いません。

## 10. プラットフォーム対応

- **Raspberry Pi / Linux**: CMake オプション `USE_GLES`（OpenGL ES 3.1）と `USE_LIBCAMERA` を提供し、ARM 環境では既定で有効にします。シェーダーは `#version 330` で記述し、GLES 有効時は `ggCreateShader()` が `#version 310 es` への置換と精度修飾子の付与を行います。`/sys/class/video4linux` を走査し、SoC 内部処理ノード（bcm2835-codec, bcm2835-isp, pisp 等）を除いたカメラを選択肢にします。
- **macOS**: `CamAvf` を使用し、`AVFoundation` と `CoreMedia` をリンクします。OpenCV のビルド設定で Homebrew 由来の外部依存の探索を無効化し、組み込み 3rdparty ライブラリ（ZLIB, JPEG, PNG, TIFF, WEBP）を強制して自己完結ビルドとします。
- **デバイス名とグリフ**: カメラ名は `CamMf` / `CamAvf` の `sanitizeDeviceName()` で正規化し、ImGui には一般句読点、文字様記号、矢印、囲み英数字、幾何学模様のグリフ範囲を追加登録します。
- **OpenXR**: `GgApp::OpenXR` が VR / MR ヘッドセットへの出力を担い、OpenXR API への依存を `GgApp` 内に閉じ込めます。
- **Android**:
  - UI は Jetpack Compose（`MainActivity.kt`）とし、ネイティブ層は EGL / OpenGL ES / ImGui に依存しません。`NativeBridge.cpp` が `ANativeWindow` へ CPU 直接転送で描画し、縦横比を保った中央配置にします。
  - Android 版の起動とフレーム処理は `NativeBridge.cpp` が担い、`mfcapture.cpp` と `main.cpp` はビルドしません。デスクトップ専用コードは `#if !defined(__ANDROID__)` で分離します。
  - 歪み補正方式の切り替え、ArUco Marker 検出の有効／無効と辞書・マーカー長、解像度の選択、較正ファイルの読み込みは JNI を介して C++ 側と同期します。入出力はアプリ内部ストレージ（`context.filesDir`）を起点とし、APK の `assets/` は起動時に展開します。
  - `android/` の Gradle プロジェクトからトップレベルの `CMakeLists.txt` を参照し、OpenCV Android SDK 4.11.0 を自動取得して `OpenCV_LIBS`, `android`, `log`, `camera2ndk`, `mediandk` だけをリンクします。

## 11. コメントと Doxygen

- コメントにはコードの言い換えではなく、「何のために」「どの状態を保つために」その処理を行うかを書きます。
- 教材として処理単位を追えるよう、ファイル読み込み、検証、キャッシュ更新、CPU／GPU 転送、座標変換の各ブロックに説明を付けます。
- 公開型と公開関数、重要な非公開関数に Doxygen コメントを付け、`@param` は宣言の引数名と一致させ、戻り値がある関数には `@return` を書きます。
- 実装を変更したときは、コメント、Doxygen、README、必要なら REQUESTS を同時に更新します。
- ソースコードに関する Doxygen 警告（引数名の不一致、未記載引数、未知のコマンド）を残しません。

## 12. 検証方針

- Windows の Debug と Release の両構成をビルドします。
- 共通ファイルや `Menu`、`Capture` を変更したときは、Android の Debug APK（`android\gradlew.bat assembleDebug`）もビルドします。
- `git diff --check` で差分の空白エラーを確認します。
- Doxygen を実行し、ソースコードのコメント警告がないことを確認します。
- C++ ソースの BOM 付き UTF-8、GLSL ソースの BOM なし UTF-8 を確認します。
- 較正ファイルの正常系、必須行列の欠落、行列サイズの不正を確認します。
- 同じ入力について、補正なし、OpenCV、OpenGL の切り替えで表示が一致することを確認します。
- 入力解像度の変更時だけ補正マップと Framebuffer が再構築されることを確認します。
