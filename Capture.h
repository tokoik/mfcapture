#pragma once

///
/// キャプチャクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date Aplil 3, 2023
///

// バッファクラス
#include "Buffer.h"

// OpenCV による画像ファイルの入力
#include "CamImage.h"

// OpenCV による動画の入力
#include "CamCv.h"

// Microsoft Media Foundation による動画の入力
#include "CamMf.h"

///
/// キャプチャクラス
///
class Capture
{
  /// 選択しているキャプチャデバイスのポインタ
  std::unique_ptr<Camera> camera;

  /// 使用可能なビデオフォーマットの表示名のリスト
  const std::vector<std::string>* formatList;

public:

  ///
  /// キャプチャーオブジェクトのデフォルトコンストラクタ
  ///
  Capture()
    : camera{ nullptr }
    , formatList{ nullptr }
  {
  }

  ///
  /// デストラクタ
  ///
  virtual ~Capture()
  {
    close();
  }

  ///
  /// 画像ファイルを開く
  ///
  /// @param 開く画像ファイル名
  /// @return 開くことができたら true
  ///
  bool openImage(const std::string& filename);

  ///
  /// 動画ファイルを開く
  ///
  /// @param filename 開く動画ファイル名
  /// @param backend バックエンドの種類
  /// @return 開くことができたら true
  ///
  bool openMovie(const std::string& filename);

  ///
  /// キャプチャデバイスを開く
  ///
  /// @param deviceNumber 開くデバイス番号
  /// @return 開くことができたら true
  ///
  bool openDevice(int deviceNumber);

  ///
  /// 使用可能なビデオフォーマットの表示名のリストを得る
  ///
  /// @return 使用可能なビデオフォーマットの表示名のリスト
  ///
  const std::vector<std::string>* getFormatList() const
  {
    return formatList;
  }

  ///
  /// キャプチャ開始
  ///
  void start();

  ///
  /// キャプチャ終了
  ///
  void stop();

  ///
  /// キャプチャデバイスを閉じる
  ///
  void close();

  ///
  /// キャプチャ中か否か
  ///
  /// @return キャプチャ中なら true
  ///
  explicit operator bool() const
  {
    return camera && camera->isRunning();
  }

  ///
  /// キャプチャデバイスが有効かどうか
  ///
  bool isOpend() const
  {
    return bool(camera);
  }

  ///
  /// キャプチャデバイスのフレームの解像度を得る
  /// 
  /// @return キャプチャデバイスのフレームの解像度
  ///
  std::array<int, 2> getSize() const;

  ///
  /// キャプチャデバイスのフレームレートを得る
  /// 
  /// @return キャプチャデバイスのフレームレート
  ///
  double getFps() const;

  ///
  /// フレームを取得する
  ///
  /// @param buffer 取得したフレームを格納するバッファ
  ///
  void retrieve(Buffer& buffer);
};
