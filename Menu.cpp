///
/// メニューの描画クラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///
#include "Menu.h"

// ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// ファイルダイアログ
#include "nfd.h"

namespace
{
  // JSON ファイル名のフィルタ
  constexpr nfdfilteritem_t jsonFilter[]{ { "JSON", "json" } };

  // 画像ファイル名のフィルタ
  constexpr nfdfilteritem_t imageFilter[]{ "Images", "png,jpg,jpeg,jfif,bmp,dib" };

  // 動画ファイル名のフィルタ
  constexpr nfdfilteritem_t movieFilter[]{ "Movies", "mp4,m4v,mpg,mov,avi,ogg,mkv" };
}

// 初期表示の画像ファイル名
std::string Config::initialImage{ "initial.jpg" };

// 標準ライブラリ
#include <sstream>

#if !defined(_WIN32)
// バックエンドのリスト
const std::map<cv::VideoCaptureAPIs, const char*> Menu::backendList
{
#  if defined(USE_LIBCAMERA)
  { CAP_LIBCAMERA, "libcamera" },
#  endif
  { cv::CAP_ANY, "(any)" },
#  if defined(__APPLE__)
  { cv::CAP_AVFOUNDATION, "AV Foundation" },
#  elif defined(__linux__)
  { cv::CAP_V4L2, "V4L2" },
#  endif
  { cv::CAP_FFMPEG, u8"動画ファイル履歴" }
};

// コーデックのリスト
const std::vector<const char*> Menu::codecList
{
  "(any)",
  "MJPG",
  "H264",
  "BGR3",
  "YUY2",
  "I420",
  "NV12"
};

// キャプチャデバイスのリスト
std::map <cv::VideoCaptureAPIs, std::vector<std::string>> Menu::deviceList;

//
// デフォルトのビデオデバイスの一覧を作る
//
void getAnyList(std::vector<std::string>& list)
{
  list.emplace_back("(any)");
  list.emplace_back("Device 1");
  list.emplace_back("Device 2");
  list.emplace_back("Device 3");
  list.emplace_back("Device 4");
  list.emplace_back("Device 5");
  list.emplace_back("Device 6");
  list.emplace_back("Device 7");
}

#  if defined(__APPLE__)
//
// macOS のビデオデバイスの一覧を作る
//
void getAvFoundationList(std::vector<std::string>& list)
{
  getAnyList(list);
}
#  elif defined(__linux__)
#    include <filesystem>
#    include <fstream>
#    include <fcntl.h>
#    include <unistd.h>
#    include <sys/ioctl.h>
#    include <linux/videodev2.h>

//
// Linux (V4L2) のビデオデバイスの一覧を作る
//
void getV4L2List(std::vector<std::string>& list)
{
  namespace fs = std::filesystem;
  std::error_code ec;

  // /sys/class/video4linux ディレクトリを走査する
  const fs::path v4l2Path{ "/sys/class/video4linux" };
  if (fs::exists(v4l2Path, ec))
  {
    std::map<int, std::string> cameraDevices;
    std::map<int, std::string> otherDevices;

    for (const auto& entry : fs::directory_iterator(v4l2Path, ec))
    {
      const auto filename{ entry.path().filename().string() };
      // "video" で始まるノード (video0, video1, ...)
      if (filename.rfind("video", 0) == 0)
      {
        try
        {
          const int index{ std::stoi(filename.substr(5)) };
          const std::string devPath{ "/dev/" + filename };

          // デバイスファイルを開いてケーパビリティを調べる
          const int fd{ ::open(devPath.c_str(), O_RDONLY | O_NONBLOCK) };
          if (fd >= 0)
          {
            v4l2_capability cap{};
            if (::ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0)
            {
              const uint32_t caps{ cap.device_caps ? cap.device_caps : cap.capabilities };
              const std::string card{ reinterpret_cast<const char*>(cap.card) };
              const std::string driver{ reinterpret_cast<const char*>(cap.driver) };

              // キャプチャ機能 (VIDEO_CAPTURE) を持ち、出力 (OUTPUT) や M2M ではないこと
              const bool isCapture{ (caps & (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_CAPTURE_MPLANE)) != 0 };
              const bool isM2M{ (caps & (V4L2_CAP_VIDEO_M2M | V4L2_CAP_VIDEO_M2M_MPLANE)) != 0 };
              const bool isOutput{ (caps & (V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_VIDEO_OUTPUT_MPLANE)) != 0 };
              const bool isMeta{ (caps & (V4L2_CAP_META_CAPTURE | V4L2_CAP_META_OUTPUT)) != 0 };

              // Raspberry Pi の bcm2835-codec や bcm2835-isp, pisp 等の SoC 内部処理用ノードは除外
              const bool isSoCInternal{
                driver.find("bcm2835") != std::string::npos ||
                card.find("bcm2835") != std::string::npos ||
                driver.find("pisp") != std::string::npos ||
                card.find("pisp") != std::string::npos
              };

              std::string displayName{ filename + ": " + (card.empty() ? driver : card) };

              if (isCapture && !isM2M && !isOutput && !isMeta && !isSoCInternal)
              {
                cameraDevices[index] = displayName;
              }
              else
              {
                otherDevices[index] = displayName;
              }
            }
            ::close(fd);
          }
        }
        catch (...)
        {
        }
      }
    }

    // カメラデバイスがあればそれを登録
    for (const auto& [idx, devName] : cameraDevices)
    {
      list.emplace_back(devName);
    }

    // カメラデバイスが見つからなかった場合はフォールバックとしてその他を登録
    if (list.empty())
    {
      for (const auto& [idx, devName] : otherDevices)
      {
        list.emplace_back(devName);
      }
    }
  }

  // デバイスが取得できなかった場合はフォールバック
  if (list.empty())
  {
    getAnyList(list);
  }
}
#  endif

