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
#pragma comment(lib, "MFuuid.lib")
#pragma comment(lib, "MFreadwrite.lib")

// COM ライブラリの初期化と終了を行うオブジェクト
CamMf::ComInitializer CamMf::ComInitializer::instance;

// H.264 MFT デコーダーの CLSID (Windows標準デコーダー)
const CLSID CLSID_CMSH264DecoderMFT
{
  0x62CE7C78,
  0xCEE9,
  0x48CC,
  {
    0xA0,
    0x63,
    0x6F,
    0x1E,
    0x07,
    0x9A,
    0xAA,
    0x08
  }
};

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

  return "";
}

//
// COM ライブラリの初期化と終了を行うクラスのコンストラクタ
//
CamMf::ComInitializer::ComInitializer()
  : deviceList{}
  , ppSourceActivate{ nullptr }
  , cSourceActivate{ 0 }
  , coInitialized{ false }
  , mfStarted{ false }
{
}

//
// COM ライブラリの初期化と終了を行うクラスのデストラクタ
//
CamMf::ComInitializer::~ComInitializer()
{
  // メディアソースのリストを取得していれば
  if (ppSourceActivate)
  {
    // すべてのメディアソースを解放して
    for (DWORD i = 0; i < cSourceActivate; ++i) SafeRelease(&ppSourceActivate[i]);

    // メディアソースのリストに使ったメモリを解放して
    CoTaskMemFree(ppSourceActivate);
    ppSourceActivate = nullptr;

    // ビデオキャプチャデバイスの表示名のリストを空にする
    deviceList.clear();
  }

  // Media Foundation が起動していればシャットダウンする
  if (mfStarted) MFShutdown();

  // COM ライブラリが初期化されていれば終了する
  if (coInitialized) CoUninitialize();
}

//
// 初期化
//
const char* CamMf::ComInitializer::initialize()
{
  // エラーメッセージ
  const char* message{ nullptr };

  // 検索条件を保持する属性ストア
  IMFAttributes* pAttributes{ nullptr };

  // COM ライブラリを初期化する
  if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
  {
    // COM ライブラリの初期化に失敗した
    message = "Failed to initialize COM library.";
    goto done;
  }

  // COM ライブラリの初期化に成功した
  coInitialized = true;

  // Media Foundation を起動する
  if (FAILED(MFStartup(MF_VERSION)))
  {
    // Media Foundation の起動に失敗した
    message = "Failed to start Media Foundation.";
    goto done;
  }

  // Media Foundation の起動に成功した
  mfStarted = true;

  // 検索条件を保持する属性ストアを作成する
  if (FAILED(MFCreateAttributes(&pAttributes, 1)))
  {
    // 属性ストアの作成失敗した
    message = "Failed to create attribute store.";
    goto done;
  }

  // 属性ストアにビデオキャプチャデバイスの属性を設定する
  if (FAILED(pAttributes->SetGUID(
    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)))
  {
    // ビデオキャプチャデバイスの属性の設定に失敗した
    message = "Failed to set attribute for video capture device.";
    goto done;
  }

  // メディアソースを列挙する
  if (FAILED(MFEnumDeviceSources(
    pAttributes, &ppSourceActivate, &cSourceActivate)))
  {
    // メディアソースの列挙に失敗した
    message = "Failed to enumerate media sources.";
    goto done;
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
      // ビデオキャプチャデバイス名を作る
      std::stringstream ss;
      ss << TCharToUtf8(szFriendlyName) << "##" << i;

      // ビデオキャプチャデバイス名をリストに追加する
      deviceList.emplace_back(ss.str());
    }

    // 表示名のリストに使ったメモリを解放する
    CoTaskMemFree(szFriendlyName);
  }

done:

  // 属性ストアはもう使わないので解放する
  SafeRelease(&pAttributes);

  // エラーメッセージを返す
  return message;
}

