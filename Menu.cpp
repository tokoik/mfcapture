///
/// メニューの描画クラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///
#include "Menu.h"

// ImGui
#if !defined(__ANDROID__)
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#endif

// ファイルダイアログ
#if !defined(__ANDROID__)
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
#endif

// 標準ライブラリ
#include <sstream>
#include <limits>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
namespace
{
  //
  // 解像度の表示文字列 (例: "1280 x 720") から幅と高さを取り出す
  //
  bool parseResolution(const std::string& resolution, int& width, int& height)
  {
    return std::sscanf(resolution.c_str(), "%d x %d", &width, &height) == 2;
  }

  //
  // フォーマットが未選択のときに使う既定のフォーマットを選ぶ
  //
  // 1280 x 720 があればそれを選び、なければ画素数が 1280 x 720 に近いものを選ぶ。
  // 1920 x 1080 を超える解像度はデコード負荷が大きいため優先度を下げる。
  //
  std::vector<CaptureFormat>::const_iterator findDefaultFormat(const std::vector<CaptureFormat>& formats)
  {
    constexpr long long targetArea{ 1280LL * 720LL };
    constexpr long long largeArea{ 1920LL * 1080LL };
    constexpr long long largePenalty{ 10000000LL };

    auto best{ formats.begin() };
    long long bestScore{ std::numeric_limits<long long>::max() };
    for (auto it = formats.begin(); it != formats.end(); ++it)
    {
      int width{ 0 }, height{ 0 };
      if (!parseResolution(it->resolution, width, height)) continue;
      if (width == 1280 && height == 720) return it;

      const long long area{ static_cast<long long>(width) * height };
      const long long score{ std::llabs(area - targetArea) + (area > largeArea ? largePenalty : 0LL) };
      if (score < bestScore)
      {
        bestScore = score;
        best = it;
      }
    }
    return best;
  }
}
#endif

#if !defined(_WIN32) && !defined(__ANDROID__) && !defined(__APPLE__)
// バックエンドのリスト
const std::map<cv::VideoCaptureAPIs, const char*> Menu::backendList
{
#  if defined(USE_LIBCAMERA)
  { CAP_LIBCAMERA, "libcamera" },
#  endif
  { cv::CAP_ANY, "(any)" },
#  if defined(__linux__)
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

#  if defined(__linux__)
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
#endif // !defined(_WIN32) && !defined(__ANDROID__) && !defined(__APPLE__)

//
// キャプチャデバイスを開く
//
bool Menu::openDevice()
{
#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
  // 何のデバイスも接続されていなければ戻る
  if (deviceNumber < 0) return false;

  // 前に開いていたキャプチャデバイスを (キャプチャスレッドを止めてから) 閉じる
  capture.close();

  // 選択したキャプチャデバイスを開く
  if (capture.openDevice(deviceNumber))
  {
    // フォーマットの選択肢を作り直す (未選択なら既定のフォーマットを選ぶ)
    updateFormatDropdowns();

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

  // 実解像度と焦点距離から、この入力を見やすく表示する初期画角を設定する
  initializeInputIntrinsics(capture.getSize());

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

#if !defined(__ANDROID__)
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

    // ダイアログで指定した動画ファイルが開けたら
    if (capture.openMovie(filepath))
    {
      // 実解像度と焦点距離から、この動画を見やすく表示する初期画角を設定する
      initializeInputIntrinsics(capture.getSize());
    }
    else
    {
      // 開けなかった
      errorMessage = u8"動画ファイルが開けません";
    }

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
#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
    else
    {
      // 較正ファイルに解像度が記録されていれば、最も近い解像度に自動で切り替える
      const auto& calibSize{ undistortion.getImageSize() };
      if (calibSize.width > 0 && calibSize.height > 0)
      {
        selectBestResolution(calibSize.width, calibSize.height);
      }
    }
#endif

    // 較正値が変わったので、入力の解像度と合っているか調べ直す
    checkCalibrationSize();

    // ファイルパスの取り出しに使ったメモリを開放する
    NFD_FreePath(filepath);
  }
}
#endif

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
#if !defined(__ANDROID__)
  NFD_Init();
#endif

  // Dear ImGui の入力デバイス
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // キーボードコントロールを使う
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // ゲームパッドを使う

  // Dear ImGui のスタイル
  //ImGui::StyleColorsDark();                                 // 暗めのスタイル
  //ImGui::StyleColorsClassic();                              // 以前のスタイル

#if !defined(__ANDROID__)
  // 日本語を表示できるメニューフォントを読み込む
  // 基本の日本語グリフセット（常用・人名用漢字、ひらがな、カタカナ、英数字）に加え、
  // デバイス名等に含まれる一般句読点（引用符、ダッシュ等）や文字様記号（商標記号等）を追加する
  ImFontGlyphRangesBuilder builder;
  builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesJapanese());

  // 追加の Unicode 範囲 (2要素で1範囲、0終端)
  static const ImWchar additionalRanges[] =
  {
    0x2000, 0x206F, // General Punctuation (引用符、ダッシュ、リーダー等)
    0x2100, 0x214F, // Letterlike Symbols (商標記号 TM 等)
    0x2190, 0x21FF, // Arrows (矢印記号等)
    0x2460, 0x24FF, // Enclosed Alphanumerics (丸数字・囲み英数字等)
    0x25A0, 0x25FF, // Geometric Shapes (幾何学模様・図形記号等)
    0,
  };
  builder.AddRanges(additionalRanges);

  // フォントアトラス構築時までメモリを維持するため static で保持する
  static ImVector<ImWchar> glyphRanges;
  builder.BuildRanges(&glyphRanges);

  if (!ImGui::GetIO().Fonts->AddFontFromFileTTF(config.getMenuFont().c_str(), config.getMenuFontSize(),
    nullptr, glyphRanges.Data))
  {
    // メニューフォントが読み込めなかったらエラーにする
    throw std::runtime_error("Cannot find any menu fonts.");
  }
