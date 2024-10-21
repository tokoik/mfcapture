#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <mfobjects.h>
#include <mferror.h>
#include <dshow.h>
#include <iostream>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfplay.lib")
#pragma comment(lib, "shlwapi.lib")

class WebCamCapture {
public:
  WebCamCapture() : pSourceReader(nullptr), pMediaSource(nullptr), hwndVideo(nullptr) {}

  ~WebCamCapture() {
    if (pSourceReader) {
      pSourceReader->Release();
    }
    if (pMediaSource) {
      pMediaSource->Release();
    }
    MFShutdown();
  }

  bool Initialize(HWND hwnd) {
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
      std::cerr << "MFStartup failed\n";
      return false;
    }

    IMFAttributes* pAttributes = nullptr;
    hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr)) {
      std::cerr << "MFCreateAttributes failed\n";
      return false;
    }

    hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr)) {
      std::cerr << "SetGUID failed\n";
      pAttributes->Release();
      return false;
    }

    IMFActivate** ppDevices = nullptr;
    UINT32 count = 0;
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    if (FAILED(hr) || count == 0) {
      std::cerr << "No webcam found\n";
      pAttributes->Release();
      return false;
    }

    hr = ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pMediaSource));
    for (UINT32 i = 0; i < count; i++) {
      ppDevices[i]->Release();
    }
    CoTaskMemFree(ppDevices);
    if (FAILED(hr)) {
      std::cerr << "ActivateObject failed\n";
      pAttributes->Release();
      return false;
    }

    hr = MFCreateSourceReaderFromMediaSource(pMediaSource, pAttributes, &pSourceReader);
    pAttributes->Release();
    if (FAILED(hr)) {
      std::cerr << "MFCreateSourceReaderFromMediaSource failed\n";
      return false;
    }

    hr = pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    if (FAILED(hr)) {
      std::cerr << "Failed to set video processing attribute\n";
      return false;
    }

    hwndVideo = hwnd;
    return true;
  }

  void CaptureFrame() {
    IMFSample* pSample = nullptr;
    DWORD streamIndex, flags;
    LONGLONG llTimestamp;

    HRESULT hr = pSourceReader->ReadSample(
      (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
      0, &streamIndex, &flags, &llTimestamp, &pSample);

    if (SUCCEEDED(hr)) {
      if (pSample) {
        IMFMediaBuffer* pBuffer = nullptr;
        hr = pSample->ConvertToContiguousBuffer(&pBuffer);
        if (SUCCEEDED(hr)) {
          BYTE* pData = nullptr;
          DWORD maxLength = 0, currentLength = 0;
          hr = pBuffer->Lock(&pData, &maxLength, &currentLength);
          if (SUCCEEDED(hr)) {
            // Process frame data (pData)
            pBuffer->Unlock();
          }
          pBuffer->Release();
        }
        pSample->Release();
      }
    }
  }

private:
  IMFSourceReader* pSourceReader;
  IMFMediaSource* pMediaSource;
  HWND hwndVideo;
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