//
// COM ライブラリのシングルトンインスタンスを返す
//
const CamMf::ComInitializer& CamMf::ComInitializer::getInstance()
{
  // COM ライブラリが初期化され Media Foundation が起動されていなければ
  if (!instance.ppSourceActivate)
  {
    // COM ライブラリを初期化して Media Foundation を起動する
    auto message{ instance.initialize() };

    // COM ライブラリの初期化と Media Foundation の起動に失敗したら例外を投げる
    if (message) throw std::runtime_error(message);
  }

  // COM ライブラリのシングルトンインスタンスへの参照を返す
  return instance;
}

//
// 有効化
//
bool CamMf::ComInitializer::activate(int device, IMFMediaSource** pMediaSource)
{
  // メディアソースを作成して結果を返す
  return device >= 0 && static_cast<UINT32>(device) < instance.cSourceActivate
    && SUCCEEDED(instance.ppSourceActivate[device]->ActivateObject(IID_PPV_ARGS(pMediaSource)))
    && pMediaSource;
}

//
// ビデオキャプチャデバイスの表示名のリストを返す
//
const std::vector<std::string>& CamMf::ComInitializer::getDeviceList()
{
  // ビデオキャプチャデバイスの表示名のリストを返す
  return getInstance().deviceList;
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
  formatList.clear();

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
        // コーデックの文字列を取り出す
        const auto& codecName{ SubTypeToName(subType) };

        // コーデックか対応可能でフレームレートに問題が無ければ
        if (codecName.size() > 0 && denominator > 0 && numerator > 0)
        {
          // フレームレートを求める
          const double fps{ static_cast<double>(numerator) / static_cast<double>(denominator) };

          // フレームレートが 5 以上なら
          if (fps >= 5.0)
          {
            // 使用可能なビデオフォーマットの表示名を作成する
            std::stringstream ss;
            ss << width << " x " << height << " @ "
              << std::fixed << std::setprecision(2) << fps
              << " fps (" << codecName << ")##" << dwMediaTypeIndex;

            // 使用可能なビデオフォーマットの表示名をリストに追加する
            formatList.emplace_back(ss.str());

            // 使用可能なビデオフォーマットのリストに追加する
            availableFormats.emplace_back(width, height, numerator, denominator, subType);
          }
        }
      }
    }

    // メディアタイプに使ったメモリを解放する
    SafeRelease(&pMediaType);

    // 次のメディアタイプに進む
    ++dwMediaTypeIndex;
  }

  // リストが空でなければ成功
  return !formatList.empty();
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
  if (FAILED(hr)) goto done;

  // メジャータイプにビデオを指定する
  hr = pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  if (FAILED(hr)) goto done;

  // サブタイプにピクセルフォーマット/コーデックを指定する
  hr = pMediaType->SetGUID(MF_MT_SUBTYPE, selectedFormat.subType);
  if (FAILED(hr)) goto done;

  // 解像度を設定する
  hr = MFSetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, selectedFormat.width, selectedFormat.height);
  if (FAILED(hr)) goto done;

  // フレームレートを設定する
  hr = MFSetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, selectedFormat.fpsNum, selectedFormat.fpsDenom);
  if (FAILED(hr)) goto done;

  // Source Reader の出力メディアタイプを設定する
  hr = pSourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pMediaType);
  if (FAILED(hr)) goto done;

  // 基底クラスの frame メンバーを初期化する（サイズとタイプを記録するためにヘッダだけ作る）
  frame.create(selectedFormat.height, selectedFormat.width, CV_8UC3);

  // インターバルを設定する
  interval = (selectedFormat.fpsDenom != 0)
    ? (1000.0 * selectedFormat.fpsDenom / selectedFormat.fpsNum)
    : 10.0;

  // 選択したフォーマットのコーデックが H.264 のとき MFT デコーダーパイプラインをセットアップする
  if (selectedFormat.subType == MFVideoFormat_H264 && !setupDecoderPipeline(selectedFormat))
  {
    // MFT セットアップ失敗時のクリーンアップ
    close();
    return false;
  }

done:

  // メディアタイプはもう使わないので解放する
  SafeRelease(&pMediaType);

  // フォーマットの設定に成功していたら true を返す
  return SUCCEEDED(hr);
}

