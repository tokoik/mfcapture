// 補助プログラム
#include "gg.h"

// 標準ライブラリ
#include <iostream>

// Microsoft Media Foundation
#include <MFapi.h>
#include <MFidl.h>
#include <MFreadwrite.h>
#pragma comment(lib, "MF.lib")
#pragma comment(lib, "MFplat.lib")
#pragma comment(lib, "MFreadwrite.lib")

//
// Media Foundation のビデオデバイスの一覧を作る
//
//   https://docs.microsoft.com/ja-jp/windows/win32/medfound/audio-video-capture-in-media-foundation
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
    , pMediaSource{nullptr}
    , hwndVideo{ nullptr }
  {}

  ///
  /// デストラクタ
  ///
  ~WebCamCapture()
  {
    // Source Reader を解放する
    if (pSourceReader) pSourceReader->Release();

    // Media Source 解放する
    if (pMediaSource) pMediaSource->Release();

    // Media Foundation をシャットダウンする
    MFShutdown();
  }

  ///
  /// Media Foundation の初期化
  /// 
  /// @param hwnd ウィンドウのハンドル
  /// @return 初期化に成功したら true
  /// 
  bool Initialize(HWND hwnd)
  {
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
      std::cerr << "MFStartup failed\n";
      return false;
    }

    IMFAttributes* pAttributes{ nullptr };
    hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr))
    {
      std::cerr << "MFCreateAttributes failed\n";
      return false;
    }

    hr = pAttributes->SetGUID(
      MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
      MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
    -);
    if (FAILED(hr))
    {
      std::cerr << "SetGUID failed\n";
      pAttributes->Release();
      return false;
    }

    IMFActivate** ppDevices{ nullptr };
    UINT32 count{ 0 };
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    if (FAILED(hr) || count == 0)
    {
      std::cerr << "No webcam found\n";
      pAttributes->Release();
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

#if 1
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
