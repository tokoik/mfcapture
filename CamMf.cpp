///
/// Microsoft Media Foundation を使ったビデオキャプチャクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date December 24, 2024
///
#include "CamMf.h"

// Microsoft Media Foundation
#pragma comment(lib, "MF.lib")
#pragma comment(lib, "MFplat.lib")
#pragma comment(lib, "MFreadwrite.lib")

//
// メモリの開放
//
template <class T> void SafeRelease(T** ppT)
{
  if (*ppT)
  {
    (*ppT)->Release();
    *ppT = nullptr;
  }
}

//
// GUID から人間が読める形式の名前を返すヘルパー関数
//
std::string SubTypeToName(const GUID& subType)
{
  if (subType == MFVideoFormat_YUY2) return "YUY2";
  if (subType == MFVideoFormat_NV12) return "NV12";
  if (subType == MFVideoFormat_RGB24) return "RGB24";
  if (subType == MFVideoFormat_RGB32) return "RGB32";
  if (subType == MFVideoFormat_MJPG) return "MJPG";
  if (subType == MFVideoFormat_H264) return "H264";

  // TODO: 他に使用するフォーマットがあればここに追加

  return "Unknown";
}

//
// ビデオフォーマットの詳細を保持する構造体のコンストラクタ
//
VideoFormat::VideoFormat(UINT32 width, UINT32 height, UINT32 fps_num, UINT32 fps_den, GUID subType)
  : width{ width }
  , height{ height }
  , fps_num{ fps_num }
  , fps_den{ fps_den }
  , subType{ subType }
{
  // 表示名を作成する
  std::stringstream ss;
  ss << width << " x " << height << " @ "
    << std::fixed << std::setprecision(2) << static_cast<double>(fps_num) / static_cast<double>(fps_den)
    << " fps (" << SubTypeToName(subType) << ")";

  // 表示名を保存する
  formatName = ss.str();
}

// COM ライブラリの初期化と終了を行うオブジェクト
CamMf::ComInitializer* CamMf::ComInitializer::instance{ nullptr };

//
// COM ライブラリの初期化と終了を行うクラスのコンストラクタ
//
CamMf::ComInitializer::ComInitializer() :
  deviceList{},
  ppSourceActivate{ nullptr },
  cSourceActivate{ 0 }
{
}

//
// COM ライブラリの初期化と終了を行うクラスのデストラクタ
//
CamMf::ComInitializer::~ComInitializer()
{
  if (this == instance)
  {
    // 後始末
    cleanup();

    // Media Foundation をシャットダウンし
    MFShutdown();

    // COM ライブラリを終了してから
    CoUninitialize();

    // インスタンスを解放する
    instance = nullptr;
  }
}

