#pragma once

///
/// ArUco Marker 認識クラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date March 6, 2024
///

// 補助プログラム
#include "gg.h"

// OpenCV ArUco (OpenCV 4.7+)
#include "opencv_link.h"
#include <opencv2/objdetect.hpp>

// 標準ライブラリ
#include <map>
#include <string>
#include <vector>

///
/// ArUco Marker 認識クラス
///
class Aruco
{
  /// ArUco Marker 辞書
  cv::aruco::Dictionary dictionary;

  /// ArUco Marker 検出器
  cv::Ptr<cv::aruco::ArucoDetector> detector;

  /// ArUco Marker の検出結果
  std::vector<std::vector<cv::Point2f>> corners, rejected;
  std::vector<int> ids;

public:

  /// ArUco Marker 辞書のリスト
  static const std::map<const std::string, const cv::aruco::PredefinedDictionaryType> dictionaryList;

  ///
  /// コンストラクタ
  ///
  /// @param dictionaryName ArUco Marker の辞書名
  ///
  explicit Aruco(const std::string& dictionaryName = "DICT_4X4_50");

  ///
  /// コピーコンストラクタは使用しない
  ///
  /// @param aruco コピー元
  ///
  Aruco(const Aruco& aruco) = delete;

  ///
  /// デストラクタ
  ///
  virtual ~Aruco() = default;

  ///
  /// 代入演算子は使用しない
  ///
  /// @param aruco 代入元
  ///
  Aruco& operator=(const Aruco& aruco) = delete;

  ///
  /// ArUco Marker の辞書と検出器を設定する
  ///
  /// @param dictionaryName ArUco Marker の辞書名
  ///
  void setDictionary(const std::string& dictionaryName);

  ///
  /// ArUco Marker を検出して画像に描画する
  ///
  /// @param image 検出対象および描画先の画像 (BGR または BGRA)
  /// @param markerLength ArUco Marker の一辺の長さ (単位 cm)
  /// @param cameraMatrix カメラの内部パラメータ行列 (3x3)。空行列の場合は枠と ID を描画する
  /// @param distCoeffs カメラのレンズ歪み係数。補正済み画像の場合は空行列またはゼロ
  ///
  void detectMarkers(cv::Mat& image, float markerLength,
    const cv::Mat& cameraMatrix = {}, const cv::Mat& distCoeffs = {});
};