// パスワードのエントリからホームディレクトリの場所を得るときに使う
#  include <unistd.h>
#  include <sys/types.h>
#  include <pwd.h>

#endif // !defined(_WIN32)

//
// キャプチャデバイスを開く
//
bool Menu::openDevice()
{
#if defined(_WIN32)
  // 何のデバイスも接続されていなければ戻る
  if (deviceNumber < 0) return false;

  // キャプチャスレッドが動いていたら止める
  capture.stop();

  // 前に開いていたキャプチャデバイスを閉じる
  capture.close();

  // 選択したキャプチャデバイスを開く
  if (capture.openDevice(deviceNumber))
  {
    if (formatNumber < 0) formatNumber = 0;

#if defined(_WIN32)
    updateFormatDropdowns();
#endif

    // フォーマットを指定して開始できるように準備する
    if (capture.select(formatNumber))
    {
      // 実解像度と焦点距離から、この入力を見やすく表示する初期画角を設定する
      initializeInputIntrinsics(capture.getSize());
      return true;
    }
  }

  // 開けなかった
  errorMessage = u8"デバイスが開けません";
  return false;
#else
  // コーデック
  char codec[5]{};
  if (codecNumber > 0) strncpy(codec, codecList[codecNumber], 5);

  // 実際のデバイス番号を決定する
  int actualDeviceNumber{ deviceNumber };
#  if defined(__linux__)
#    if defined(USE_LIBCAMERA)
  if (backend == CAP_LIBCAMERA)
  {
    actualDeviceNumber = deviceNumber;
  }
  else
#    endif
  if (backend == cv::CAP_V4L2)
  {
    const auto& name{ getDeviceName(backend, deviceNumber) };
    if (name.rfind("video", 0) == 0)
    {
      try
      {
        actualDeviceNumber = std::stoi(name.substr(5));
      }
      catch (...)
      {
      }
    }
  }
#  endif

  // ダイアログで指定したキャプチャデバイスが開けなかったら
  if (!capture.openDevice(actualDeviceNumber,
    intrinsics.size, intrinsics.fps, backend, codec))
  {
    // 開けなかった
    errorMessage = u8"デバイスが開けません";
    return false;
  }

  // 使うことになったコーデックの番号を調べる
  for (size_t i = 0; i < codecList.size(); ++i)
  {
    if (strncmp(codec, codecList[i], 4) == 0)
    {
      // コーデックが分かった
      codecNumber = static_cast<int>(i);
      return true;
    }
  }

  // コーデックが分からない
  codecNumber = 0;
  return true;
#endif
}

//
// 画像ファイルを開く
//
void Menu::openImage()
{
  // ファイルダイアログから得るパス
  nfdchar_t* filepath;

  // ファイルダイアログを開く
  if (NFD_OpenDialog(&filepath, imageFilter, 1, NULL) == NFD_OKAY)
  {
    // スレッドが動作中なら停止する
    capture.stop();

    // ダイアログで指定した画像ファイルが開けたら
    if (capture.openImage(filepath))
    {
      // 実解像度と焦点距離から、この画像を見やすく表示する初期画角を設定する
      initializeInputIntrinsics(capture.getSize());
    }
    else
    {
      // 開けなかった
      errorMessage = u8"画像ファイルが開けません";
    }

    // ファイルパスの取り出しに使ったメモリを開放する
    NFD_FreePath(filepath);
  }
}
  