//
// 初期化
//
const char* CamMf::ComInitializer::initialize()
{
  // COM ライブラリを初期化する
  if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
  {
    // COM ライブラリの初期化に失敗したら戻る
    return "Failed to initialize COM library.";
  }

  // Media Foundation を起動する
  if (FAILED(MFStartup(MF_VERSION)))
  {
    // Media Foundation の起動に失敗したら COM ライブラリを終了して
    CoUninitialize();

    // 戻る
    return "Failed to start Media Foundation.";
  }

  // 検索条件を保持する属性ストア
  IMFAttributes* pAttributes{ nullptr };

  // 検索条件を保持する属性ストアを作成する
  if (FAILED(MFCreateAttributes(&pAttributes, 1)))
  {
    // 属性ストアの作成失敗したら Media Foundation を終了して
    MFShutdown();

    // COM ライブラリを終了して
    CoUninitialize();

    // 戻る
    return "Failed to create attribute store.";
  }

  // 属性ストアにビデオキャプチャデバイスの属性を設定する
  if (FAILED(pAttributes->SetGUID(
    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)))
  {
    // ビデオキャプチャデバイスの属性の設定に失敗したら属性ストアを解放して
    SafeRelease(&pAttributes);

    // Media Foundation を終了して
    MFShutdown();

    // COM ライブラリを終了して
    CoUninitialize();

    // 戻る
    return "Failed to set attribute for video capture device.";
  }

  // メディアソースを列挙する
  if (FAILED(MFEnumDeviceSources(
    pAttributes, &ppSourceActivate, &cSourceActivate)))
  {
    // メディアソースの列挙に失敗したら属性ストアを解放して
    SafeRelease(&pAttributes);

    // Media Foundation を終了して
    MFShutdown();

    // COM ライブラリを終了して
    CoUninitialize();

    // 戻る
    return "Failed to enumerate media sources.";
  }

  // すべてのメディアソースについて
  for (DWORD i = 0; i < cSourceActivate; ++i)
  {
    // メディアソースの表示名のリスト
    WCHAR* szFriendlyName{ nullptr };

    // メディアソースの表示名の数
    UINT32 cFriendlyName{ 0 };

    // メディアソースの表示名を取得する
    if (SUCCEEDED(ppSourceActivate[i]->GetAllocatedString(
      MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyName, &cFriendlyName)))
    {
      // ビデオキャプチャデバイス名をリストに追加する
      deviceList.emplace_back(TCharToUtf8(szFriendlyName));
    }

    // 表示名のリストに使ったメモリを解放する
    CoTaskMemFree(szFriendlyName);
  }

  // 属性ストアはもう使わないので解放する
  SafeRelease(&pAttributes);

  // ビデオキャプチャデバイスが見つからなかったら
  if (deviceList.empty())
  {
    // 後始末をして
    cleanup();

    // 戻る
    return "No video capture devices found.";
  }

  // 初期化が成功した
  return nullptr;
}

//
// 後始末
//
void CamMf::ComInitializer::cleanup()
{
  // メディアソースのリストを解放して
  for (DWORD i = 0; i < cSourceActivate; ++i) SafeRelease(&ppSourceActivate[i]);

  // メディアソースのリストに使ったメモリを解放する
  CoTaskMemFree(ppSourceActivate);
  ppSourceActivate = nullptr;

  // Media Foundation をシャットダウンする
  MFShutdown();

  // COM ライブラリを終了する
  CoUninitialize();
}

//
// 有効化
//
bool CamMf::ComInitializer::activate(int device, IMFMediaSource** pMediaSource)
{
  // メディアソースを作成して結果を返す
  return instance
    && device >= 0 && static_cast<UINT32>(device) < instance->cSourceActivate
    && SUCCEEDED(instance->ppSourceActivate[device]->ActivateObject(IID_PPV_ARGS(pMediaSource)))
    && pMediaSource;
}

//
// ビデオキャプチャデバイスの表示名のリストを返す
//
const std::vector<std::string>& CamMf::ComInitializer::getDeviceList()
{
  // COM ライブラリが初期化され Media Foundation が起動されていなければ
  if (!instance)
  {
    // インスタンスを生成して
    instance = new ComInitializer;

    // COM ライブラリを初期化して Media Foundation を起動する
    auto message{ instance->initialize() };

    // COM ライブラリの初期化と Media Foundation の起動に失敗したら例外を投げる
    if (message) throw std::runtime_error(message);
  }

  // ビデオキャプチャデバイスの表示名のリストを返す
  return instance->deviceList;
}

//
// 使用可能な解像度、フレームレート、コーデックのリストを作成する
//
bool CamMf::enumerateFormats()
{
  // Source Reader が作成されていなければ戻る
  if (!pSourceReader) return false;

  // 以前に作成したリストをクリアする
  availableFormats.clear();

  // ネイティブメディアタイプを一つずつ列挙する
  IMFMediaType* pMediaType{ nullptr };
  DWORD dwMediaTypeIndex{ 0 };
  while (SUCCEEDED(pSourceReader->GetNativeMediaType(
    MF_SOURCE_READER_FIRST_VIDEO_STREAM,
    dwMediaTypeIndex,
    &pMediaType
  )))
  {
    // メディアタイプがビデオなら
    GUID majorType{ 0 };
    if (SUCCEEDED(pMediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType)) && (majorType == MFMediaType_Video))
    {
      // 解像度、フレームレート、ピクセルフォーマット/コーデックを取得する
      GUID subType{ 0 };
      UINT32 width{ 0 }, height{ 0 };
      UINT32 numerator{ 0 }, denominator{ 0 };
      if (SUCCEEDED(pMediaType->GetGUID(MF_MT_SUBTYPE, &subType)) &&
        SUCCEEDED(MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height)) &&
        SUCCEEDED(MFGetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, &numerator, &denominator)))
      {
        // 使用可能なフォーマットのリストに追加する
        availableFormats.emplace_back(width, height, numerator, denominator, subType);
      }
    }

    // メディアタイプに使ったメモリを解放する
    SafeRelease(&pMediaType);

    // 次のメディアタイプに進む
    ++dwMediaTypeIndex;
  }

  // リストが空でなければ成功
  return !availableFormats.empty();
}

