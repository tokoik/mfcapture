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
    ComInitializer();

    ///
    /// デストラクタ
    ///
    ~ComInitializer();

  public:

    ///
    /// シングルトンなのでコピーは作らせない
    ///
    ComInitializer(const ComInitializer& com) = delete;
    ComInitializer(ComInitializer&& com) = delete;
    ComInitializer& operator=(const ComInitializer& com) = delete;
    ComInitializer& operator=(ComInitializer&&) = delete;

    ///
    /// 初期化
    ///
    static const char* initialize();  

    ///
    /// 後始末
    ///
    static void cleanup();
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

public:

  ///
  /// コンストラクタ
  ///
  CamMf()
    : pSourceReader{ nullptr }
    , pMediaSource{ nullptr }
  {
    // 初期化
    initialize();
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
  /// 初期化
  ///
  static void initialize()
  {
    // COM ライブラリが初期化されていなければ初期化する
    if (!comInit) comInit = std::make_shared<ComInitializer>();
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