//
// 動画ファイルを開く
//
void Menu::openMovie()
{
  // ファイルダイアログから得るパス
  nfdchar_t* filepath;

  // ファイルダイアログを開く
  if (NFD_OpenDialog(&filepath, movieFilter, 1, NULL) == NFD_OKAY)
  {
    // スレッドが動作中なら停止する
    capture.stop();

#if defined(_WIN32)
    // ダイアログで指定した動画ファイルが開けたら
    if (capture.openMovie(filepath))
    {
      // 実解像度と焦点距離から、この動画を見やすく表示する初期画角を設定する
      initializeInputIntrinsics(capture.getSize());
    }
    else
    {
      errorMessage = u8"動画ファイルが開けません";
    }
#else
    // 入力特性をファイルに切り替えて
    backend = cv::CAP_FFMPEG;

    // ファイルのリストを取り出し
    const auto fileListLength{ static_cast<int>(fileHistory.size()) };

    // ファイルのリストの各ファイルについて
    for (deviceNumber = 0; deviceNumber < fileListLength; ++deviceNumber)
    {
      // 選択したファイルと同じものがあればそれを選択する
      if (fileHistory[deviceNumber] == filepath) break;
    }

    // 選択したファイルがファイルのリストの中になければ
    if (deviceNumber == fileListLength)
    {
      // その先頭にファイルパスを挿入して
      fileHistory.insert(fileHistory.begin(), filepath);

      // そのエントリを選択する
      deviceNumber = 0;
    }

    // ダイアログで指定した動画ファイルが開けたら
    if (capture.openMovie(filepath, backend))
    {
      // 実解像度と焦点距離から、この動画を見やすく表示する初期画角を設定する
      initializeInputIntrinsics(capture.getSize());
    }
    else
    {
      // 開けなかった
      errorMessage = u8"動画ファイルが開けません";
    }
#endif

    // ファイルパスの取り出しに使ったメモリを開放する
    NFD_FreePath(filepath);
  }
}

//
// 構成ファイルを読み込む
//
void Menu::loadConfig()
{
  // ファイルダイアログから得るパス
  nfdchar_t* filepath;

  // ファイルダイアログを開く
  if (NFD_OpenDialog(&filepath, jsonFilter, 1, NULL) == NFD_OKAY)
  {
    // 現在の構成を構成ファイルの内容にする
    if (config.load(filepath))
    {
      // 現在の設定に反映する
      settings = config.getSettings();

      // 選択番号を有効範囲に収め、新しい投影方式の内部パラメータを反映する
      selectPreference(preferenceNumber < static_cast<int>(config.getPreferences().size())
        ? preferenceNumber : 0);
    }
    else
    {
      // 読み込めなかった
      errorMessage = u8"構成ファイルが読み込めません";
    }

    // ファイルパスの取り出しに使ったメモリを開放する
    NFD_FreePath(filepath);
  }
}

//
// 構成ファイルを保存する
//
void Menu::saveConfig() const
{
  // ファイルダイアログから得るパス
  nfdchar_t* filepath;

  // ファイルダイアログを開く
  if (NFD_SaveDialog(&filepath, jsonFilter, 1, NULL, "*.json") == NFD_OKAY)
  {
    // 現在の設定で構成を更新する
    config.setSettings(settings);

    // 現在の構成を保存する
    if (!config.save(filepath))
    {
      // 保存できなかった
      errorMessage = u8"構成ファイルが保存できません";
    }

    // ファイルパスの取り出しに使ったメモリを開放する
    NFD_FreePath(filepath);
  }
}

//
// 較正ファイルを読み込む
//
void Menu::loadCalibration()
{
  // ファイルダイアログから得るパス
  nfdchar_t* filepath;

  // ファイルダイアログを開く
  if (NFD_OpenDialog(&filepath, jsonFilter, 1, NULL) == NFD_OKAY)
  {
    // 現在のキャリブレーションパラメータを較正ファイルの内容にする
    if (!undistortion.load(filepath))
    {
      // 読み込めなかった
      errorMessage = u8"較正ファイルが読み込めません";
      
      // だから画像を補正しない
      undistortionMode = UndistortionMode::None;
    }

    // ファイルパスの取り出しに使ったメモリを開放する
    NFD_FreePath(filepath);
  }
}

