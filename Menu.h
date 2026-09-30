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

// 内部パラメータ
#include "Intrinsics.h"

// キャプチャデバイス
#include "Capture.h"

// レンズ歪み補正
#include "Undistortion.h"

// ArUco Marker 認識
#include "Aruco.h"

///
/// メニューの描画
///
class Menu
{
  /// 読み込み・保存の対象となる構成データへの参照
  Config& config;

  /// 設定データのコピー
  Settings settings;

#if !defined(_WIN32) && !defined(__ANDROID__) && !defined(__APPLE__)
  /// バックエンドのリスト
  static const std::map<cv::VideoCaptureAPIs, const char*> backendList;

  /// コーデックのリスト
  static const std::vector<const char*> codecList;

  /// キャプチャデバイスのリスト
  static std::map <cv::VideoCaptureAPIs, std::vector<std::string>> deviceList;

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

  /// ArUco Marker 認識処理
  Aruco& aruco;

  /// 選択中の補正方法
  UndistortionMode undistortionMode{ UndistortionMode::None };

  /// 選択しているキャプチャデバイスの番号
  int deviceNumber{ 0 };

#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
  /// 選択しているビデオフォーマットの番号
  int formatNumber{ -1 };

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
  /// @details formatNumber が選択肢に存在しなければ、1280 x 720 に近い既定のフォーマットを選ぶ。
  ///
  void updateFormatDropdowns();

  ///
  /// 解像度、フレームレート、コーデックのいずれかを選択し、実在する組み合わせに同期する
  ///
  /// @param field 選択した項目を指す CaptureFormat のメンバポインタ
  /// @param value 選択した項目の値
  /// @details 他の項目を維持した組み合わせが存在しなければ、解像度を維持できる組み合わせ、
  /// それもなければ選択した項目が一致する最初の組み合わせに他の項目を合わせる。
  ///
  void selectFormatItem(std::string CaptureFormat::* field, const std::string& value);
#else
  /// 選択しているコーデックの番号
  int codecNumber{ 0 };

  /// デバイスプリファレンス
  cv::VideoCaptureAPIs backend{ cv::CAP_ANY };
#endif

#if !defined(__ANDROID__)
  /// 使用中の構成の番号
  int preferenceNumber{ 0 };

  /// キャプチャデバイスの姿勢
  GgMatrix pose{ ggIdentity() };

  /// メニューバーの高さ
  GLsizei menubarHeight{ 0 };
#endif

  /// 入力パネルの表示
  bool showInputPanel{ true };

  /// ArUco パネルの表示
  bool showArucoPanel{ true };

  /// 終了するなら true
  bool quit{ false };

  /// エラーが無ければ nullptr
  mutable const char* errorMessage{ nullptr };

  /// 警告が無ければ nullptr
  const char* warningMessage{ nullptr };

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
  /// 較正ファイルを読み込む
  ///
  void loadCalibration();

  ///
  /// 選択中の投影方式とその内部パラメータを同期する
  ///
  /// @param index 新しく選択する投影方式の番号
  /// @details 投影方式固有の画角と中心位置を反映し、入力が開いている場合は
  /// 実際のキャプチャ解像度だけを維持する。入力オープン時に計算した初期画角は
  /// この操作によって投影方式の設定値へ戻る。
  ///
  void selectPreference(int index);

#if !defined(__ANDROID__)
  ///
  /// 指定した番号の構成を調べる
  ///
  /// @param i 構成の番号
  /// @return 指定した投影方式への読み取り専用参照
  ///
  const auto& getPreference(int i) const
  {
    return config.getPreferences()[i];
  }

  ///
  /// 現在選択中の投影方式を調べる
  ///
  /// @return 現在選択中の投影方式への読み取り専用参照
  ///
  const auto& getPreference() const
  {
    return getPreference(preferenceNumber);
  }

  ///
  /// メインメニューバーを描画し、ファイル操作とパネル表示の要求を処理する
  ///
  void drawMainMenuBar();

  ///
  /// 投影方式と入力デバイスを設定する入力パネルを描画する
  ///
  void drawInputPanel();

  ///
  /// ArUco Marker の設定と認識を行うパネルを描画する
  ///
  void drawArucoPanel();

  ///
  /// 保留中のエラーメッセージをダイアログとして描画する
  ///
  void drawErrorDialog();
#endif

public:

  /// レイテンシを優先するなら true
  bool prioritizeLatency{ true };

  /// ArUco Marker を検出するなら true
  bool detectMarker{ false };

  ///
  /// コンストラクタ
  ///
  /// @param config 構成データ
  /// @param capture 入力フレームを取得するキャプチャデバイス
  /// @param undistortion 較正パラメータと歪み補正処理
  /// @param aruco ArUco Marker 認識処理
  ///
  Menu(Config& config, Capture& capture, Undistortion& undistortion, Aruco& aruco);

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

#if !defined(__ANDROID__)
  ///
  /// キャプチャデバイスの姿勢を得る
  ///
  /// @return 図形の姿勢
  ///
  const auto& getPose() const
  {
    return pose;
  }
#endif

