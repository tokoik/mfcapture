#pragma once

///
/// Microsoft Media Foundation を使ったビデオキャプチャクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date December 24, 2024
///

// カメラ関連の処理
#include "Camera.h"

// Microsoft Media Foundation
#include <MFapi.h>
#include <MFidl.h>
#include <MFreadwrite.h>

///
/// Microsoft Media Foundation を使ってビデオをキャプチャするクラス
///
class CamMf : public Camera
{
  ///
  /// COM ライブラリの初期化と終了を行うクラス
  ///
  class ComInitializer
  {
    ///
    /// コンストラクタ
    ///
    ComInitializer()
    {
      // COM ライブラリを初期化する
      HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
      if (hr)
      {
        // COM ライブラリの初期化に失敗したら例外を投げる
        throw std::runtime_error("Failed to initialize COM library.");
      }

      // Media Foundation を起動する
      hr = MFStartup(MF_VERSION);
      if (FAILED(hr)) goto error;

      // 検索条件を保持する属性ストア
      IMFAttributes* pAttributes{ NULL };

      // 検索条件を保持する属性ストアを作成する
      hr = MFCreateAttributes(&pAttributes, 1);
      if (FAILED(hr)) goto error;

      // ビデオキャプチャデバイスを要求する
      hr = pAttributes->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
      if (FAILED(hr)) goto error;

      // メディアソースを列挙する
      hr = MFEnumDeviceSources(
        pAttributes, &ppSourceActivate, &cSourceActivate);
      if (FAILED(hr)) goto error;

      // すべてのメディアソースについて
      for (DWORD i = 0; i < cSourceActivate; ++i)
      {
        // メディアソースの表示名のリスト
        WCHAR* szFriendlyName{ NULL };

        // メディアソースの表示名の数
        UINT32 cFriendlyName{ 0 };

        // メディアソースの表示名を取得する
        hr = ppSourceActivate[i]->GetAllocatedString(
          MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyName, &cFriendlyName);
        if (FAILED(hr)) goto error;

        // ビデオキャプチャデバイス名をリストに追加する
        deviceList.emplace_back(TCharToUtf8(szFriendlyName));

        // 表示名のリストに使ったメモリを解放する
        CoTaskMemFree(szFriendlyName);
      }

    done:
      SafeRelease(&pAttributes);

      for (DWORD i = 0; i < count; i++)
      {
        SafeRelease(&ppDevices[i]);
      }
      CoTaskMemFree(ppDevices);
      SafeRelease(&pSource);
    error:

      // COM ライブラリを終了する
      CoUninitialize();
    }

    ///
    /// コピーコンストラクタは使用しない
    ///
    /// @param com コピー元
    ///
    ComInitializer(const ComInitializer& com) = delete;

    ///
    /// ムーブコンストラクタ
    ///
    /// @param com ムーブ元
    ///
    ComInitializer(ComInitializer&& com) = default;

    ///
    /// デストラクタ
    ///
    ~ComInitializer()
    {
      // Media Foundation をシャットダウンする
      MFShutdown();

      // COM ライブラリを終了する
      CoUninitialize();
    }

    ///
    /// 代入演算子は使用しない
    ///
    /// @param com 代入元のオブジェクト
    /// @return 代入後のこのオブジェクトの参照
    ///
    ComInitializer& operator=(const ComInitializer& com) = delete;

    ///
    /// ムーブ代入演算子
    ///
    /// @param com ムーブ代入元のオブジェクト
    /// @return ムーブ代入後のこのオブジェクトの参照
    ///
    ComInitializer& operator=(ComInitializer&&) = default;
  };

  // COM ライブラリの初期化と終了を行うオブジェクト
  static std::shared_ptr<ComInitializer> comInit;

  // ビデオキャプチャデバイスの表示名のリスト
  static std::vector<std::string> deviceList;

  // メディアソースのリスト
  static IMFActivate** ppSourceActivate;

  // メディアソースの数
  static UINT32 cSourceActivate;

  IMFSourceReader* pSourceReader;
  IMFMediaSource* pMediaSource;
  HWND hwndVideo;

///
/// COM ライブラリの初期化と終了を行うクラス
///
class ComInitializer
{
public:

  ///
  /// コンストラクタ
  ///
  ComInitializer()
  {
    // COM ライブラリを初期化する
    if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
    {
      // COM ライブラリの初期化に失敗したら例外を投げる
      throw std::runtime_error("Failed to initialize COM library.");
    }

    // Media Foundation を起動する
    if (FAILED(MFStartup(MF_VERSION)))
    {
      // Media Foundation の起動に失敗したら COM ライブラリを終了して
      CoUninitialize();

      // 例外を投げる
      throw std::runtime_error("Failed to start Media Foundation.");
    }

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

  ///
  /// コピーコンストラクタは使用しない
  ///
  /// @param com コピー元
  ///
  ComInitializer(const ComInitializer& com) = delete;

  ///
  /// ムーブコンストラクタ
  ///
  /// @param com ムーブ元
  ///
  ComInitializer(ComInitializer&& com) = default;

  ///
  /// デストラクタ
  ///
  ~ComInitializer()
  {
    // Media Foundation をシャットダウンする
    MFShutdown();

    // COM ライブラリを終了する
    CoUninitialize();
  }

  ///
  /// 代入演算子は使用しない
  ///
  /// @param com 代入元のオブジェクト
  /// @return 代入後のこのオブジェクトの参照
  ///
  ComInitializer& operator=(const ComInitializer& com) = delete;

  ///
  /// ムーブ代入演算子
  ///
  /// @param com ムーブ代入元のオブジェクト
  /// @return ムーブ代入後のこのオブジェクトの参照
  ///
  ComInitializer& operator=(ComInitializer&&) = default;
};

public:

  ///
  /// コンストラクタ
  ///
  CamMf()
    : pSourceReader{ nullptr }
    , pMediaSource{ nullptr }
    , hwndVideo{ nullptr }
  {
    // COM ライブラリが初期化されていなければ初期化する
    if (!comInit) comInit = std::make_shared<ComInitializer>();
  }

  ///
  /// デストラクタ
  ///
  virtual ~CamMf()
  {
    // Source Reader を解放する
    if (pSourceReader) pSourceReader->Release();

    // Media Source 解放する
    if (pMediaSource) pMediaSource->Release();
  }

  ///
  /// 初期化
  ///
  static bool init();

  ///
  /// 後始末
  ///
  static void cleanup();

  ///
  /// カメラを開く
  ///
  /// @param device デバイスの番号
  ///
  bool open(int device);

  ///
  /// Media Foundation のビデオデバイスの一覧を作る
  ///
  /// @return デバイス名のリスト
  ///
  const std::vector<std::string>& getDeviceList()
  {
    return deviceList;
  }
};
