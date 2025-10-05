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
    // COM ライブラリの初期化と終了を行うオブジェクト
    static ComInitializer* instance;

    // ビデオキャプチャデバイスの表示名のリスト
    std::vector<std::string> deviceList;

    // メディアソースのリスト
    IMFActivate** ppSourceActivate;

    // メディアソースの数
    UINT32 cSourceActivate;

    //
    // コンストラクタ
    //
    ComInitializer();

    //
    // デストラクタ
    //
    ~ComInitializer();

  public:

    // シングルトンなのでコピーは作らせない
    ComInitializer(const ComInitializer& com) = delete;
    ComInitializer(ComInitializer&& com) = delete;
    ComInitializer& operator=(const ComInitializer& com) = delete;
    ComInitializer& operator=(ComInitializer&&) = delete;

    //
    // 初期化
    //
    const char* initialize();  

    //
    // 有効化
    //
    static bool activate(int device, IMFMediaSource** pMediaSource);

    //
    // 後始末
    //
    void cleanup();

    //
    // ビデオキャプチャデバイスの表示名のリストを返す
    //
    static const std::vector<std::string>& getDeviceList();
  };

  // メディアソースの読み取り
  IMFSourceReader* pSourceReader;

  // メディアソース
  IMFMediaSource* pMediaSource;

public:

  ///
  /// コンストラクタ
  ///
  CamMf()
    : pSourceReader{ nullptr }
    , pMediaSource{ nullptr }
  {
  }

  ///
  /// デストラクタ
  ///
  virtual ~CamMf()
  {
    // カメラを閉じる
    close();
  }

  ///
  /// Media Foundation のビデオデバイスの一覧を作る
  ///
  /// @return デバイス名のリスト
  ///
  const std::vector<std::string>& getDeviceList()
  {
    return ComInitializer::getDeviceList();
  }

  ///
  /// カメラを開く
  ///
  /// @param device デバイスの番号
  ///
  bool open(int device);

  ///
  /// フレームをキャプチャする
  ///
  void capture();

  ///
  /// カメラを閉じる
  ///
  void close();
};