//
// コンストラクタ
//
Menu::Menu(Config& config, Capture& capture, Undistortion& undistortion, Aruco& aruco)
  : config{ config }
  , settings{ config.getSettings() }
  , capture{ capture }
  , undistortion{ undistortion }
  , aruco{ aruco }
{
  // ファイルダイアログ (Native File Dialog Extended) を初期化する
  NFD_Init();

  // Dear ImGui の入力デバイス
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // キーボードコントロールを使う
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // ゲームパッドを使う

  // Dear ImGui のスタイル
  //ImGui::StyleColorsDark();                                 // 暗めのスタイル
  //ImGui::StyleColorsClassic();                              // 以前のスタイル

  // 日本語を表示できるメニューフォントを読み込む
  if (!ImGui::GetIO().Fonts->AddFontFromFileTTF(config.getMenuFont().c_str(), config.getMenuFontSize(),
    nullptr, ImGui::GetIO().Fonts->GetGlyphRangesJapanese()))
  {
    // メニューフォントが読み込めなかったらエラーにする
    throw std::runtime_error("Cannot find any menu fonts.");
  }

#if defined(_WIN32)
  // 初期状態で最初のデバイスのフォーマットリストを取得しておく
  if (!config.getDeviceList().empty())
  {
    capture.updateFormatList(deviceNumber);
  }
#else
  // バックエンドごとのキャプチャデバイスの一覧を初期化する
  for (auto& [api, name] : backendList)
  {
    // バックエンドごとに空のリストを追加する
    deviceList.emplace(api, std::vector<std::string>());
  }

  // キャプチャデバイスの一覧を作る
  getAnyList(deviceList.at(cv::CAP_ANY));
#if defined(_MSC_VER)
  getDirectShowList(deviceList.at(cv::CAP_DSHOW));
  getMediaFoundationList(deviceList.at(cv::CAP_MSMF));
#elif defined(__APPLE__)
  getAvFoundationList(deviceList.at(cv::CAP_AVFOUNDATION));
#elif defined(__linux__)
#  if defined(USE_LIBCAMERA)
  deviceList.at(CAP_LIBCAMERA) = CamLibcam::getDeviceList();
  if (!deviceList.at(CAP_LIBCAMERA).empty())
  {
    backend = CAP_LIBCAMERA;
  }
#  endif
  getV4L2List(deviceList.at(cv::CAP_V4L2));
#endif
#endif
}

//
// デストラクタ
//
Menu::~Menu()
{
  // キャプチャスレッドが動いていたら止める
  capture.stop();

  // 前に開いていたキャプチャデバイスを閉じる
  capture.close();

  // ファイルダイアログ (Native File Dialog Extended) を終了する
  NFD_Quit();
}

//
// 入力画像に合わせて内部パラメータを初期化する
//
void Menu::initializeInputIntrinsics(const std::array<int, 2>& size)
{
  // 実際の入力解像度を処理系へ反映する
  intrinsics.size = size;

  // 無効な入力によるゼロ除算を避け、有効な場合だけ焦点距離から初期画角を求める
  if (size[0] > 0 && size[1] > 0 && settings.focal > 0.0f)
  {
    intrinsics.setFov(settings.focal);
  }
}

//
// 選択中の入力設定を適用してキャプチャを開始する
//
bool Menu::startCapture()
{
  // オープンとフォーマット適用を一つの入口に集約し、失敗時は開始処理を中断する
  if (!openDevice()) return false;

  // デバイスが確定してから動作モードと入力に合う初期画角を反映し、取得スレッドを開始する
  capture.setPrioritizeLatency(prioritizeLatency);
  initializeInputIntrinsics(capture.getSize());
  capture.start();
  return true;
}

//
// 選択中の投影方式とその内部パラメータを同期する
//
void Menu::selectPreference(int index)
{
  // 不正な選択番号では現在の投影状態を変更しない
  if (index < 0 || index >= static_cast<int>(config.getPreferences().size())) return;

  // 入力中の実解像度を、投影方式の構成値で上書きしないため退避する
  const auto size{ intrinsics.size };
  preferenceNumber = index;
  intrinsics = getPreference().getIntrinsics();

  // 入力中は実解像度だけを戻し、画角と中心位置は選択した投影方式の設定値を使用する
  if (capture.isOpened()) intrinsics.size = size;
}