#endif

#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
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
#  if defined(__linux__)
#    if defined(USE_LIBCAMERA)
  deviceList.at(CAP_LIBCAMERA) = CamLibcam::getDeviceList();
  if (!deviceList.at(CAP_LIBCAMERA).empty())
  {
    backend = CAP_LIBCAMERA;
  }
#    endif
  getV4L2List(deviceList.at(cv::CAP_V4L2));
#  endif
#endif
}

//
// デストラクタ
//
Menu::~Menu()
{
  // 前に開いていたキャプチャデバイスを (キャプチャスレッドを止めてから) 閉じる
  capture.close();

  // ファイルダイアログ (Native File Dialog Extended) を終了する
#if !defined(__ANDROID__)
  NFD_Quit();
#endif
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

  // 入力の解像度が変わったので、較正時の解像度と合っているか調べ直す
  checkCalibrationSize();
}

//
// 入力画像の解像度が較正時の解像度と合っているか調べて警告を設定する
//
void Menu::checkCalibrationSize()
{
  // 較正値を読み込んでいなければ警告しない
  warningMessage = nullptr;
  if (!undistortion.ready()) return;

  // 入力の解像度と較正時の解像度の関係に応じた警告を設定する
  warningMessage = getCalibrationWarningText(
    undistortion.matchSize(cv::Size{ intrinsics.size[0], intrinsics.size[1] }));
}

//
// 較正時の解像度との関係に応じた警告の文言を得る
//
const char* Menu::getCalibrationWarningText(CalibrationSizeMatch match)
{
  switch (match)
  {
  case CalibrationSizeMatch::Scaled:
    return u8"入力の解像度が較正時と異なるので、較正値を換算して補正します。"
      u8"解像度によって視野の切り出し方が変わるカメラでは、補正がずれます。";

  case CalibrationSizeMatch::AspectMismatch:
    return u8"入力のアスペクト比が較正時と異なるので、正しく補正できません。"
      u8"較正時と同じアスペクト比の解像度を選んでください。";

  default:
    return nullptr;
  }
}

