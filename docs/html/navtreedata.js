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
    [ "実践カメラキャリブレーション &amp; レンズ歪み補正 スライド構成", "md_presentation.html", [
      [ "1. アジェンダ", "md_presentation.html#autotoc_md2", [
        [ "前半: 理論と基礎", "md_presentation.html#autotoc_md3", null ],
        [ "後半: 実装とハンズオン", "md_presentation.html#autotoc_md4", null ]
      ] ],
      [ "2. カメラモデリングの基礎原理", "md_presentation.html#autotoc_md6", [
        [ "ピンホールモデルと透視投影 (Perspective Projection)", "md_presentation.html#autotoc_md7", null ]
      ] ],
      [ "3. レンズ歪みの数理モデル", "md_presentation.html#autotoc_md9", [
        [ "1. 放射歪み (Radial Distortion)", "md_presentation.html#autotoc_md10", null ],
        [ "2. 接線歪み (Tangential Distortion)", "md_presentation.html#autotoc_md11", null ]
      ] ],
      [ "4. ChArUco Board によるキャリブレーション", "md_presentation.html#autotoc_md13", null ],
      [ "4.1 キャリブレーションで推定するもの", "md_presentation.html#autotoc_md15", null ],
      [ "4.2 C++ 実装：ChArUco Board の定義と構成", "md_presentation.html#autotoc_md17", [
        [ "ChArUco Board とは (OpenCV チュートリアル準拠)", "md_presentation.html#autotoc_md18", null ],
        [ "<span class=\"tt\">cv::aruco::CharucoBoard</span> 引数解説", "md_presentation.html#autotoc_md19", null ]
      ] ],
      [ "4.3 C++ 実装：検出と標本座標マッチング", "md_presentation.html#autotoc_md21", [
        [ "処理概略と <span class=\"tt\">corners.size() &gt;= 4</span> の理由", "md_presentation.html#autotoc_md22", null ],
        [ "関数の引数解説", "md_presentation.html#autotoc_md23", null ]
      ] ],
      [ "4.4 C++ 実装：最適化計算 (<span class=\"tt\">calibrateCamera</span>)", "md_presentation.html#autotoc_md25", [
        [ "「最適化計算」の意味", "md_presentation.html#autotoc_md26", null ]
      ] ],
      [ "5. 全体システムアーキテクチャ", "md_presentation.html#autotoc_md28", null ],
      [ "6. 歪み補正パイプラインの比較 (CPU vs GPU)", "md_presentation.html#autotoc_md30", null ],
      [ "7. OpenCV (CPU) による C++ 歪み補正処理", "md_presentation.html#autotoc_md32", null ],
      [ "8. GLSL 歪み補正シェーダー (<span class=\"tt\">undistortion.frag</span>)", "md_presentation.html#autotoc_md34", null ],
      [ "9. なぜ「逆向き」に座標を求めるのか (逆写像)", "md_presentation.html#autotoc_md36", null ],
      [ "10. C++ クラス設計と安全なカプセル化", "md_presentation.html#autotoc_md38", null ],
      [ "11. Windows Media Foundation (MSMF) による低遅延キャプチャ", "md_presentation.html#autotoc_md40", null ],
      [ "12. まとめ", "md_presentation.html#autotoc_md42", null ]
    ] ],
    [ "mfcapture", "md_README.html", [
      [ "概要", "md_README.html#autotoc_md44", null ],
      [ "主な機能", "md_README.html#autotoc_md45", null ],
      [ "プログラムの処理の流れ", "md_README.html#autotoc_md46", null ],
      [ "レンズ歪み補正", "md_README.html#autotoc_md47", [
        [ "較正ファイルの読み込み", "md_README.html#autotoc_md48", null ],
        [ "OpenCV 方式 (CPU 補正)", "md_README.html#autotoc_md49", null ],
        [ "OpenGL 方式 (GPU 補正)", "md_README.html#autotoc_md50", null ]
      ] ],
      [ "主要クラスと責務", "md_README.html#autotoc_md51", [
        [ "<span class=\"tt\">Camera</span> と入力実装", "md_README.html#autotoc_md52", null ],
        [ "<span class=\"tt\">Config</span>、<span class=\"tt\">Preference</span>、<span class=\"tt\">Intrinsics</span>", "md_README.html#autotoc_md53", null ],
        [ "<span class=\"tt\">Undistortion</span>", "md_README.html#autotoc_md54", null ],
        [ "<span class=\"tt\">Menu</span>", "md_README.html#autotoc_md55", null ]
      ] ],
      [ "Windowsでの低遅延キャプチャ", "md_README.html#autotoc_md56", null ],
      [ "macOSでの低遅延キャプチャ", "md_README.html#autotoc_md57", null ],
      [ "基本操作", "md_README.html#autotoc_md58", null ],
      [ "構成ファイル", "md_README.html#autotoc_md59", null ],
      [ "開発環境とビルド", "md_README.html#autotoc_md60", [
        [ "macOS でのビルド例", "md_README.html#autotoc_md61", null ],
        [ "Raspberry Pi (Linux ARM) でのビルド例", "md_README.html#autotoc_md62", null ],
        [ "Android スマートフォンでのビルド例", "md_README.html#autotoc_md63", null ]
      ] ],
      [ "開発時の確認事項", "md_README.html#autotoc_md64", null ],
      [ "ドキュメント・関連資料", "md_README.html#autotoc_md65", [
        [ "開発・管理ドキュメント", "md_README.html#autotoc_md66", null ],
        [ "プラットフォーム・機能別ガイド (docs)", "md_README.html#autotoc_md67", null ],
        [ "勉強会プレゼンテーション・ハンドブック", "md_README.html#autotoc_md68", null ]
      ] ]
    ] ],
    [ "実践カメラキャリブレーション &amp; レンズ歪み補正 講義・実習ハンドブック", "md_workshop__handbook.html", [
      [ "第1章 開発環境とプロジェクト規約", "md_workshop__handbook.html#autotoc_md71", [
        [ "1.1 開発環境要件", "md_workshop__handbook.html#autotoc_md72", null ],
        [ "1.2 ソースコード文字コード規約", "md_workshop__handbook.html#autotoc_md73", null ]
      ] ],
      [ "第2章 カメラモデリングとレンズ歪みの数理", "md_workshop__handbook.html#autotoc_md75", [
        [ "2.1 ピンホールカメラモデル・内部パラメータ・外部パラメータ", "md_workshop__handbook.html#autotoc_md76", [
          [ "1. カメラ内部行列 (Camera Matrix $K$)", "md_workshop__handbook.html#autotoc_md77", null ],
          [ "2. カメラ外部行列 (Extrinsic Parameters $[R | t]$)", "md_workshop__handbook.html#autotoc_md78", null ]
        ] ]
      ] ],
      [ "第3章 ChArUco Board によるキャリブレーション原理", "md_workshop__handbook.html#autotoc_md80", [
        [ "3.1 ChArUco Board とは (OpenCV チュートリアル準拠)", "md_workshop__handbook.html#autotoc_md81", null ],
        [ "3.4 C++ 実装手順1：辞書、ボード、検出器を作る", "md_workshop__handbook.html#autotoc_md83", null ],
        [ "3.7 C++ 実装手順4：標本データを抽出・保存する", "md_workshop__handbook.html#autotoc_md85", null ],
        [ "3.8 C++ 実装手順5：内部パラメータを推定する (<span class=\"tt\">calibrateCamera</span>)", "md_workshop__handbook.html#autotoc_md87", null ]
      ] ],
      [ "第4章 プログラム設計とアーキテクチャ", "md_workshop__handbook.html#autotoc_md89", [
        [ "4.1 C++ クラス設計と安全なカプセル化", "md_workshop__handbook.html#autotoc_md90", null ]
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
"classFramebuffer.html#a5ed969e962332c9f108511e21b4e82c3",
"classgg_1_1GgMatrix.html#a0c9004fe440a597d57f52e1d1ac05c6a",
"classgg_1_1GgPoints.html#aa9170eea649cf940adc6698d98964bd4",
"classgg_1_1GgShader.html#afb49a96fa6fa7b981013ee783511b292",
"classgg_1_1GgTrackball.html#af0ff2b315542776b0b465f5e166c4c8a",
"md_presentation.html#autotoc_md21"
];

var SYNCONMSG = 'クリックで同期表示が無効になります';
var SYNCOFFMSG = 'クリックで同期表示が有効になります';
var LISTOFALLMEMBERS = '全メンバ一覧';