//
// Source Reader の出力フォーマットを設定し、基底クラスの frame を初期化する
//
bool CamMf::setFormat(int index)
{
  // 使用可能なフォーマットのリストから選択されたフォーマットを取得する
  const VideoFormat& selectedFormat{ availableFormats[index] };

  // 新しいメディアタイプを作成する
  IMFMediaType* pMediaType{ nullptr };
  HRESULT hr{ MFCreateMediaType(&pMediaType) };

  // 新しいメディアタイプが作成できたら
  if (SUCCEEDED(hr) && pMediaType)
  {
    // メジャータイプにビデオを指定する
    hr = pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);

    // サブタイプにピクセルフォーマット/コーデックを指定する
    if (SUCCEEDED(hr))
      hr = pMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);

    // 解像度を設定する
    if (SUCCEEDED(hr))
      hr = MFSetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, selectedFormat.width, selectedFormat.height);

    // フレームレートを設定する
    if (SUCCEEDED(hr))
      hr = MFSetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, selectedFormat.fps_num, selectedFormat.fps_den);

    // Source Reader の出力メディアタイプを設定する
    if (SUCCEEDED(hr))
      hr = pSourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pMediaType);
  }

  // メディアタイプはもう使わないので解放する
  SafeRelease(&pMediaType);

  // フォーマットの設定に失敗してしたら戻る
  if (FAILED(hr)) return false;

  // 基底クラスの frame メンバーと currentFormatIndex を更新する
  int type{ CV_8UC3 };  // デフォルトは RGB24

  // ピクセルフォーマットに応じて OpenCV の型を設定する
  if (selectedFormat.subType == MFVideoFormat_RGB32 || selectedFormat.subType == MFVideoFormat_ARGB32)
  {
    // アルファチャンネル付きなど 4 バイトのとき
    type = CV_8UC4;
  }
  else if (selectedFormat.subType == MFVideoFormat_YUY2)
  {
    // YUY2は2バイト/ピクセルということにしておく
    type = CV_8UC2;
  }
  else if (selectedFormat.subType == MFVideoFormat_NV12)
  {
    // NV12 は Y プレーンのみを取得してとりあえずグレースケールとして扱う
    type = CV_8UC1;
  }

  // 基底クラスの frame メンバーを初期化する
  frame.create(selectedFormat.height, selectedFormat.width, type);

  // インターバルを設定する
  interval = (selectedFormat.fps_den != 0)
    ? (1000.0 * selectedFormat.fps_den / selectedFormat.fps_num)
    : 10.0;

  // 現在選択されているフォーマットのインデックスを保存しておく
  currentFormatIndex = index;

  // フォーマットの設定に成功した
  return true;
}