//
// 選択中の入力設定を適用してキャプチャを開始する
//
bool Menu::startCapture()
{
  // オープン、フォーマット適用、初期画角の設定を一つの入口に集約し、失敗時は開始処理を中断する
  if (!openDevice()) return false;

  // デバイスが確定してから動作モードを反映し、取得スレッドを開始する
  capture.setPrioritizeLatency(prioritizeLatency);
  capture.start();
  return true;
}

//
// 選択中の投影方式とその内部パラメータを同期する
//
void Menu::selectPreference(int index)
{
#if !defined(__ANDROID__)
  // 不正な選択番号では現在の投影状態を変更しない
  if (index < 0 || index >= static_cast<int>(config.getPreferences().size())) return;

  // 入力中の実解像度を、投影方式の構成値で上書きしないため退避する
  const auto size{ intrinsics.size };
  preferenceNumber = index;
  intrinsics = getPreference().getIntrinsics();

  // 入力中は実解像度だけを戻し、画角と中心位置は選択した投影方式の設定値を使用する
  if (capture.isOpened()) intrinsics.size = size;
#endif
}

#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
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
  auto selected{ std::find_if(availableFormats.cbegin(), availableFormats.cend(),
    [this](const CaptureFormat& info) { return info.index == formatNumber; }) };

  // formatNumber が未設定または範囲外の場合は既定のフォーマットを自動選択する
  if (selected == availableFormats.cend() && !availableFormats.empty())
  {
    selected = findDefaultFormat(availableFormats);
    formatNumber = selected->index;
  }

  if (selected != availableFormats.cend())
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

//
// 解像度、フレームレート、コーデックのいずれかを選択し、実在する組み合わせに同期する
//
void Menu::selectFormatItem(std::string CaptureFormat::* field, const std::string& value)
{
  // 選択した項目だけを変更し、他の項目は現在の選択を維持した組み合わせを作る
  CaptureFormat wanted{ currentRes, currentFps, currentCodec, formatNumber };
  wanted.*field = value;

  // 選択した項目が一致するフォーマットなら真
  const auto sameItem{ [field, &value](const CaptureFormat& f) { return f.*field == value; } };

  // 組み合わせがそのまま実在すればそれを使い、なければ解像度を維持できるもの、
  // それもなければ選択した項目が一致する最初のフォーマットへ同期する
  auto it{ std::find_if(availableFormats.cbegin(), availableFormats.cend(), [&wanted](const CaptureFormat& f)
    { return f.resolution == wanted.resolution && f.fps == wanted.fps && f.codec == wanted.codec; }) };
  if (it == availableFormats.cend())
  {
    it = std::find_if(availableFormats.cbegin(), availableFormats.cend(), [&](const CaptureFormat& f)
      { return sameItem(f) && f.resolution == wanted.resolution; });
  }
  if (it == availableFormats.cend())
  {
    it = std::find_if(availableFormats.cbegin(), availableFormats.cend(), sameItem);
  }

  // 同期先が見つからなければ選択した値だけを反映する (「フォーマットが存在しません」と表示される)
  const auto& result{ it != availableFormats.cend() ? *it : wanted };
  currentRes = result.resolution;
  currentFps = result.fps;
  currentCodec = result.codec;
}
#endif

#if !defined(__ANDROID__)
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
    undistortion.getCameraParameters(cv::Size{ intrinsics.size[0], intrinsics.size[1] }),
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
    undistortion.getCameraParameters(cv::Size{ intrinsics.size[0], intrinsics.size[1] }),
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
    const float uiScale{ ImGui::GetIO().FontGlobalScale };
    const float displayW{ ImGui::GetIO().DisplaySize.x };
    const float displayH{ ImGui::GetIO().DisplaySize.y };

    const float posY{ 2.0f + menubarHeight };
