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
/// 較正時の画像サイズと入力画像のサイズの関係
///
enum class CalibrationSizeMatch
{
  Unknown,       ///< 較正ファイルに画像サイズが記録されていないので換算しない
  Exact,         ///< 較正時と同じ解像度
  Scaled,        ///< アスペクト比は同じで解像度が違うので、カメラ行列を換算して使う
  AspectMismatch ///< アスペクト比が違うので、換算しても正しく補正できない
};

///
/// calib が出力した内部パラメータを読み込み、レンズ歪みを補正するクラス
///
/// @details
/// 較正ファイルの "camera matrix" と "distortion" を保持する。
/// OpenCV 方式では補正マップを作って cv::remap() に渡し、OpenGL 方式では
/// 同じ値を GLSL の uniform へ渡せるよう float 配列へ変換する。
///
/// カメラ行列は較正時の画像の画素単位なので、較正時と解像度が違う入力には
/// 解像度の比で換算して使う。歪み係数は正規化したカメラ座標で定義されているので、
/// 解像度によらずそのまま使える。
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
  cv::Size mapSize{ 0, 0 };



  /// 較正ファイルに記録された画像サイズ
  cv::Size imageSize{ 0, 0 };

public:

  ///
  /// アスペクト比が同じとみなす相対誤差の上限
  ///
  /// @note 1280 x 720 と 854 x 480 のように、画素数の丸めで生じる差を許容する。
  ///
  static constexpr double aspectTolerance{ 0.01 };

  ///
  /// 二つの画像サイズのアスペクト比が同じか調べる
  ///
  /// @param a 一方の画像サイズ
  /// @param b もう一方の画像サイズ
  /// @return アスペクト比の相対誤差が aspectTolerance 以下なら true
  ///
  static bool isSameAspect(const cv::Size& a, const cv::Size& b);

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
  /// 入力画像のサイズが較正時の画像サイズと合っているか調べる
  ///
  /// @param size 入力画像のサイズ
  /// @return 較正時の画像サイズとの関係
  ///
  CalibrationSizeMatch matchSize(const cv::Size& size) const;

  ///
  /// 入力画像のサイズに換算したカメラ行列を得る
  ///
  /// @param size 入力画像のサイズ
  /// @return 換算したカメラ行列 (較正時の画像サイズが不明なら較正値そのもの)
  ///
  /// @details
  /// 焦点距離は幅と高さの比で拡大縮小し、主点は画素の中心を基準にして
  /// cx' = (cx + 0.5) × s - 0.5 で換算する。
  /// アスペクト比が違う場合も同じ式で換算するが、正しい補正にはならない。
  ///
  cv::Mat getCameraMatrix(const cv::Size& size) const;

  ///
  /// GLSL に渡すカメラ行列の主要成分を得る
  ///
  /// @param size 入力画像のサイズ
  /// @return 入力画像のサイズに換算した (fx, fy, cx, cy) の順に格納した配列
  ///
  std::array<float, 4> getCameraParameters(const cv::Size& size) const;

  ///
  /// GLSL に渡す歪み係数を得る
  ///
  /// @return (k1, k2, p1, p2, k3) の順に格納した配列
  ///
  std::array<float, 5> getDistortionParameters() const;

  ///
  /// 較正時の画像サイズのカメラ行列を取り出す
  ///
  /// @return カメラ行列への参照
  ///
  /// @note 入力画像に使うときは、入力画像のサイズに換算する getCameraMatrix(size) を使う。
  ///
  const cv::Mat& getCameraMatrix() const
  {
    return cameraMatrix;
  }

  ///
  /// 歪み係数を取り出す
  ///
  /// @return 歪み係数への参照
  ///
  const cv::Mat& getDistortion() const
  {
    return distortion;
  }

  ///
  /// 較正ファイルに記録された画像サイズを取り出す
  ///
  /// @return 較正時の画像サイズ
  ///
  const cv::Size& getImageSize() const
  {
    return imageSize;
  }
};
