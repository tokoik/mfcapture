///
/// 歪補正処理クラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date July 27, 2027
///
#include "Undistortion.h"
#if !defined(__ANDROID__)
#include "gg.h"
#else
namespace
{
  inline const std::string& Utf8ToTChar(const std::string& string) { return string; }
}
#endif

// JSON
#include "picojson.h"

// 標準ライブラリ
#include <cmath>
#include <fstream>

namespace
{
  ///
  /// JSON 配列を指定サイズの OpenCV 行列へ変換する
  ///
  /// @param object 較正ファイルのルートオブジェクト
  /// @param key 読み込む JSON ノードの名前
  /// @param matrix 変換した行列の格納先
  /// @param rows 作成する行列の行数
  /// @param columns 作成する行列の列数
  /// @return 数値配列を指定サイズの行列へ変換できたら true
  ///
  bool readMatrix(const picojson::object& object, const std::string& key,
    cv::Mat& matrix, int rows, int columns)
  {
    // 必須ノードが存在し、その値が JSON 配列であることを確認する。
    const auto item{ object.find(key) };
    if (item == object.end() || !item->second.is<picojson::array>()) return false;

    // 要素数が期待する行列サイズと異なるファイルは受け付けない。
    const auto& values{ item->second.get<picojson::array>() };
    if (values.size() != static_cast<size_t>(rows * columns)) return false;

    // 丸括弧で行数・列数・型を渡し、目的の二次元行列を確保する。
    // 波括弧では initializer_list 版が選ばれて 3 x 1 行列になるので注意する。
    cv::Mat loaded(rows, columns, CV_64F);

    // JSON は行優先の一次元配列なので、商を行、剰余を列として格納する。
    for (size_t i = 0; i < values.size(); ++i)
    {
      if (!values[i].is<double>()) return false;
      loaded.at<double>(static_cast<int>(i) / columns,
        static_cast<int>(i) % columns) = values[i].get<double>();
    }

    // 全要素を検証できた場合だけ呼び出し元の行列を更新する。
    matrix = std::move(loaded);
    return true;
  }
}

//
// calib が作成した較正ファイルを読み込む
//
bool Undistortion::load(const std::string& filename)
{
  // 指定された JSON ファイルを開く。
  std::ifstream stream{ Utf8ToTChar(filename) };
  if (!stream) return false;

  // picojson で構文解析し、ルートが JSON オブジェクトであることを確認する。
  picojson::value value;
  stream >> value;
  if (!stream || !value.is<picojson::object>()) return false;

  // 読み込み途中の失敗で現在の有効な較正値を壊さないよう、一時行列へ格納する。
  cv::Mat loadedCamera;
  cv::Mat loadedDistortion;
  const auto& object{ value.get<picojson::object>() };

  // calib の保存形式に合わせ、3 x 3 のカメラ行列と 5 x 1 の歪み係数を要求する。
  if (!readMatrix(object, "camera matrix", loadedCamera, 3, 3)
    || !readMatrix(object, "distortion", loadedDistortion, 5, 1))
  {
    return false;
  }

  // 両方の行列が正常な場合だけ、新しい較正値へまとめて置き換える。
  cameraMatrix = std::move(loadedCamera);
  distortion = std::move(loadedDistortion);

  // 較正ファイルに画像サイズが記録されていれば読み込む。
  imageSize = cv::Size{ 0, 0 };
  auto sizeIt{ object.find("size") };
  if (sizeIt == object.end()) sizeIt = object.find("image size");
  if (sizeIt != object.end() && sizeIt->second.is<picojson::array>())
  {
    const auto& arr{ sizeIt->second.get<picojson::array>() };
    if (arr.size() == 2 && arr[0].is<double>() && arr[1].is<double>())
    {
      imageSize = cv::Size{ static_cast<int>(arr[0].get<double>()), static_cast<int>(arr[1].get<double>()) };
    }
  }

  // 較正値が変わったため、古い値から作った OpenCV の補正マップを無効化する。
  mapX.release();
  mapY.release();
  mapSize = {};
  return true;
}

//
// 補正に必要なパラメータが読み込まれているか調べる
//
bool Undistortion::ready() const
{
  return cameraMatrix.total() == 9 && distortion.total() == 5;
}