#if defined(_WIN32)
    const float targetH{ 576.0f * uiScale };
#else
    const float targetH{ 606.0f * uiScale };
#endif
    const float maxH{ (displayH > posY + 10.0f) ? (displayH - posY - 4.0f) : targetH };
    const float winW{ (displayW > 10.0f) ? std::min(262.0f * uiScale, displayW - 4.0f) : 262.0f * uiScale };
    const float winH{ std::min(targetH, maxH) };

    ImGui::SetNextWindowPos(ImVec2(2.0f, posY), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(100.0f * uiScale, 100.0f * uiScale), ImVec2(displayW, maxH));
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

#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
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
              // 前に開いていたキャプチャデバイスを (キャプチャスレッドを止めてから) 閉じる
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
              selectFormatItem(&CaptureFormat::resolution, res);
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
              selectFormatItem(&CaptureFormat::fps, fpsVal);
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
              selectFormatItem(&CaptureFormat::codec, cod);
            }
          }
          ImGui::EndCombo();
        }

        // 選択された組み合わせが availableFormats に存在するか探す
        const auto found{ std::find_if(availableFormats.cbegin(), availableFormats.cend(),
          [this](const CaptureFormat& info)
          { return info.resolution == currentRes && info.fps == currentFps && info.codec == currentCodec; }) };
        const bool formatExists{ found != availableFormats.cend() };

        // 存在する組み合わせに変わったら取得を止め、次の [開始] でそのフォーマットを適用する
        if (formatExists && formatNumber != found->index)
        {
          capture.stop();
          formatNumber = found->index;
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
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", u8"フォーマットが存在しません");
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
    const float uiScale{ ImGui::GetIO().FontGlobalScale };
    const float displayW{ ImGui::GetIO().DisplaySize.x };
    const float displayH{ ImGui::GetIO().DisplaySize.y };

    const float panelW{ 222.0f * uiScale };
    float posX{ 270.0f * uiScale };
    float posY{ 2.0f + menubarHeight };
    if (posX + panelW > displayW && displayW > 0.0f)
    {
      posX = 20.0f * uiScale;
      posY += 30.0f * uiScale;
    }
    const float targetH{ 133.0f * uiScale };
    const float maxH{ (displayH > posY + 10.0f) ? (displayH - posY - 4.0f) : targetH };
    const float winW{ (displayW > 10.0f) ? std::min(panelW, displayW - 4.0f) : panelW };
    const float winH{ std::min(targetH, maxH) };

    ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(100.0f * uiScale, 100.0f * uiScale), ImVec2(displayW, maxH));
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
    const float uiScale{ ImGui::GetIO().FontGlobalScale };
    ImGui::SetNextWindowPos(ImVec2(60.0f * uiScale, 60.0f * uiScale), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(272.0f * uiScale, 92.0f * uiScale), ImGuiCond_Always);

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

  // 較正時の解像度に関する警告が設定されていたら
  if (warningMessage)
  {
    // ウィンドウの位置・サイズとタイトル (エラーと重ならない位置に置く)
    const float uiScale{ ImGui::GetIO().FontGlobalScale };
    ImGui::SetNextWindowPos(ImVec2(60.0f * uiScale, 160.0f * uiScale), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(360.0f * uiScale, 0.0f), ImGuiCond_Always);

    // ウィンドウを表示するとき true
    bool status{ true };

    // 警告ウィンドウを表示する
    ImGui::Begin(u8"警告", &status);

    // 警告の表示 (長い文はウィンドウの幅で折り返す)
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s", warningMessage);
    ImGui::PopTextWrapPos();

    // クローズボックスか「閉じる」ボタンをクリックしたら
    if (!status || ImGui::Button(u8"閉じる"))
    {
      // 警告を消去する (補正は換算した較正値で続ける)
      warningMessage = nullptr;
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
#endif

#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
//
// カメラ解像度を選択する
//
bool Menu::selectResolution(const std::string& resolution)
{
  if (resolution == currentRes && bool(capture)) return true;

  for (const auto& item : availableFormats)
  {
    if (item.resolution == resolution)
    {
      const std::string prevRes{ currentRes };
      const std::string prevFps{ currentFps };
      const std::string prevCodec{ currentCodec };
      const int prevFormatNumber{ formatNumber };

      currentRes = resolution;
      currentFps = item.fps;
      currentCodec = item.codec;
      formatNumber = item.index;

      if (capture.isOpened())
      {
        const bool wasRunning{ bool(capture) };
        if (wasRunning) capture.stop();
        if (capture.select(formatNumber))
        {
          initializeInputIntrinsics(capture.getSize());
          if (wasRunning)
          {
            capture.start();
            if (!bool(capture))
            {
              currentRes = prevRes;
              currentFps = prevFps;
              currentCodec = prevCodec;
              formatNumber = prevFormatNumber;
              return false;
            }
          }
          return true;
        }

        // フォーマット選択自体が失敗した場合は元に戻す
        currentRes = prevRes;
        currentFps = prevFps;
        currentCodec = prevCodec;
        formatNumber = prevFormatNumber;
        if (wasRunning) capture.start();
      }
      break;
    }
  }

  return false;
}

//
// 指定された解像度に最も近いカメラ解像度を選択する
//
bool Menu::selectBestResolution(int targetWidth, int targetHeight)
{
  if (targetWidth <= 0 || targetHeight <= 0) return false;

  // 解像度ごとに一度だけ評価すれば十分なので、重複のない解像度のリストから選ぶ。
  // 優先順位は (1) 同じ解像度、(2) アスペクト比が同じで画素数が最も近い解像度、
  // (3) アスペクト比が最も近く、その中で画素数が最も近い解像度とする。
  // アスペクト比が違うモードは視野の切り出し方が違うので、(3) は補正が正しくならない。
  const std::string* sameAspectRes{ nullptr };
  double sameAspectAreaDiff{ std::numeric_limits<double>::max() };
  const std::string* otherRes{ nullptr };
  double otherAspectDiff{ std::numeric_limits<double>::max() };
  double otherAreaDiff{ std::numeric_limits<double>::max() };

  const cv::Size target{ targetWidth, targetHeight };
  const double targetArea{ static_cast<double>(targetWidth) * targetHeight };
  const double targetAspect{ static_cast<double>(targetWidth) / targetHeight };

  for (const auto& res : uniqueResolutions)
  {
    int fw{ 0 }, fh{ 0 };
    if (!parseResolution(res, fw, fh) || fw <= 0 || fh <= 0) continue;

    // 完全に一致する解像度があればそれを使う
    if (fw == targetWidth && fh == targetHeight) return selectResolution(res);

    // 画素数の差
    const double areaDiff{ std::abs(static_cast<double>(fw) * fh - targetArea) };

    if (Undistortion::isSameAspect(cv::Size{ fw, fh }, target))
    {
      // アスペクト比が同じものの中では画素数が最も近いものを選ぶ
      if (areaDiff < sameAspectAreaDiff)
      {
        sameAspectAreaDiff = areaDiff;
        sameAspectRes = &res;
      }
    }
    else
    {
      // アスペクト比が違うものは、アスペクト比の差、画素数の差の順に比べる
      const double aspectDiff{ std::abs(static_cast<double>(fw) / fh - targetAspect) };
      if (aspectDiff < otherAspectDiff || (aspectDiff == otherAspectDiff && areaDiff < otherAreaDiff))
      {
        otherAspectDiff = aspectDiff;
        otherAreaDiff = areaDiff;
        otherRes = &res;
      }
    }
  }

  // 選んだ解像度に切り替える (警告は切り替え後の initializeInputIntrinsics() で設定される)
  const std::string* const bestRes{ sameAspectRes ? sameAspectRes : otherRes };
  return bestRes && selectResolution(*bestRes);
}
#endif
