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

// COM ライブラリの初期化と終了を行うオブジェクト
std::shared_ptr<CamMf::ComInitializer> CamMf::comInit{ nullptr };

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
  // 後始末
  cleanup();
}

//
// 初期化
//
const char* CamMf::ComInitializer::initialize()
{
  // エラーメッセージ
  char* message{ nullptr };

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

  // 属性ストアを解放する
  SafeRelease(&pAttributes);

  // ビデオキャプチャデバイスが見つからなかったら
  if (deviceList.empty())
  {
    // 後始末をして
    cleanup();

    // 戻る
    return "No video capture devices found.";
  }
}

//
// 後始末
//
void CamMf::ComInitializer::cleanup()
{
  // メディアソースのリストを解放して
  for (DWORD i = 0; i < cSourceActivate; ++i) SafeRelease(&ppSourceActivate[i]);

  // メディアソースのリストに使ったメモリを解放して
  CoTaskMemFree(ppSourceActivate);
  ppSourceActivate = nullptr;

  // Media Foundation をシャットダウンする
  MFShutdown();

  // COM ライブラリを終了する
  CoUninitialize();
}

//
// カメラを開く
//
bool CamMf::open(int device)
{
  // デバイス番号が不正なら false を返す
  if (device < 0 || static_cast<UINT32>(device) >= cSourceActivate) return false;

  // メディアソースを作成する
  if (FAILED(ppSourceActivate[device]->ActivateObject(IID_PPV_ARGS(&pMediaSource))))
  {
    // メディアソースの作成に失敗したら false を返す
    return false;
  }

  // Source Reader の属性ストア
  IMFAttributes* pAttributes{ nullptr };

  // Source Reader の属性ストアを作成する
  if (FAILED(MFCreateAttributes(&pAttributes, 1)))
  {
    // 属性ストアの作成に失敗したらメディアソースを解放して false を返す
    SafeRelease(&pMediaSource);
    return false;
  }

  // Source Reader の属性ストアにデコード能力を設定する
  if (FAILED(pAttributes->SetUINT32(
    MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE)))
  {
    // デコード能力の設定に失敗したら属性ストアを解放して
    SafeRelease(&pAttributes);

    // メディアソースを解放して false を返す
    SafeRelease(&pMediaSource);
    return false;
  }

  // Source Reader を作成する
  if (FAILED(MFCreateSourceReaderFromMediaSource(
    pMediaSource, pAttributes, &pSourceReader)))
  {
    // Source Reader の作成に失敗したら属性ストアを解放して
    SafeRelease(&pAttributes);

    // メディアソースを解放して false を返す
    SafeRelease(&pMediaSource);
    return false;
  }

  // 属性ストアはもう必要ないので解放する
  SafeRelease(&pAttributes);

  // ビデオキャプチャデバイスが開けたので true を返す
  return true;
}

//
// フレームをキャプチャする
//
void CamMf::capture()
{
  // キャプチャデバイスのロックを試みる
  if (mtx.try_lock())
  {
    // フレームを取得する
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
      // サンプルからメディアバッファを取得する
      IMFMediaBuffer* pBuffer{ nullptr };
      if (SUCCEEDED(pSample->ConvertToContiguousBuffer(&pBuffer)) && pBuffer)
      {
        // メディアバッファからフレームの情報を取得する
        BYTE* pData{ nullptr };
        DWORD cbDataLength{ 0 };
        if (SUCCEEDED(pBuffer->Lock(&pData, nullptr, &cbDataLength)) && pData)
        {
          // フレームの大きさを求める
          const auto length{ frame.cols * frame.rows * frame.channels() };

          // 転送用に必要なメモリサイズが以前と違ったらメモリを確保しなおす
          if (static_cast<int>(pixels.size()) != length) pixels.resize(length);

          // データをコピーする
          std::copy(pData, pData + std::min(cbDataLength, static_cast<DWORD>(length)), pixels.data());

          // メディアバッファのロックを解除する
          pBuffer->Unlock();

          // 新しいフレームが取得されたことを記録しておく
          captured = true;
        }

        // メディアバッファを解放する
        pBuffer->Release();
      }

      // サンプルを解放する
      pSample->Release();
    }

    // キャプチャデバイスのロックを解除する
    mtx.unlock();
  }

  // キャプチャスレッドが実行中なら少し休む
  if (running) std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(interval)));
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
}