//
// カメラを開く
//
bool CamMf::open(int device)
{
  // メディアソースを作成する
  if (!ComInitializer::activate(device, &pMediaSource)) return false;

  // Source Reader の属性ストアを作成する
  IMFAttributes* pAttributes{ nullptr };
  if (FAILED(MFCreateAttributes(&pAttributes, 1)))
  {
    // 属性ストアの作成に失敗したらメディアソースを解放して戻る
    SafeRelease(&pMediaSource);
    return false;
  }

  // Source Reader の属性ストアにデコード能力を設定する
  if (FAILED(pAttributes->SetUINT32(
    MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE)))
  {
    // デコード能力の設定に失敗したら属性ストアとメディアソースを解放して戻る
    SafeRelease(&pAttributes);
    SafeRelease(&pMediaSource);
    return false;
  }

  // Source Reader を作成する
  if (FAILED(MFCreateSourceReaderFromMediaSource(
    pMediaSource, pAttributes, &pSourceReader)))
  {
    // Source Reader の作成に失敗したら属性ストアとメディアソースを解放して戻る
    SafeRelease(&pAttributes);
    SafeRelease(&pMediaSource);
    return false;
  }

  // 使用可能なフォーマットを列挙する
  if (!enumerateFormats() || availableFormats.empty())
  {
    // フォーマットの列挙に失敗したら全てを開放して戻る
    SafeRelease(&pAttributes);
    SafeRelease(&pSourceReader);
    SafeRelease(&pMediaSource);
    return false;
  }

  // 列挙されたフォーマットの最初のもの (インデックス 0) をデフォルトとして設定する
  if (!setFormat(0))
  {
    // デフォルトフォーマットの設定に失敗したらリストをクリアし全てを解放して戻る
    availableFormats.clear();
    SafeRelease(&pAttributes);
    SafeRelease(&pSourceReader);
    SafeRelease(&pMediaSource);
    return false;
  }

  // 属性ストアはもう必要ないので解放する
  SafeRelease(&pAttributes);

  // ビデオキャプチャデバイスが開けたので true を返す
  return true;
}

//
// フォーマットを選択して設定する
//
bool CamMf::select(int index)
{
  // カメラが開かれていないかインデックスが範囲外なら戻る
  if (!pSourceReader || index < 0 || index >= availableFormats.size()) return false;

  // キャプチャスレッドが実行中であれば安全のために停止する
  if (running) stop();

  // 選択されたフォーマットを設定する
  return setFormat(index);
}

//
// フレームをキャプチャする
//
void CamMf::capture()
{
  // スレッドが実行可の間
  while (running)
  {
    // サンプルを読み出して
    DWORD dwStreamIndex{ 0 };
    DWORD dwStreamFlags{ 0 };
    LONGLONG llTimestamp{ 0 };
    IMFSample* pSample{ nullptr };
    if (SUCCEEDED(pSourceReader->ReadSample(
      MF_SOURCE_READER_FIRST_VIDEO_STREAM,
      0,
      &dwStreamIndex,
      &dwStreamFlags,
      &llTimestamp,
      &pSample)) && pSample)
    {
      // サンプルからメディアバッファを取得して
      IMFMediaBuffer* pBuffer{ nullptr };
      if (SUCCEEDED(pSample->ConvertToContiguousBuffer(&pBuffer)) && pBuffer)
      {
        // メディアバッファからフレームの情報を取得できたら
        BYTE* pData{ nullptr };
        DWORD cbDataLength{ 0 };
        if (SUCCEEDED(pBuffer->Lock(&pData, nullptr, &cbDataLength)) && pData)
        {
          // フレームの大きさを求める
          const auto length{ frame.cols * frame.rows * frame.channels() };

          // 転送用に必要なメモリサイズが以前と違ったらメモリを確保しなおす
          if (static_cast<int>(pixels.size()) != length) pixels.resize(length);

          // ピクセルバッファオブジェクトをロックしてから
          std::lock_guard<std::mutex> lock{ mtx };

          // データをコピーする
          std::copy(pData, pData + std::min(cbDataLength, static_cast<DWORD>(length)), pixels.data());

          // メディアバッファのロックを解除する
          pBuffer->Unlock();

          // 新しいフレームがキャプチャされたことを通知する
          captured = true;
        }

        // メディアバッファを解放する
        pBuffer->Release();
      }

      // サンプルを解放する
      pSample->Release();
    }

    // 少し休む
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(interval)));
  }
}

//
// カメラを閉じる
//
void CamMf::close()
{
  // Source Reader を解放する
  SafeRelease(&pSourceReader);

  // Media Source 解放する
  SafeRelease(&pMediaSource);

  // フォーマットリストをクリア
  availableFormats.clear();

  // フォーマットを見指定に戻す
  currentFormatIndex = -1;

  // 基底クラスの close を呼び出す
  Camera::close();
}