#if defined(_WIN32)
//
// 解像度、フレームレート、コーデックの選択リストを更新する
//
void Menu::updateFormatDropdowns()
{
  const auto& formatList{ capture.getFormatList() };

  availableFormats = formatList;
  uniqueResolutions.clear();
  uniqueFpsList.clear();
  uniqueCodecs.clear();

  // 構造化されたフォーマット情報から選択肢を作成する
  for (const auto& info : availableFormats)
  {
    if (std::find(uniqueResolutions.begin(), uniqueResolutions.end(), info.resolution) == uniqueResolutions.end())
      uniqueResolutions.push_back(info.resolution);
    if (std::find(uniqueFpsList.begin(), uniqueFpsList.end(), info.fps) == uniqueFpsList.end())
      uniqueFpsList.push_back(info.fps);
    if (std::find(uniqueCodecs.begin(), uniqueCodecs.end(), info.codec) == uniqueCodecs.end())
      uniqueCodecs.push_back(info.codec);
  }

  // 現在の formatNumber のフォーマットに同期する
  const auto selected{ std::find_if(availableFormats.begin(), availableFormats.end(),
    [this](const CaptureFormat& info) { return info.index == formatNumber; }) };
  if (selected != availableFormats.end())
  {
    currentRes = selected->resolution;
    currentFps = selected->fps;
    currentCodec = selected->codec;
  }
  else
  {
    currentRes.clear();
    currentFps.clear();
    currentCodec.clear();
  }
  lastDeviceNumber = deviceNumber;
}
#endif

//
// 歪み補正シェーダを設定する
//
std::array<GLsizei, 2> Menu::setupUndistortion(GLfloat aspect) const
{
  // 現在の投影設定から歪み補正用シェーダを取り出して設定する。
  const auto& preference{ config.getPreferences()[preferenceNumber] };
  const auto& shader{ preference.getUndistortionShader() };

  return shader.setup(settings.samples, aspect, pose, intrinsics.fov,
    intrinsics.center, settings.getFocal(), config.getBackground(),
    undistortion.getCameraParameters(),
    undistortion.getDistortionParameters(), intrinsics.size);
}

//
// シェーダを設定する
//
std::array<GLsizei, 2> Menu::setup(GLfloat aspect) const
{
  // 補正方式に関わらず、展開パスでは現在の投影方式のシェーダを使用する。
  const auto& preference{ config.getPreferences()[preferenceNumber] };
  const auto& shader{ preference.getShader() };

  return shader.setup(settings.samples, aspect, pose, intrinsics.fov,
    intrinsics.center, settings.getFocal(), config.getBackground(),
    undistortion.getCameraParameters(),
    undistortion.getDistortionParameters(), intrinsics.size);
}

//
// メインメニューバーの描画
//
void Menu::drawMainMenuBar()
{
  if (ImGui::BeginMainMenuBar())
  {
    // ファイルメニュー
    if (ImGui::BeginMenu(u8"ファイル"))
    {
      // 画像ファイルを開く
      if (ImGui::MenuItem(u8"画像ファイルを開く")) openImage();

      // 動画ファイルを開く
      if (ImGui::MenuItem(u8"動画ファイルを開く")) openMovie();

      // 構成ファイルを開く
      if (ImGui::MenuItem(u8"構成ファイルを開く")) loadConfig();

      // 構成ファイルを保存する
      if (ImGui::MenuItem(u8"構成ファイルを保存")) saveConfig();

      // キャリブレーションパラメータファイルを開く
      if (ImGui::MenuItem(u8"較正ファイルを開く")) loadCalibration();

      // 終了
      quit = ImGui::MenuItem(u8"終了");

      // File メニュー修了
      ImGui::EndMenu();
    }

    // ウィンドウメニュー
    if (ImGui::BeginMenu(u8"ウィンドウ"))
    {
      // 入力パネルの表示
      ImGui::MenuItem(u8"入力", NULL, &showInputPanel);

      // ArUco パネルの表示
      ImGui::MenuItem(u8"ArUco", NULL, &showArucoPanel);

      // ウィンドウメニュー終了
      ImGui::EndMenu();
    }

    // メニューバーの高さを保存しておく
    menubarHeight = static_cast<GLsizei>(ImGui::GetWindowHeight());

    // メインメニューバー終了
    ImGui::EndMainMenuBar();
  }
}