//
// OpenCV を使って入力画像のレンズ歪みを補正する
//
void Undistortion::apply(const cv::Mat& source, cv::Mat& destination)
{
  // パラメータまたは画像が無い場合は、補正せず入力をそのまま渡す。
  if (!ready() || source.empty())
  {
    source.copyTo(destination);
    return;
  }

  // 補正座標表は画像サイズに依存するため、サイズが変わったときだけ再計算する。
  // カメラ行列は入力画像のサイズに換算してから使う。
  if (mapSize != source.size())
  {
    const cv::Mat camera{ getCameraMatrix(source.size()) };
    cv::initUndistortRectifyMap(camera, distortion, cv::Mat{},
      camera, source.size(), CV_32FC1, mapX, mapY);
    mapSize = source.size();
  }

  // 各出力画素が参照すべき入力座標をマップから求め、線形補間して補正画像を作る。
  cv::remap(source, destination, mapX, mapY, cv::INTER_LINEAR,
    cv::BORDER_CONSTANT);
}

//
// 二つの画像サイズのアスペクト比が同じか調べる
//
bool Undistortion::isSameAspect(const cv::Size& a, const cv::Size& b)
{
  if (a.width <= 0 || a.height <= 0 || b.width <= 0 || b.height <= 0) return false;
  const double aspectA{ static_cast<double>(a.width) / a.height };
  const double aspectB{ static_cast<double>(b.width) / b.height };
  return std::abs(aspectA / aspectB - 1.0) <= aspectTolerance;
}

//
// 入力画像のサイズが較正時の画像サイズと合っているか調べる
//
CalibrationSizeMatch Undistortion::matchSize(const cv::Size& size) const
{
  // どちらかのサイズが分からなければ判定できない。
  if (imageSize.width <= 0 || imageSize.height <= 0 || size.width <= 0 || size.height <= 0)
  {
    return CalibrationSizeMatch::Unknown;
  }

  // 同じ解像度ならそのまま使える。
  if (size == imageSize) return CalibrationSizeMatch::Exact;

  // アスペクト比が同じなら、多くのカメラで同じ視野を拡大縮小したものなので換算して使える。
  if (isSameAspect(size, imageSize)) return CalibrationSizeMatch::Scaled;

  // アスペクト比が違うモードは視野の切り出し方が違うので、換算しても正しくない。
  return CalibrationSizeMatch::AspectMismatch;
}

//
// 入力画像のサイズに換算したカメラ行列を得る
//
cv::Mat Undistortion::getCameraMatrix(const cv::Size& size) const
{
  // 換算できないときや同じ解像度のときは較正値をそのまま使う。
  const auto match{ matchSize(size) };
  if (match == CalibrationSizeMatch::Unknown || match == CalibrationSizeMatch::Exact)
  {
    return cameraMatrix;
  }

  // 幅と高さの比で焦点距離を拡大縮小し、主点は画素の中心を基準にして換算する。
  const double sx{ static_cast<double>(size.width) / imageSize.width };
  const double sy{ static_cast<double>(size.height) / imageSize.height };
  cv::Mat scaled{ cameraMatrix.clone() };
  scaled.at<double>(0, 0) *= sx;
  scaled.at<double>(0, 1) *= sx;
  scaled.at<double>(0, 2) = (scaled.at<double>(0, 2) + 0.5) * sx - 0.5;
  scaled.at<double>(1, 1) *= sy;
  scaled.at<double>(1, 2) = (scaled.at<double>(1, 2) + 0.5) * sy - 0.5;
  return scaled;
}

//
// GLSL に渡すカメラ行列の主要成分を取り出す
//
std::array<float, 4> Undistortion::getCameraParameters(const cv::Size& size) const
{
  // 未読込時はゼロ配列を返し、通常シェーダの uniform 設定を安全に行えるようにする。
  if (!ready()) return {};

  // 入力画像のサイズに換算したカメラ行列 K から焦点距離と主点だけを float へ変換する。
  const cv::Mat camera{ getCameraMatrix(size) };
  return {
    static_cast<float>(camera.at<double>(0, 0)),
    static_cast<float>(camera.at<double>(1, 1)),
    static_cast<float>(camera.at<double>(0, 2)),
    static_cast<float>(camera.at<double>(1, 2))
  };
}

//
// GLSL に渡す歪み係数を取り出す
//
std::array<float, 5> Undistortion::getDistortionParameters() const
{
  // 未読込時はゼロ配列を返す。
  if (!ready()) return {};

  // OpenCV の標準的な 5 係数モデルと同じ順序を維持する。
  return {
    static_cast<float>(distortion.at<double>(0)),
    static_cast<float>(distortion.at<double>(1)),
    static_cast<float>(distortion.at<double>(2)),
    static_cast<float>(distortion.at<double>(3)),
    static_cast<float>(distortion.at<double>(4))
  };
}
