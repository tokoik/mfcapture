#pragma once

///
/// 歪補正処理クラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date July 27, 2027
///

// OpenCV による画像処理
#include "opencv_link.h"

// 標準ライブラリ
#include <array>
#include <string>

///
/// 入力画像のレンズ歪みを補正する方法
///
enum class UndistortionMode
{
  None,   ///< 歪み補正を行わない
  OpenCV, ///< CPU 上で OpenCV の remap() を使って補正する
  OpenGL  ///< GPU 上で GLSL のサンプリング座標を変換して補正する
};

///
/// calib が出力した内部パラメータを読み込み、レンズ歪みを補正するクラス
///
/// @details
/// 較正ファイルの "camera matrix" と "distortion" を保持する。
/// OpenCV 方式では補正マップを作って cv::remap() に渡し、OpenGL 方式では
/// 同じ値を GLSL の uniform へ渡せるよう float 配列へ変換する。
///
class Undistortion
{
  /// カメラ行列 K（3 行 3 列、要素型 CV_64F）
  cv::Mat cameraMatrix;

  /// 歪み係数 (k1, k2, p1, p2, k3)
  cv::Mat distortion;

  /// cv::remap() が参照する入力画像の x 座標表
  cv::Mat mapX;

  /// cv::remap() が参照する入力画像の y 座標表
  cv::Mat mapY;

  /// 現在の補正マップを作成した画像サイズ
  cv::Size mapSize;

public:

///
  /// calib が作成した較正ファイルを読み込む
  ///
  /// @param filename 読み込む JSON ファイルのパス
  /// @return カメラ行列と 5 個の歪み係数を読み込めたら true
  ///
  bool load(const std::string& filename);

  ///
  /// 補正に必要なパラメータが読み込まれているか調べる
  ///
  /// @return カメラ行列と歪み係数が揃っていれば true
  ///
  bool ready() const;

  ///
  /// OpenCV を使って入力画像のレンズ歪みを補正する
  ///
  /// @param source 補正前の入力画像
  /// @param destination 補正後の画像を格納する cv::Mat
  ///
  /// @details
  /// 入力サイズが変わったときだけ補正マップを作り直し、通常フレームでは
  /// 作成済みのマップを cv::remap() で再利用する。
  ///
  void apply(const cv::Mat& source, cv::Mat& destination);

  ///
  /// GLSL に渡すカメラ行列の主要成分を得る
  ///
  /// @return (fx, fy, cx, cy) の順に格納した配列
  ///
  std::array<float, 4> getCameraParameters() const;

  ///
  /// GLSL に渡す歪み係数を得る
  ///
  /// @return (k1, k2, p1, p2, k3) の順に格納した配列
  ///
  std::array<float, 5> getDistortionParameters() const;
};
