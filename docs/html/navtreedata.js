/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "mfcapture", "index.html", [
    [ "ゲームグラフィックス特論の宿題用補助プログラム GLFW3 版.", "index.html", null ],
    [ "プロジェクト開発方針と環境定義 (GEMINI.md)", "md_GEMINI.html", [
      [ "1. プロジェクトの目的", "md_GEMINI.html#autotoc_md1", null ],
      [ "2. 開発環境", "md_GEMINI.html#autotoc_md2", null ],
      [ "3. 入力とキャプチャ", "md_GEMINI.html#autotoc_md3", null ],
      [ "4. 画像処理パイプライン", "md_GEMINI.html#autotoc_md4", null ],
      [ "5. 較正ファイルと歪み補正", "md_GEMINI.html#autotoc_md5", null ],
      [ "6. シェーダーと構成", "md_GEMINI.html#autotoc_md6", null ],
      [ "7. UI と状態管理", "md_GEMINI.html#autotoc_md7", null ],
      [ "8. リソース管理と安全性", "md_GEMINI.html#autotoc_md8", null ],
      [ "9. コメントと Doxygen", "md_GEMINI.html#autotoc_md9", null ],
      [ "10. 検証方針", "md_GEMINI.html#autotoc_md10", null ],
      [ "11. Raspberry Pi / 組み込み環境対応方針", "md_GEMINI.html#autotoc_md11", null ],
      [ "12. OpenXR サポート方針", "md_GEMINI.html#autotoc_md12", null ],
      [ "13. Android スマートフォン対応方針", "md_GEMINI.html#autotoc_md13", null ],
      [ "14. macOS (AV Foundation) 対応方針", "md_GEMINI.html#autotoc_md14", null ]
    ] ],
    [ "実践カメラキャリブレーション &amp; レンズ歪み補正 スライド構成", "md_presentation.html", [
      [ "1. アジェンダ", "md_presentation.html#autotoc_md17", [
        [ "前半: 理論と基礎", "md_presentation.html#autotoc_md18", null ],
        [ "後半: 実装とハンズオン", "md_presentation.html#autotoc_md19", null ]
      ] ],
      [ "2. カメラモデリングの基礎原理", "md_presentation.html#autotoc_md21", [
        [ "ピンホールモデルと透視投影 (Perspective Projection)", "md_presentation.html#autotoc_md22", null ]
      ] ],
      [ "3. レンズ歪みの数理モデル", "md_presentation.html#autotoc_md24", [
        [ "1. 放射歪み (Radial Distortion)", "md_presentation.html#autotoc_md25", null ],
        [ "2. 接線歪み (Tangential Distortion)", "md_presentation.html#autotoc_md26", null ]
      ] ],
      [ "4. ChArUco Board によるキャリブレーション", "md_presentation.html#autotoc_md28", null ],
      [ "4.1 キャリブレーションで推定するもの", "md_presentation.html#autotoc_md30", null ],
      [ "4.2 C++ 実装：ChArUco Board の定義と構成", "md_presentation.html#autotoc_md32", [
        [ "ChArUco Board とは (OpenCV チュートリアル準拠)", "md_presentation.html#autotoc_md33", null ],
        [ "<span class=\"tt\">cv::aruco::CharucoBoard</span> 引数解説", "md_presentation.html#autotoc_md34", null ]
      ] ],
      [ "4.3 C++ 実装：検出と標本座標マッチング", "md_presentation.html#autotoc_md36", [
        [ "処理概略と <span class=\"tt\">corners.size() &gt;= 4</span> の理由", "md_presentation.html#autotoc_md37", null ],
        [ "関数の引数解説", "md_presentation.html#autotoc_md38", null ]
      ] ],
      [ "4.4 C++ 実装：最適化計算 (<span class=\"tt\">calibrateCamera</span>)", "md_presentation.html#autotoc_md40", [
        [ "「最適化計算」の意味", "md_presentation.html#autotoc_md41", null ]
      ] ],
      [ "5. 全体システムアーキテクチャ", "md_presentation.html#autotoc_md43", null ],
      [ "6. 歪み補正パイプラインの比較 (CPU vs GPU)", "md_presentation.html#autotoc_md45", null ],
      [ "7. OpenCV (CPU) による C++ 歪み補正処理", "md_presentation.html#autotoc_md47", null ],
      [ "8. GLSL 歪み補正シェーダー (<span class=\"tt\">undistortion.frag</span>)", "md_presentation.html#autotoc_md49", null ],
      [ "9. なぜ「逆向き」に座標を求めるのか (逆写像)", "md_presentation.html#autotoc_md51", null ],
      [ "10. C++ クラス設計と安全なカプセル化", "md_presentation.html#autotoc_md53", null ],
      [ "11. Windows Media Foundation (MSMF) による低遅延キャプチャ", "md_presentation.html#autotoc_md55", null ],
      [ "12. まとめ", "md_presentation.html#autotoc_md57", null ]
    ] ],
    [ "mfcapture", "md_README.html", [
      [ "概要", "md_README.html#autotoc_md59", null ],
      [ "主な機能", "md_README.html#autotoc_md60", null ],
      [ "プログラムの処理の流れ", "md_README.html#autotoc_md61", null ],
      [ "レンズ歪み補正", "md_README.html#autotoc_md62", [
        [ "較正ファイルの読み込み", "md_README.html#autotoc_md63", null ],
        [ "OpenCV 方式 (CPU 補正)", "md_README.html#autotoc_md64", null ],
        [ "OpenGL 方式 (GPU 補正)", "md_README.html#autotoc_md65", null ]
      ] ],
      [ "主要クラスと責務", "md_README.html#autotoc_md66", [
        [ "<span class=\"tt\">Camera</span> と入力実装", "md_README.html#autotoc_md67", null ],
        [ "<span class=\"tt\">Config</span>、<span class=\"tt\">Preference</span>、<span class=\"tt\">Intrinsics</span>", "md_README.html#autotoc_md68", null ],
        [ "<span class=\"tt\">Undistortion</span>", "md_README.html#autotoc_md69", null ],
        [ "<span class=\"tt\">Menu</span>", "md_README.html#autotoc_md70", null ]
      ] ],
      [ "Windowsでの低遅延キャプチャ", "md_README.html#autotoc_md71", null ],
      [ "macOSでの低遅延キャプチャ", "md_README.html#autotoc_md72", null ],
      [ "基本操作", "md_README.html#autotoc_md73", null ],
      [ "構成ファイル", "md_README.html#autotoc_md74", null ],
      [ "開発環境とビルド", "md_README.html#autotoc_md75", [
        [ "macOS でのビルド例", "md_README.html#autotoc_md76", null ],
        [ "Raspberry Pi (Linux ARM) でのビルド例", "md_README.html#autotoc_md77", null ],
        [ "Android スマートフォンでのビルド例", "md_README.html#autotoc_md78", null ]
      ] ],
      [ "開発時の確認事項", "md_README.html#autotoc_md79", null ],
      [ "ドキュメント・関連資料", "md_README.html#autotoc_md80", [
        [ "開発・管理ドキュメント", "md_README.html#autotoc_md81", null ],
        [ "モジュール解説ドキュメント", "md_README.html#autotoc_md82", null ],
        [ "勉強会プレゼンテーション・ハンドブック", "md_README.html#autotoc_md83", null ]
      ] ]
    ] ],
    [ "作業指示および対応履歴", "md_REQUESTS.html", [
      [ "概要", "md_REQUESTS.html#autotoc_md85", null ],
      [ "作業履歴", "md_REQUESTS.html#autotoc_md86", [
        [ "1. <span class=\"tt\">CamMf</span> のバックポート", "md_REQUESTS.html#autotoc_md87", null ],
        [ "2. CMake ビルドと依存ライブラリ管理", "md_REQUESTS.html#autotoc_md88", null ],
        [ "3. 較正機能と ChArUco Board 作成機能の削除", "md_REQUESTS.html#autotoc_md89", null ],
        [ "4. ソーストップディレクトリの整理", "md_REQUESTS.html#autotoc_md90", null ],
        [ "5. 描画ループ内の <span class=\"tt\">Framebuffer::resize()</span> の確認", "md_REQUESTS.html#autotoc_md91", null ],
        [ "6. 較正ファイルによる歪み補正の追加", "md_REQUESTS.html#autotoc_md92", null ],
        [ "7. 較正ファイル読み込み時の例外修正", "md_REQUESTS.html#autotoc_md93", null ],
        [ "8. 教材向けコメントと Doxygen の整備", "md_REQUESTS.html#autotoc_md94", null ],
        [ "9. プロジェクト文書の整備", "md_REQUESTS.html#autotoc_md95", null ],
        [ "10. クラスメンバ変数の初期化の集約", "md_REQUESTS.html#autotoc_md96", null ],
        [ "11. <span class=\"tt\">const_cast</span> および <span class=\"tt\">friend</span> の完全廃止と公開 API の採用", "md_REQUESTS.html#autotoc_md97", null ],
        [ "12. 共通処理における命名規約・コメントの統一とドキュメント同期", "md_REQUESTS.html#autotoc_md98", null ],
        [ "13. GStreamer 関連コードの削除", "md_REQUESTS.html#autotoc_md99", null ],
        [ "14. Raspberry Pi (<span class=\"tt\">CamLibcam</span>) および Linux ARM (GLES 3.1) のサポート", "md_REQUESTS.html#autotoc_md100", null ],
        [ "15. <span class=\"tt\">GgApp::OpenXR</span> への統合と同期", "md_REQUESTS.html#autotoc_md101", null ],
        [ "16. レンズ歪み補正の 2 パス描画パイプライン統合 (OpenCV / OpenGL 結果・アスペクト比の完全一致)", "md_REQUESTS.html#autotoc_md102", null ],
        [ "17. CamMf.md / CamLibcam.md の calib への移行と Camera クラスの再設計・最適化", "md_REQUESTS.html#autotoc_md103", null ],
        [ "18. プログラム終了時の純粋仮想関数呼び出し例外の解消", "md_REQUESTS.html#autotoc_md104", null ],
        [ "19. Android スマートフォン対応", "md_REQUESTS.html#autotoc_md105", null ],
        [ "20. macOS における AV Foundation (<span class=\"tt\">CamAvf</span>) ネイティブカメラキャプチャ対応とインタフェース統一", "md_REQUESTS.html#autotoc_md106", null ],
        [ "21. macOS ビルドエラーの解消、Homebrew 非依存の自己完結化、およびカメラ名サニタイズとグリフ拡張", "md_REQUESTS.html#autotoc_md107", null ],
        [ "22. ドキュメントの整理、LaTeX 特殊文字エラー解消、および Doxygen マニュアル (HTML/PDF) 作成", "md_REQUESTS.html#autotoc_md108", null ]
      ] ]
    ] ],
    [ "実践カメラキャリブレーション &amp; レンズ歪み補正 講義・実習ハンドブック", "md_workshop__handbook.html", [
      [ "第1章 開発環境とプロジェクト規約", "md_workshop__handbook.html#autotoc_md111", [
        [ "1.1 開発環境要件", "md_workshop__handbook.html#autotoc_md112", null ],
        [ "1.2 ソースコード文字コード規約", "md_workshop__handbook.html#autotoc_md113", null ]
      ] ],
      [ "第2章 カメラモデリングとレンズ歪みの数理", "md_workshop__handbook.html#autotoc_md115", [
        [ "2.1 ピンホールカメラモデル・内部パラメータ・外部パラメータ", "md_workshop__handbook.html#autotoc_md116", [
          [ "1. カメラ内部行列 (Camera Matrix $K$)", "md_workshop__handbook.html#autotoc_md117", null ],
          [ "2. カメラ外部行列 (Extrinsic Parameters $[R | t]$)", "md_workshop__handbook.html#autotoc_md118", null ]
        ] ]
      ] ],
      [ "第3章 ChArUco Board によるキャリブレーション原理", "md_workshop__handbook.html#autotoc_md120", [
        [ "3.1 ChArUco Board とは (OpenCV チュートリアル準拠)", "md_workshop__handbook.html#autotoc_md121", null ],
        [ "3.4 C++ 実装手順1：辞書、ボード、検出器を作る", "md_workshop__handbook.html#autotoc_md123", null ],
        [ "3.7 C++ 実装手順4：標本データを抽出・保存する", "md_workshop__handbook.html#autotoc_md125", null ],
        [ "3.8 C++ 実装手順5：内部パラメータを推定する (<span class=\"tt\">calibrateCamera</span>)", "md_workshop__handbook.html#autotoc_md127", null ]
      ] ],
      [ "第4章 プログラム設計とアーキテクチャ", "md_workshop__handbook.html#autotoc_md129", [
        [ "4.1 C++ クラス設計と安全なカプセル化", "md_workshop__handbook.html#autotoc_md130", null ]
      ] ]
    ] ],
    [ "名前空間", "namespaces.html", [
      [ "名前空間一覧", "namespaces.html", "namespaces_dup" ],
      [ "名前空間メンバ", "namespacemembers.html", [
        [ "全て", "namespacemembers.html", null ],
        [ "関数", "namespacemembers_func.html", null ],
        [ "変数", "namespacemembers_vars.html", null ],
        [ "列挙型", "namespacemembers_enum.html", null ],
        [ "列挙値", "namespacemembers_eval.html", null ]
      ] ]
    ] ],
    [ "クラス", "annotated.html", [
      [ "クラス一覧", "annotated.html", "annotated_dup" ],
      [ "クラス索引", "classes.html", null ],
      [ "クラス階層", "hierarchy.html", "hierarchy" ],
      [ "クラスメンバ", "functions.html", [
        [ "全て", "functions.html", "functions_dup" ],
        [ "関数", "functions_func.html", "functions_func" ],
        [ "変数", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "ファイル", "files.html", [
      [ "ファイル一覧", "files.html", "files_dup" ],
      [ "ファイルメンバ", "globals.html", [
        [ "全て", "globals.html", null ],
        [ "関数", "globals_func.html", null ],
        [ "型定義", "globals_type.html", null ],
        [ "列挙型", "globals_enum.html", null ],
        [ "マクロ定義", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"Aruco_8cpp.html",
"classFramebuffer.html#ae52075be4a114e941309bd12f1adf84c",
"classgg_1_1GgMatrix.html#a1411224c2d99609e2a9c8753903df624",
"classgg_1_1GgQuaternion.html#a014d3ef2b47e5981860388ef3119077a",
"classgg_1_1GgShape.html#a860e6671d9d80295599ed2cf9f2449bc",
"classgg_1_1GgTriangles.html#a5482f2023b35d5937f7a4b6da3501d44",
"md_REQUESTS.html#autotoc_md103",
"structgg_1_1GgVertex.html#af1839d03faeafaaa6e812fb0387535b6"
];

var SYNCONMSG = 'クリックで同期表示が無効になります';
var SYNCOFFMSG = 'クリックで同期表示が有効になります';
var LISTOFALLMEMBERS = '全メンバ一覧';