  ///
  /// 選択するキャプチャデバイスの番号を設定する
  ///
  /// @param number デバイス番号
  ///
  void setDeviceNumber(int number)
  {
    deviceNumber = number;
  }

  ///
  /// 選択されているキャプチャデバイスの番号を得る
  ///
  /// @return デバイス番号
  ///
  int getDeviceNumber() const
  {
    return deviceNumber;
  }

  ///
  /// 選択中の入力設定を適用してキャプチャを開始する
  ///
  /// @return デバイスを開いてキャプチャを開始できたら true
  ///
  bool startCapture();

#if !defined(__ANDROID__)
  ///
  /// メニューバーの高さを得る
  ///
  /// @return メニューバーの高さ
  /// 
  auto getMenubarHeight() const
  {
    return menubarHeight;
  }
#else
  ///
  /// メニューバーの高さを得る
  ///
  /// @return メニューバーの高さ (Android では 0)
  ///
  auto getMenubarHeight() const
  {
    return 0;
  }
#endif

  ///
  /// ArUco Marker の一辺の長さを得る
  ///
  /// @return ArUco Marker の一辺の長さ (単位 cm)
  ///
  auto getMarkerLength() const
  {
    return settings.markerLength;
  }

  ///
  /// 設定データを得る
  ///
  /// @return 設定データへの参照
  ///
  const auto& getSettings() const
  {
    return settings;
  }

  ///
  /// 設定データを得る
  ///
  /// @return 設定データへの参照
  ///
  auto& getSettings()
  {
    return settings;
  }

  ///
  /// 入力画像に合わせて内部パラメータを初期化する
  ///
  /// @param size 開かれた入力フレームの解像度
  ///
  /// @details 実解像度を反映し、現在の焦点距離から画像全体が見やすい初期画角を
  /// 計算する。中心位置は投影方式の設定値を維持する。
  /// 
  void initializeInputIntrinsics(const std::array<int, 2>& size);

  ///
  /// 入力画像の解像度が較正時の解像度と合っているか調べて警告を設定する
  ///
  /// @details
  /// 較正ファイルを読み込んだときと、入力画像の解像度が変わったときに呼ぶ。
  /// 較正時とアスペクト比が同じで解像度が違えばカメラ行列を換算して補正することを、
  /// アスペクト比が違えば正しく補正できないことを警告する。
  ///
  void checkCalibrationSize();

  ///
  /// 較正時の解像度との関係に応じた警告の文言を得る
  ///
  /// @param match 入力画像のサイズと較正時の画像サイズの関係
  /// @return 警告の文字列 (UTF-8), 警告が無ければ nullptr
  ///
  /// @note デスクトップ版の警告ウィンドウと Android 版のダイアログで同じ文言を使う。
  ///
  static const char* getCalibrationWarningText(CalibrationSizeMatch match);

  ///
  /// 較正時の解像度に関する警告を得る
  ///
  /// @return 警告の文字列, 警告が無ければ nullptr
  ///
  const char* getWarningMessage() const
  {
    return warningMessage;
  }

#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
  ///
  /// 利用可能なカメラ解像度の総数を取得する
  ///
  /// @return 解像度の数
  ///
  int getResolutionCount() const
  {
    return static_cast<int>(uniqueResolutions.size());
  }

  ///
  /// インデックス指定で利用可能なカメラ解像度を取得する
  ///
  /// @param index 解像度インデックス (0 <= index < getResolutionCount())
  /// @return 解像度文字列 (例: "1280 x 720"、範囲外なら空文字列)
  ///
  std::string getResolutionByIndex(int index) const
  {
    return (index >= 0 && index < static_cast<int>(uniqueResolutions.size())) ? uniqueResolutions[index] : std::string{};
  }

  ///
  /// 現在選択されているカメラ解像度を取得する
  ///
  /// @return 現在の解像度文字列 (例: "1280 x 720")
  ///
  const std::string& getCurrentResolution() const
  {
    return currentRes;
  }

  ///
  /// カメラ解像度を選択する
  ///
  /// @param resolution 選択する解像度文字列 (例: "1280 x 720")
  /// @return 変更に成功したら true
  ///
  bool selectResolution(const std::string& resolution);

  ///
  /// 指定された解像度に最も近いカメラ解像度を選択する
  ///
  /// @param targetWidth 目標とする解像度の幅 (px)
  /// @param targetHeight 目標とする解像度の高さ (px)
  /// @return 変更に成功したら true
  ///
  bool selectBestResolution(int targetWidth, int targetHeight);
#endif

  ///
  /// UI で選択されている歪み補正方法を得る
  ///
  /// @return 現在の歪み補正方法
  ///
  auto getUndistortionMode() const
  {
    return undistortionMode;
  }

#if !defined(__ANDROID__)
  ///
  /// 歪み補正シェーダを設定する
  ///
  /// @param aspect 表示領域の縦横比
  /// @return 描画すべきメッシュの横と縦の格子点数
  ///
  std::array<GLsizei, 2> setupUndistortion(GLfloat aspect) const;

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
  /// メニューを描画する
  ///
  void draw();
#endif
};
