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
// Media Foundation のビデオデバイスの一覧を作る
//
void getMediaFoundationList(std::vector<std::string>& list)
{
  // Create an attribute store to hold the search criteria.
  IMFAttributes* pConfig{ NULL };
  HRESULT hr{ MFCreateAttributes(&pConfig, 1) };

  // Request video capture devices.
  if (SUCCEEDED(hr))
  {
    hr = pConfig->SetGUID(
      MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
      MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
    );
  }

  // Enumerate the devices,
  IMFActivate** ppDevices{ NULL };
  UINT32 count{ 0 };
  if (SUCCEEDED(hr))
  {
    hr = MFEnumDeviceSources(pConfig, &ppDevices, &count);
  }

  for (DWORD i = 0; i < count; i++)
  {
    // Try to get the display name.
    WCHAR* szFriendlyName{ NULL };
    UINT32 cchName{ 0 };
    HRESULT hr{
      ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
      &szFriendlyName, &cchName)
    };

    if (SUCCEEDED(hr))
    {
      list.emplace_back(TCharToUtf8(szFriendlyName));
    }
    CoTaskMemFree(szFriendlyName);
  }

  for (DWORD i = 0; i < count; i++)
  {
    ppDevices[i]->Release();
  }
  CoTaskMemFree(ppDevices);
}

//
// 初期化
//
bool CamMf::init()
{
  // Media Foundation を初期化する
  if (SUCCEEDED(MFStartup(MF_VERSION)))
  {
    // 検索条件を保持する属性ストア
    IMFAttributes* pAttributes{ NULL };

    // 検索条件を保持する属性ストアを作成する
    if (SUCCEEDED(MFCreateAttributes(&pAttributes, 1)))
    {
      // ビデオキャプチャデバイスを要求する
      if (SUCCEEDED(pAttributes->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)))
      {
        // メディアソースを列挙する
        if (SUCCEEDED(MFEnumDeviceSources(
          pAttributes, &ppSourceActivate, &cSourceActivate)))
        {
          // すべてのメディアソースについて
          for (DWORD i = 0; i < cSourceActivate; ++i)
          {
            // メディアソースの表示名のリスト
            WCHAR* szFriendlyName{ NULL };

            // メディアソースの表示名の数
            UINT32 cFriendlyName{ 0 };

            // メディアソースの表示名を取得する
            if (SUCCEEDED(ppSourceActivate[i]->GetAllocatedString(
              MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
              &szFriendlyName, &cFriendlyName)))
            {
              // ビデオキャプチャデバイス名をリストに追加する
              deviceList.emplace_back(TCharToUtf8(szFriendlyName));
            }

            // 表示名のリストに使ったメモリを解放する
            CoTaskMemFree(szFriendlyName);
          }
        }
      }
    }
  }
}


  HRESULT hr{ ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pMediaSource)) };
  for (UINT32 i = 0; i < count; i++)
  {
    ppDevices[i]->Release();
  }
  CoTaskMemFree(ppDevices);
  if (FAILED(hr))
  {
    std::cerr << "ActivateObject failed\n";
    pAttributes->Release();
    return false;
  }
  hr = MFCreateSourceReaderFromMediaSource(pMediaSource, pAttributes, &pSourceReader);
  pAttributes->Release();
  if (FAILED(hr))
  {
    std::cerr << "MFCreateSourceReaderFromMediaSource failed\n";
    return false;
  }
  hr = pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
  if (FAILED(hr))
  {
    std::cerr << "Failed to set video processing attribute\n";
    return false;
  }
  hwndVideo = hwnd;
  return true;
}  //

//
// 後始末
//
void CamMf::cleanup()
{
  // メディアソースのリストに使ったメモリを解放する

  // すべての表示名について
  for (DWORD i = 0; i < cFriendlyName; ++i)
  {
    // 表示名に使ったメモリを解放する
    ppSourceActivate[i]->Release();
  }
  // キャプチャデバイスのリストに使ったメモリを解放する
  CoTaskMemFree(ppSourceActivate);

  // Media Foundation をシャットダウンする
  MFShutdown();
}

//
// 初期化
//
bool CamMf::open(int device)
{
  // 結果
  HRESULT hr;

  // Media Foundation の初期化
  hr = MFStartup(MF_VERSION);
  if (FAILED(hr))
  {
    std::cerr << "MFStartup failed\n";
    return false;
  }

  hr = ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pMediaSource));
  for (UINT32 i = 0; i < count; i++)
  {
    ppDevices[i]->Release();
  }
  CoTaskMemFree(ppDevices);
  if (FAILED(hr))
  {
    std::cerr << "ActivateObject failed\n";
    pAttributes->Release();
    return false;
  }
  hr = MFCreateSourceReaderFromMediaSource(pMediaSource, pAttributes, &pSourceReader);
  pAttributes->Release();
  if (FAILED(hr))
  {
    std::cerr << "MFCreateSourceReaderFromMediaSource failed\n";
    return false;
  }
  hr = pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
  if (FAILED(hr))
  {
    std::cerr << "Failed to set video processing attribute\n";
    return false;
  }
  hwndVideo = hwnd;
  return true;
}  //

///
/// Microsoft Media Foundation による Web カメラ
///
class WebCamCapture
{
  IMFSourceReader* pSourceReader;
  IMFMediaSource* pMediaSource;
  HWND hwndVideo;

public:

  ///
  /// コンストラクタ
  ///
  WebCamCapture()
    : pSourceReader{ nullptr }
    , pMediaSource{ nullptr }
    , hwndVideo{ nullptr }
  {
  }

  ///
  /// デストラクタ
  ///
  ~WebCamCapture()
  {
  }

  ///
  /// Media Foundation の初期化
  /// 
  /// @param hwnd ウィンドウのハンドル
  /// @return 初期化に成功したら true
  /// 
  bool Initialize(HWND hwnd)
  {
  }

  void CaptureFrame()
  {
    IMFSample* pSample{ nullptr };
    DWORD streamIndex, flags;
    LONGLONG llTimestamp;

    HRESULT hr = pSourceReader->ReadSample(
      (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
      0, &streamIndex, &flags, &llTimestamp, &pSample);

    if (SUCCEEDED(hr))
    {
      if (pSample)
      {
        IMFMediaBuffer* pBuffer{ nullptr };
        hr = pSample->ConvertToContiguousBuffer(&pBuffer);
        if (SUCCEEDED(hr))
        {
          BYTE* pData{ nullptr };
          DWORD maxLength = 0, currentLength = 0;
          hr = pBuffer->Lock(&pData, &maxLength, &currentLength);
          if (SUCCEEDED(hr))
          {
            // Process frame data (pData)
            std::cout << "max = " << maxLength << ", current = " << currentLength << '\n';
            pBuffer->Unlock();
          }
          pBuffer->Release();
        }
        pSample->Release();
      }
    }
  }
};

#if 0
int main() {
  WebCamCapture capture;
  HWND hwnd = GetConsoleWindow();
  if (capture.Initialize(hwnd)) {
    while (true) {
      capture.CaptureFrame();
      Sleep(30); // 30 ms delay for approx 30 FPS
    }
  }
  else {
    std::cerr << "Failed to initialize webcam capture\n";
  }
  return 0;
}
#endif
