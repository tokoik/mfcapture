///
/// ArUco Marker 認識クラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date March 6, 2024
///
#include "Aruco.h"

// OpenCV キャリブレーション (solvePnP, drawFrameAxes)
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

//
// コンストラクタ
//
Aruco::Aruco(const std::string& dictionaryName)
{
  // 指定された辞書名で検出器を初期化する
  setDictionary(dictionaryName);
}

//
// ArUco Marker の辞書と検出器を設定する
//
void Aruco::setDictionary(const std::string& dictionaryName)
{
  // ArUco Marker の辞書を検索する
  auto dictionaryItem{ dictionaryList.find(dictionaryName) };

  // ArUco Marker の辞書が見つからなかったら辞書リストの最初の辞書を使う
  if (dictionaryItem == dictionaryList.end()) dictionaryItem = dictionaryList.begin();

  // ArUco Marker の辞書を設定する
  dictionary = cv::aruco::getPredefinedDictionary(dictionaryItem->second);

  // ArUco Marker の検出器を作成する
  cv::aruco::DetectorParameters detectorParams = cv::aruco::DetectorParameters();
  detector = new cv::aruco::ArucoDetector(dictionary, detectorParams);
}

//
// ArUco Marker を検出して画像に描画する
//
void Aruco::detectMarkers(cv::Mat& image, float markerLength,
  const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs)
{
  // 入力画像が空なら何もしない
  if (image.empty()) return;

  // 4 チャンネル画像の場合は OpenCV の検出・描画処理のために一時的に 3 チャンネル (BGR) 画像を作成する
  cv::Mat tempImage;
  const auto isFourChannels{ image.channels() == 4 };
  if (isFourChannels)
  {
    cv::cvtColor(image, tempImage, cv::COLOR_BGRA2BGR);
  }
  else
  {
    tempImage = image;
  }

  // ArUco Marker のコーナーを検出する
  detector->detectMarkers(tempImage, corners, ids, rejected);

  // コーナーが見つからなければ戻る
  if (corners.empty()) return;

  // カメラ内部パラメータ行列が有効 (3x3) であればマーカーの姿勢推定と座標軸描画を行う
  const bool hasCameraMatrix{ cameraMatrix.rows == 3 && cameraMatrix.cols == 3 };
  if (hasCameraMatrix)
  {
    // 各マーカーの 4 隅に対応する 3 次元空間上の局所座標系 (Z=0 平面、中心原点)
    const float markerCenter{ markerLength * 0.5f };
    std::vector<cv::Point3f> markerObjPoints;
    markerObjPoints.push_back(cv::Point3f(-markerCenter, markerCenter, 0.0f));
    markerObjPoints.push_back(cv::Point3f(markerCenter, markerCenter, 0.0f));
    markerObjPoints.push_back(cv::Point3f(markerCenter, -markerCenter, 0.0f));
    markerObjPoints.push_back(cv::Point3f(-markerCenter, -markerCenter, 0.0f));

    // 個々のマーカーについて
    for (size_t i = 0; i < corners.size(); ++i)
    {
      // マーカーのコーナー検出位置から 3 次元姿勢（回転ベクトル・平行移動ベクトル）を推定する
      cv::Vec3d rvec, tvec;
      cv::solvePnP(markerObjPoints, corners[i], cameraMatrix, distCoeffs,
        rvec, tvec, false, cv::SOLVEPNP_IPPE_SQUARE);

      // マーカーの位置に座標軸を描く
      cv::drawFrameAxes(tempImage, cameraMatrix, distCoeffs, rvec, tvec, markerLength);
    }
  }
  else
  {
    // カメラパラメータが読み込まれていなければ、ArUco Marker の場所に矩形枠と ID 番号を描き込む
    cv::aruco::drawDetectedMarkers(tempImage, corners, ids);
  }

  // 4 チャンネル画像の場合は検出・描画結果を元の画像へ書き戻す
  if (isFourChannels)
  {
    cv::cvtColor(tempImage, image, cv::COLOR_BGR2BGRA);
  }
}

//
// ArUco Marker 辞書のリスト
//
const std::map<const std::string, const cv::aruco::PredefinedDictionaryType> Aruco::dictionaryList
{
  { "DICT_4X4_50", cv::aruco::DICT_4X4_50 },
  { "DICT_4X4_100", cv::aruco::DICT_4X4_100 },
  { "DICT_4X4_250", cv::aruco::DICT_4X4_250 },
  { "DICT_4X4_1000", cv::aruco::DICT_4X4_1000 },
  { "DICT_5X5_50", cv::aruco::DICT_5X5_50 },
  { "DICT_5X5_100", cv::aruco::DICT_5X5_100 },
  { "DICT_5X5_250", cv::aruco::DICT_5X5_250 },
  { "DICT_5X5_1000", cv::aruco::DICT_5X5_1000 },
  { "DICT_6X6_50", cv::aruco::DICT_6X6_50 },
  { "DICT_6X6_100", cv::aruco::DICT_6X6_100 },
  { "DICT_6X6_250", cv::aruco::DICT_6X6_250 },
  { "DICT_6X6_1000", cv::aruco::DICT_6X6_1000 },
  { "DICT_7X7_50", cv::aruco::DICT_7X7_50 },
  { "DICT_7X7_100", cv::aruco::DICT_7X7_100 },
  { "DICT_7X7_250", cv::aruco::DICT_7X7_250 },
  { "DICT_7X7_1000", cv::aruco::DICT_7X7_1000 },
  { "DICT_ARUCO_ORIGINAL", cv::aruco::DICT_ARUCO_ORIGINAL },
  { "DICT_APRILTAG_16h5", cv::aruco::DICT_APRILTAG_16h5 },
  { "DICT_APRILTAG_25h9", cv::aruco::DICT_APRILTAG_25h9 },
  { "DICT_APRILTAG_36h10", cv::aruco::DICT_APRILTAG_36h10 },
  { "DICT_APRILTAG_36h11", cv::aruco::DICT_APRILTAG_36h11 }
};
