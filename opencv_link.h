#pragma once

///
/// @file opencv_link.h
/// @brief MSVC環境用のOpenCV自動リンクおよびインクルード一括管理ヘッダー
///
/// @details
/// カメラ基底クラス (Camera.h) および Media Foundation バックエンド (CamMf) を
/// OpenCVから完全に分離（疎結合化・Pure Media Foundationキャプチャ化）したことに伴い、
/// OpenCVを必要とする他のモジュール（CamCv, CamImageなど）における
/// 以下の処理を一括してカプセル化・再利用するために新設されました。
/// 
/// 1. MSVCコンパイラ向けの OpenCV ライブラリの自動リンク設定 (#pragma comment) の一括適用
/// 2. 日本語環境のMSVCで発生しやすい文字コード警告 C4819 (#pragma warning) の抑制
/// 3. opencv2/opencv.hpp のインクルード
///

// OpenCV
#pragma warning(disable:4819)
#include <opencv2/opencv.hpp>

#if defined(_WIN32)
#  define CV_VERSION_STR CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#  if defined(_DEBUG)
#    define CV_EXT_STR "d.lib"
#  else
#    define CV_EXT_STR ".lib"
#  endif
#  pragma comment(lib, "opencv_world" CV_VERSION_STR CV_EXT_STR)
#endif
