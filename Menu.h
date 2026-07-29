#pragma once

///
/// メニューの描画クラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

// 構成データ
#include "Config.h"

// キャプチャデバイス
#include "Capture.h"

// レンズ歪み補正
#include "Undistortion.h"

///
/// メニューの描画
///
class Menu
{
  /// 読み込み・保存の対象となる構成データへの参照
  Config& config;

  /// 設定データのコピー
  Settings settings;

#if !defined(_WIN32)
  /// バックエンドのリスト
  static const std::map<cv::VideoCaptureAPIs, const char*> backendList;

  /// コーデックのリスト
  static const std::vector<const char*> codecList;

  /// キャプチャデバイスのリスト
  static std::map <cv::VideoCaptureAPIs, std::vector<std::string>> deviceList;

  /// 読み込む動画ファイル名の履歴
  std::vector<std::string> fileHistory;

  ///
  /// キャプチャデバイスのリストを取り出す
  ///
  /// @param api 使用しているバックエンドの API 名
  /// @return キャプチャデバイスのリスト
  ///
  const auto& getDeviceList(cv::VideoCaptureAPIs api) const
  {
    return deviceList.at(api);
  }

  ///
  /// キャプチャデバイスの数を調べる
  ///
  /// @param api 使用しているバックエンドの API 名
  /// @return キャプチャデバイスの数
  ///
  auto getDeviceCount(cv::VideoCaptureAPIs api) const
  {
    return static_cast<int>(deviceList.at(api).size());
  }

  ///
  /// キャプチャデバイスの名前を調べる
  ///
  /// @param api 使用しているバックエンドの API 名
  /// @param number キャプチャデバイスの番号
  /// @return キャプチャデバイスの名前
  ///
  const auto& getDeviceName(cv::VideoCaptureAPIs api, int number) const
  {
    static const std::string empty{};
    const auto& list{ deviceList.at(api) };
    return list.empty() ? empty : list[number];
  }
#endif

  /// 使用中の構成のキャプチャデバイス固有のパラメータのコピー
  Intrinsics intrinsics;

  /// キャプチャデバイス
  Capture& capture;

  /// 較正パラメータと補正処理
  Undistortion& undistortion;

  /// 選択中の補正方法
  UndistortionMode undistortionMode{ UndistortionMode::None };

  /// 選択しているキャプチャデバイスの番号
  int deviceNumber{ 0 };

#if defined(_WIN32)
  /// 選択しているビデオフォーマットの番号
  int formatNumber{ 0 };

  /// 使用可能なビデオフォーマットのリスト
  std::vector<CaptureFormat> availableFormats;

  /// 重複のない解像度のリスト
  std::vector<std::string> uniqueResolutions;

  /// 重複のないフレームレートのリスト
  std::vector<std::string> uniqueFpsList;

  /// 重複のないコーデックのリスト
  std::vector<std::string> uniqueCodecs;

  /// 現在選択されている解像度
  std::string currentRes;

  /// 現在選択されているフレームレート
  std::string currentFps;

  /// 現在選択されているコーデック
  std::string currentCodec;

  /// 最後に処理したデバイスの番号
  int lastDeviceNumber{ -1 };

  ///
  /// 構造化フォーマットから解像度、フレームレート、コーデックの選択肢を更新する
  ///
  void updateFormatDropdowns();
#else
  /// 選択しているコーデックの番号
  int codecNumber{ 0 };

  /// デバイスプリファレンス
  cv::VideoCaptureAPIs backend{ cv::CAP_ANY };
#endif

  /// 使用中の構成の番号
  int preferenceNumber{ 0 };

  /// キャプチャデバイスの姿勢
  GgMatrix pose{ ggIdentity() };

  /// メニューバーの高さ
  GLsizei menubarHeight{ 0 };

  /// 入力パネルの表示
  bool showInputPanel{ true };

  /// 終了するなら true
  bool quit{ false };

  /// エラーが無ければ nullptr
  mutable const char* errorMessage{ nullptr };

  ///
  /// キャプチャデバイスを開く
  ///
  /// @return 選択中のデバイスとフォーマットを適用できたら true
  bool openDevice();

  ///
  /// 画像ファイルを開く
  ///
  void openImage();

  ///
  /// 動画ファイルを開く
  ///
  void openMovie();

  ///
  /// 構成ファイルを読み込む
  ///
  void loadConfig();

  ///
  /// 構成ファイルを保存する
  ///
  void saveConfig() const;

  ///
  /// calib が作成した較正パラメータファイルを読み込む
  ///
  /// @details NFD のファイルダイアログで JSON を選択し、読み込みに失敗した場合は
  /// 補正を無効にしてエラーダイアログを表示する。
  ///
  void loadCalibration();

  ///
  /// 指定した番号の構成を調べる
  ///
  /// @param i 構成の番号
  ///
  const auto& getPreference(int i) const
  {
    return config.getPreferences()[i];
  }

  ///
  /// 現在の構成を調べる
  ///
  const auto& getPreference() const
  {
    return getPreference(preferenceNumber);
  }

public:

  ///
  /// コンストラクタ
  ///
  /// @param config 構成データ
  /// @param capture 入力フレームを取得するキャプチャデバイス
  /// @param undistortion 較正パラメータと歪み補正処理
  ///
  Menu(Config& config, Capture& capture, Undistortion& undistortion);

  ///
  /// コピーコンストラクタは使用しない
  ///
  /// @param menu コピー元
  ///
  Menu(const Menu& menu) = delete;

  ///
  /// デストラクタ
  ///
  virtual ~Menu();

  ///
  /// 代入演算子は使用しない
  ///
  /// @param menu 代入元のメニュー
  /// @return 代入後のこのメニューの参照
  ///
  Menu& operator=(const Menu& menu) = delete;

  ///
  /// 処理を継続するかどうか調べる
  ///
  /// @return 処理を継続するなら true
  /// 
  explicit operator bool() const
  {
    return !quit;
  }

  ///
  /// 正規化デバイス座標系における焦点距離を求める
  ///
  /// @return 図形の姿勢
  ///
  const auto& getPose() const
  {
    return pose;
  }

  ///
  /// メニューバーの高さを得る
  ///
  /// @return メニューバーの高さ
  /// 
  auto getMenubarHeight() const
  {
    return menubarHeight;
  }

  ///
  /// 解像度初期値を設定する
  ///
  /// @param size キャプチャされるフレームの解像度
  ///
  /// @note
  /// 解像度と現在の焦点距離 focal をもとに、
  /// 撮像系の縦横の画角の初期値も設定する。
  /// 投影面の中心位置も設定する。
  /// 
  void setSize(const std::array<int, 2>& size);

  ///
  /// シェーダを設定する
  ///
  /// @param aspect 表示領域の縦横比
  /// @return 描画すべきメッシュの横と縦の格子点数
  ///
  /// @note
  /// 格子点数は画角 aspect と展開用メッシュのサンプル点数 samples から求める。
  ///
  std::array<GLsizei, 2> setup(GLfloat aspect) const;

  ///
  /// UI で選択されている歪み補正方法を得る
  ///
  /// @return 現在の歪み補正方法
  ///
  UndistortionMode getUndistortionMode() const
  {
    return undistortionMode;
  }

  ///
  /// メニューを描画する
  ///
  void draw();

};
