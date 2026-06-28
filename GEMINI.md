# プロジェクト開発方針と環境定義 (GEMINI.md)

本ドキュメントは、`mfcapture` プロジェクトにおけるビデオキャプチャモジュールの設計変更方針、および開発・動作環境に関する要件を定義します。

## 1. 開発環境・環境設定

- **統合開発環境 (IDE)**: Visual Studio 2022
- **開発言語**: C++17 (`/std:c++17`)
- **ターゲットアーキテクチャ**: 64bit (x64)
- **外部依存ライブラリ (OpenCV, GLFW, ImGui)**:
  - プロジェクトディレクトリ直下の `libs` ディレクトリ（Windowsのディレクトリジャンクション経由）に配置されているものを使用します。
  - インクルードパスおよびライブラリパスは、`libs/win/$(Platform)/` 以下を参照するように構成されています。
- **ファイル文字コード**:
  - **C++ ソースファイル（拡張子 `.h`, `.cpp`）**: 文字コードは `UTF-8` とし、**BOM を付与**します。
  - **GLSL ソースファイル（拡張子 `.vert`, `.frag`, `.comp`）**: 文字コードは `UTF-8` とし、**BOM は付与しません**。
- **デバッグ実行の環境設定**:
  - Visual Studio 2022 上で正常に実行・デバッグするために、プロジェクトの「プロパティ」→「デバッグ」→「環境」に以下の環境変数（DLL のパス）を追加します。
    `Path=$(ProjectDir)libs\win\$(Platform)\bin;$(Path)`

## 2. 設計およびリファクタリング方針

### 目的
RICOH THETA V などの H.264 出力Webカメラにおいて、OpenCV の `cv::VideoCapture` 経由ではキャプチャおよびデコードが著しく低速になる問題を解消するため、Microsoft Media Foundation（`CamMf`）を完全に OpenCV から独立させて高速にキャプチャ・デコード処理を行います。

### 具体的アプローチ
1. **Camera 基底クラスのデカップリング**:
   - `Camera.h` から OpenCV のインクルード (`#include <opencv2/opencv.hpp>`) を削除します。
   - `cv::Mat frame;` および `cv::Mat image;` を標準のバッファ `std::vector<GLubyte> frame;` および `std::vector<GLubyte> image;` に置き換えます。
   - 解像度やチャンネル情報を保持するメンバ変数 (`width`, `height`, `channels`) を追加し、ゲッターをこれらに基づくシンプルな実装にします。
   
2. **データの転送 (`transmit`)**:
   - 既存の `transmit(cv::Mat&)` は、`Camera.h` の OpenCV 依存を排除するためテンプレート関数 `template <typename MatType> void transmit(MatType& buffer)` に変更します。
   - `GLuint` (PBO用) および `std::vector<GLubyte>&` 用 of `transmit` オーバーロードは非テンプレート関数のまま残します。

3. **Media Foundation バックエンド (`CamMf`)**:
   - 内部のフレームバッファ処理において `cv::Mat` のアロケーションや `copyTo()` の呼び出しを排除し、Media Foundation のメモリバッファから `std::vector` へ直接 `memcpy` することで、デコードデータの転送を最適化します。

4. **OpenCV 依存バックエンド (`CamCv`, `CamImage`, `Calibration`) とリンク設定の分離**:
   - これら OpenCV 依存のモジュールについては、自動リンク設定 (`#pragma comment`) や C4819 警告抑制などを一括管理するための共通ヘッダ `opencv_link.h` を新設してインクルードします。
   - これにより、`Camera.h` および `CamMf` から OpenCV への結合を完全に切断（Pure Media Foundation キャプチャ化）しつつ、他の OpenCV 依存部での設定の重複を防ぎ、メンテナンス性を向上させます。

5. **ビルド構成**:
   - ビルド時の OpenCV リンク構成や他の OpenCV 依存モジュール（Calibration等）はそのまま維持します。
