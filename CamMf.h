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

//
// ビデオフォーマットの詳細を保持する構造体
//
struct VideoFormat
{
  UINT32 width;           // 幅
  UINT32 height;          // 高さ
  UINT32 fpsNum;         // フレームレートの分子 (Numerator)
  UINT32 fpsDenom;         // フレームレートの分母 (Denominator)
  GUID subType;           // ピクセルフォーマット/コーデックの GUID
  std::string formatName; // 人間が読める形式の文字列

  //
  // コンストラクタ
  //
  VideoFormat(UINT32 width, UINT32 height, UINT32 fps_num, UINT32 fps_den, GUID subType);
};

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
    static ComInitializer instance;

    // ビデオキャプチャデバイスの表示名のリスト
    std::vector<std::string> deviceList;

    // メディアソースのリスト
    IMFActivate** ppSourceActivate;

    // メディアソースの数
    UINT32 cSourceActivate;

    // COM ライブラリが初期化されていれば true
    bool coInitialized;

    // Media Foundation が起動されていれば true
    bool mfStarted;

    //
    // コンストラクタ
    //
    ComInitializer();

    //
    // デストラクタ
    //
    ~ComInitializer();

    //
    // 初期化
    //
    const char* initialize();

  public:

    // シングルトンなのでコピーは作らせない
    ComInitializer(const ComInitializer& com) = delete;
    ComInitializer(ComInitializer&& com) = delete;
    ComInitializer& operator=(const ComInitializer& com) = delete;
    ComInitializer& operator=(ComInitializer&&) = delete;

    //
    // 有効化
    //
    static bool activate(int device, IMFMediaSource** pMediaSource);

    //
    // ビデオキャプチャデバイスの表示名のリストを返す
    //
    static const std::vector<std::string>& getDeviceList();
  };

  // メディアソースの読み取り
  IMFSourceReader* pSourceReader;

  // メディアソース
  IMFMediaSource* pMediaSource;

  // 使用可能なビデオフォーマットのリスト
  std::vector<VideoFormat> availableFormats;

  // 現在選択されているフォーマットのインデックス
  int currentFormatIndex;

  //
  // 使用可能な解像度、フレームレート、コーデックのリストを作成する
  //
  bool enumerateFormats();

  //
  // Source Reader の出力フォーマットを設定し、基底クラスの frame を初期化する
  //
  bool setFormat(int index);

public:

  ///
  /// コンストラクタ
  ///
  CamMf()
    : pSourceReader{ nullptr }
    , pMediaSource{ nullptr }
    , currentFormatIndex{ -1 }
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
  static const std::vector<std::string>& getDeviceList()
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
  /// 使用可能なビデオフォーマットのリストを返す
  ///
  const std::vector<VideoFormat>& getAvailableFormats() const
  {
    return availableFormats;
  }

  ///
  /// フォーマットを選択して設定する
  ///
  /// @param index 選択するフォーマットのリストインデックス
  /// @return 成功したら true
  ///
  bool select(int index);

  ///
  /// フレームをキャプチャする
  ///
  void capture();

  ///
  /// カメラを閉じる
  ///
  void close();
};