//
// MFTデコーダーのセットアップと接続を行う
//
bool CamMf::setupDecoderPipeline(const VideoFormat& nativeH264Format)
{
  // 結果
  HRESULT hr{ S_OK };

  // H.264 デコーダー MFT をインスタンス化する
  hr = CoCreateInstance(CLSID_CMSH264DecoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDecoderMFT));
  if (FAILED(hr)) return false;

  // MFT 入力タイプの設定 (Source Readerのネイティブ H.264 ストリームの形式)
  IMFMediaType* pInputType{ nullptr };
  hr = MFCreateMediaType(&pInputType);
  if (SUCCEEDED(hr)) hr = pInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  if (SUCCEEDED(hr)) hr = pInputType->SetGUID(MF_MT_SUBTYPE, nativeH264Format.subType); // MFVideoFormat_H264
  if (SUCCEEDED(hr)) hr = MFSetAttributeSize(pInputType, MF_MT_FRAME_SIZE, nativeH264Format.width, nativeH264Format.height);
  if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(pInputType, MF_MT_FRAME_RATE, nativeH264Format.fpsNum, nativeH264Format.fpsDenom);

  // MFTに入力タイプを設定する
  if (SUCCEEDED(hr)) hr = pDecoderMFT->SetInputType(0, pInputType, 0);
  SafeRelease(&pInputType);
  if (FAILED(hr)) return false;

  // MFT 出力タイプの設定 (デコード後の形式: RGB32 を要求)
  hr = MFCreateMediaType(&pOutputMediaType);
  if (SUCCEEDED(hr)) hr = pOutputMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  if (SUCCEEDED(hr)) hr = pOutputMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32); // RGB32を要求
  if (SUCCEEDED(hr)) hr = MFSetAttributeSize(pOutputMediaType, MF_MT_FRAME_SIZE, nativeH264Format.width, nativeH264Format.height);
  if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(pOutputMediaType, MF_MT_FRAME_RATE, nativeH264Format.fpsNum, nativeH264Format.fpsDenom);

  // MFT に出力タイプを設定する
  if (SUCCEEDED(hr)) hr = pDecoderMFT->SetOutputType(0, pOutputMediaType, 0);
  if (FAILED(hr))
  {
    SafeRelease(&pOutputMediaType); // エラー時は pOutputMediaType も解放
    return false;
  }

  // MFT をアクティブにする
  hr = pDecoderMFT->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, NULL);
  if (SUCCEEDED(hr)) hr = pDecoderMFT->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, NULL);
  if (SUCCEEDED(hr)) hr = pDecoderMFT->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, NULL);

  // 5. frame メンバーをデコード後の RGB32 サイズで初期化
  frame.create(nativeH264Format.height, nativeH264Format.width, CV_8UC4);

  return SUCCEEDED(hr);
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
  HRESULT hr{ MFCreateAttributes(&pAttributes, 1) };
  if (FAILED(hr)) goto done;

  // Source Reader の属性ストアにデコード能力を設定する
  hr = pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
  if (FAILED(hr)) goto done;

  // Source Reader を作成する
  SafeRelease(&pSourceReader);
  hr = MFCreateSourceReaderFromMediaSource(pMediaSource, pAttributes, &pSourceReader);
  if (FAILED(hr)) goto done;

  // 属性ストアはもう必要ないので解放する
  SafeRelease(&pAttributes);

  // 使用可能なフォーマットを列挙して最初のフォーマットを選択する
  if (enumerateFormats() && setFormat(0)) return true;

done:

  // フォーマットの設定に失敗したら全てを解放して戻る
  SafeRelease(&pSourceReader);
  SafeRelease(&pAttributes);
  availableFormats.clear();
  formatList.clear();

  // ビデオキャプチャデバイスが開けたので true を返す
  return false;
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
  // MFT デコーダーを解放する
  SafeRelease(&pDecoderMFT);

  // 出力メディアタイプを解放する
  SafeRelease(&pOutputMediaType);

  // Source Reader を解放する
  SafeRelease(&pSourceReader);

  // Media Source 解放する
  SafeRelease(&pMediaSource);

  // フォーマットリストをクリア
  availableFormats.clear();
  formatList.clear();

  // 基底クラスの close を呼び出す
  Camera::close();
}