//
// 入力パネルの描画
//
void Menu::drawInputPanel()
{
  // 入力パネル
  if (showInputPanel)
  {
    // ウィンドウの位置とサイズ
    ImGui::SetNextWindowPos(ImVec2(2.0f, 2.0f + menubarHeight), ImGuiCond_Once);
#if defined(_WIN32)
    ImGui::SetNextWindowSize(ImVec2(262, 576), ImGuiCond_Once);
#else
    ImGui::SetNextWindowSize(ImVec2(262, 606), ImGuiCond_Once);
#endif
    ImGui::Begin(u8"入力", &showInputPanel);

    // 3種類の処理経路をラジオボタンで排他的に選択する。
    ImGui::TextUnformatted(u8"歪み補正");

    // 「なし」は較正ファイルの有無に関係なく選択できる。
    if (ImGui::RadioButton(u8"なし",
      undistortionMode == UndistortionMode::None))
      undistortionMode = UndistortionMode::None;
    ImGui::SameLine();
    // OpenCV方式はCPU上で処理するので、有効な較正値がある場合だけ選択を許可する。
    if (ImGui::RadioButton("OpenCV",
      undistortionMode == UndistortionMode::OpenCV))
    {
      if (undistortion.ready())
        undistortionMode = UndistortionMode::OpenCV;
      else
        errorMessage = u8"先に較正ファイルを読み込んでください";
    }
    ImGui::SameLine();
    // OpenGL方式も同じ較正値をuniformへ渡すため、先にファイルを要求する。
    if (ImGui::RadioButton("OpenGL",
      undistortionMode == UndistortionMode::OpenGL))
    {
      if (undistortion.ready())
        undistortionMode = UndistortionMode::OpenGL;
      else
        errorMessage = u8"先に較正ファイルを読み込んでください";
    }
    ImGui::Separator();

    // 投影方式の選択
    if (ImGui::BeginCombo(u8"投影方式", getPreference().getDescription().c_str()))
    {
      // すべての投影方式について
      for (int i = 0; i < static_cast<int>(config.getPreferences().size()); ++i)
      {
        // その投影方式が選択されていれば真
        const bool selected{ i == preferenceNumber };

        // 投影方式を（それが現在の投影方式ならハイライトして）コンボボックスに表示する
        if (ImGui::Selectable(getPreference(i).getDescription().c_str(), selected))
        {
          // 表示した投影方式が選択されていたらそれを現在の選択とする
          selectPreference(i);
        }

        // この選択を次にコンボボックスを開いたときのデフォルトにしておく
        if (selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    // レンズの画角を設定する
    ImGui::DragFloat2(u8"画角", intrinsics.fov.data(), 0.1f, -360.0f, 360.0f, "%.2f");

    // レンズの中心位置
    ImGui::DragFloat2(u8"中心", intrinsics.center.data(), 0.001f, -1.0f, 1.0f, "%.4f");

    // キャプチャデバイス固有のパラメータを元に戻す
    if (ImGui::Button(u8"回復"))
    {
      // 選択した投影方式のキャプチャデバイス固有のパラメータを回復する
      intrinsics = getPreference().getIntrinsics();
    }

    // 姿勢
    ImGui::SliderAngle(u8"方位", &settings.euler[1], -180.0f, 180.0f, "%.2f");
    ImGui::SliderAngle(u8"仰角", &settings.euler[0], -180.0f, 180.0f, "%.2f");
    ImGui::SliderAngle(u8"傾斜", &settings.euler[2], -180.0f, 180.0f, "%.2f");
    pose = ggRotateY(settings.euler[1]).rotateX(settings.euler[0]).rotateZ(settings.euler[2]);

    // 焦点距離
    ImGui::SliderFloat(u8"焦点距離", &settings.focal, settings.focalRange[0], settings.focalRange[1], "%.1f");

    // 姿勢を元に戻す
    if (ImGui::Button(u8"復帰"))
    {
      settings.euler = config.getSettings().euler;
      settings.focal = config.getSettings().focal;
      settings.focalRange = config.getSettings().focalRange;
    }

    ImGui::Separator();

#if defined(_WIN32)
    // キャプチャデバイスが存在するとき
    if (!config.getDeviceList().empty())
    {
      // 装置関連項目
      ImGui::Text("%s", u8"以下の変更は [開始] で反映します");

      // キャプチャデバイスの選択コンボボックス
      if (ImGui::BeginCombo(u8"装置", config.getDeviceName(deviceNumber).c_str()))
      {
        // すべてのキャプチャデバイスについて
        for (int i = 0; i < static_cast<int>(config.getDeviceList().size()); ++i)
        {
          // キャプチャデバイス名を (それを選択していればハイライトして) コンボボックスに表示する
          if (ImGui::Selectable(config.getDeviceName(i).c_str(), i == deviceNumber))
          {
            // キャプチャデバイスが変わったら
            if (deviceNumber != i)
            {
              // キャプチャスレッドが動いていたら止める
              capture.stop();

              // 前に開いていたキャプチャデバイスを閉じる
              capture.close();

              // 表示したキャプチャデバイスが選択されていたらそのキャプチャデバイスを選択する
              deviceNumber = i;

              // キャプチャデバイスが変わったので最初のビデオフォーマットを選択する
              formatNumber = 0;

              // 開始ボタンが押されるまでは、フォーマットリストだけを一時取得して更新する
              capture.updateFormatList(deviceNumber);

              // 選択可能なドロップダウンのリストを更新する
              updateFormatDropdowns();
            }

            // この選択を次にコンボボックスを開いたときのデフォルトにしておく
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }

      // 使用可能なビデオフォーマットの表示名のリスト
      const auto& formatList{ capture.getFormatList() };

      // 使用可能なビデオフォーマットが存在するなら
      if (!formatList.empty())
      {
        // 必要な場合（デバイス番号の不一致やリスト未作成時）にリストを更新する
        if (lastDeviceNumber != deviceNumber || availableFormats.empty())
        {
          updateFormatDropdowns();
        }

        // 1. 解像度の選択コンボボックス
        if (ImGui::BeginCombo(u8"解像度", currentRes.c_str()))
        {
          for (const auto& res : uniqueResolutions)
          {
            if (ImGui::Selectable(res.c_str(), currentRes == res))
            {
              currentRes = res;
            }
          }
          ImGui::EndCombo();
        }

        // 2. フレームレートの選択コンボボックス
        std::string fpsLabel = currentFps + " fps";
        if (ImGui::BeginCombo(u8"コマ数", fpsLabel.c_str()))
        {
          for (const auto& fpsVal : uniqueFpsList)
          {
            std::string valLabel = fpsVal + " fps";
            if (ImGui::Selectable(valLabel.c_str(), currentFps == fpsVal))
            {
              currentFps = fpsVal;
            }
          }
          ImGui::EndCombo();
        }

        // 3. コーデックの選択コンボボックス
        if (ImGui::BeginCombo(u8"符号化", currentCodec.c_str()))
        {
          for (const auto& cod : uniqueCodecs)
          {
            if (ImGui::Selectable(cod.c_str(), currentCodec == cod))
            {
              currentCodec = cod;
            }
          }
          ImGui::EndCombo();
        }

        // 選択された組み合わせが availableFormats に存在するか探す
        int foundIndex = -1;
        for (const auto& info : availableFormats)
        {
          if (info.resolution == currentRes && info.fps == currentFps && info.codec == currentCodec)
          {
            foundIndex = info.index;
            break;
          }
        }

        bool formatExists = (foundIndex != -1);
        if (formatExists)
        {
          // 存在する場合は、必要なら formatNumber を更新する
          if (formatNumber != foundIndex)
          {
            capture.stop();
            formatNumber = foundIndex;
          }
        }

        // 4. レイテンシ優先のチェックボックス
        if (ImGui::Checkbox(u8"レイテンシ優先", &prioritizeLatency))
        {
          if (capture) capture.setPrioritizeLatency(prioritizeLatency);
        }

        // キャプチャの開始と停止
        if (capture)
        {
          // キャプチャスレッドが動いているので止める
          if (ImGui::Button(u8"停止")) capture.stop();
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.0f, 1.0f), "%s", u8"取得中");
        }
        else
        {
          if (formatExists)
          {
            // 「開始」ボタンをクリックしたときデバイスが選択されていれば
            if (ImGui::Button(u8"開始") && deviceNumber >= 0)
            {
              startCapture();
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", u8"停止中");
          }
          else
          {
            // 存在しない組み合わせの時はメッセージを表示する
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", u8"フォーマットが存在ません");
          }
        }
      }
      else
      {
        // キャプチャデバイスが開けなかった
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", u8"デバイスが開けません");
      }
    }
    else
    {
      // キャプチャデバイスが存在しないとき
      ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", u8"デバイスが見つかりません");
    }
#else
    // 装置関連項目
    ImGui::Text("%s", u8"以下の変更は [開始] で反映します");

    // デバイスプリファレンスを選択する
    if (ImGui::BeginCombo(u8"装置特性", backendList.at(backend)))
    {
      // すべての表示方式について
      for (auto& [apiId, apiName] : backendList)
      {
        // その表示方式が選択されていれば真
        const bool selected{ apiId == backend };

        // 装置特性を（それが現在の装置特性ならハイライトして）コンボボックスに表示する
        if (ImGui::Selectable(apiName, selected))
        {
          // 表示した装置特性が選択されていたらそれを現在の選択とする
          backend = apiId;

          // 切り替え前の装置特性のデバイスが存在しなければ最初のデバイスの番号を選択する
          if (deviceNumber < 0) deviceNumber = 0;

          // 選択されているデバイスの番号が接続されたキャプチャデバイスの数を超えないようにする
          const int count{ getDeviceCount(backend) };
          if (deviceNumber >= count) deviceNumber = count - 1;
        }

        // この選択を次にコンボボックスを開いたときのデフォルトにしておく
        if (selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    // キャプチャデバイスが存在すれば
    if (deviceNumber >= 0)
    {
      // キャプチャデバイスの選択コンボボックス
      if (ImGui::BeginCombo(u8"入力源", getDeviceName(backend, deviceNumber).c_str()))
      {
        // すべてのキャプチャデバイスについて
        for (int i = 0; i < static_cast<int>(getDeviceList(backend).size()); ++i)
        {
          // キャプチャデバイス名を（それを選択していればハイライトして）コンボボックスに表示する
          if (ImGui::Selectable(getDeviceName(backend, i).c_str(), i == deviceNumber))
          {
            // 表示したキャプチャデバイスが選択されていたらそのキャプチャデバイスを選択する
            deviceNumber = i;

            // この選択を次にコンボボックスを開いたときのデフォルトにしておく
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
    }
    else
    {
      // 使えるキャプチャデバイスがない
      ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", u8"デバイスが見つかりません");
    }

    // キャプチャするサイズとフレームレート
    ImGui::InputInt2(u8"解像度", intrinsics.size.data());
    ImGui::InputDouble(u8"周波数", &intrinsics.fps, 1.0f, 1.0f, "%.1f");

    // コーデックを選択する
    if (ImGui::BeginCombo(u8"符号化", codecList[codecNumber]))
    {
      // すべてのコーデックについて
      for (int i = 0; i < static_cast<int>(codecList.size()); ++i)
      {
        // コーデックを（それを選択していればハイライトして）コンボボックスに表示する
        if (ImGui::Selectable(codecList[i], i == codecNumber))
        {
          // 表示したキャプチャデバイスが選択されていたらそのキャプチャデバイスを選択する
          codecNumber = i;

          // この選択を次にコンボボックスを開いたときのデフォルトにしておく
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    // キャプチャの開始と停止
    if (capture)
    {
      // キャプチャスレッドが動いているので止める
      if (ImGui::Button(u8"停止")) capture.stop();
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.0f, 1.0f), "%s", u8"取得中");
    }
    else
    {
      // キャプチャスレッドが止まっているので
      if (ImGui::Button(u8"開始") && deviceNumber >= 0)
      {
        startCapture();
      }
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", u8"停止中");
    }
#endif
    ImGui::End();
  }
}

//
// ArUco パネルの描画
//
void Menu::drawArucoPanel()
{
  // ArUco パネルを表示するなら
  if (showArucoPanel)
  {
    // ウィンドウの位置と初期サイズを設定する
    ImGui::SetNextWindowPos(ImVec2(270.0f, 2.0f + menubarHeight), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(222, 133), ImGuiCond_Once);
    ImGui::Begin(u8"ArUco", &showArucoPanel);

    // 認識に使用する ArUco Marker 辞書の選択コンボボックス
    if (ImGui::BeginCombo(u8"辞書", settings.dictionaryName.c_str()))
    {
      // 事前定義されたすべての辞書について選択肢を表示する
      for (auto d = aruco.dictionaryList.begin(); d != aruco.dictionaryList.end(); ++d)
      {
        // 現在選択中の辞書ならハイライトする
        const bool selected{ d->first == settings.dictionaryName };

        // 辞書名をコンボボックスに表示する
        if (ImGui::Selectable(d->first.c_str(), selected))
        {
          // 選択された辞書名を保存し、検出器を再生成する
          settings.dictionaryName = d->first;
          aruco.setDictionary(settings.dictionaryName);
        }

        // 次回コンボボックスを開いたときに現在の選択位置にフォーカスする
        if (selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    ImGui::Separator();

    // ArUco Marker 認識の有効／無効チェックボックス
    ImGui::Checkbox(u8"ArUco Marker 検出", &detectMarker);

    // 姿勢推定および座標軸表示に使用する ArUco Marker の一辺の長さ (cm)
    ImGui::InputFloat(u8"マーカ長", &settings.markerLength, 0.0f, 0.0f, "%.2f cm");

    ImGui::End();
  }
}

//
// エラーダイアログの描画
//
void Menu::drawErrorDialog()
{
  // エラーメッセージが設定されていたら
  if (errorMessage)
  {
    // ウィンドウの位置・サイズとタイトル
    ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(272, 92), ImGuiCond_Always);

    // ウィンドウを表示するとき true
    bool status{ true };

    // エラーメッセージウィンドウを表示する
    ImGui::Begin(u8"エラー", &status);

    // エラーメッセージの表示
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", errorMessage);

    // クローズボックスか「閉じる」ボタンをクリックしたら
    if (!status || ImGui::Button(u8"閉じる"))
    {
      // エラーメッセージを消去する
      errorMessage = nullptr;
    }
    ImGui::End();
  }
}

//
// メニューの描画
//
void Menu::draw()
{
  // 各ウィンドウの描画責務を分離し、この関数では一フレーム分の呼び出し順だけを管理する
  drawMainMenuBar();
  drawInputPanel();
  drawArucoPanel();
  drawErrorDialog();
}
