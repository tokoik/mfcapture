#pragma once

///
/// Android JNI ブリッジとレンダリングエンジンの定義 (OpenGL 非依存)
///
/// @file
/// @author Kohe Tokoi
/// @date March 2026
///
#if defined(__ANDROID__)

#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>

#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <vector>

#include "Config.h"
#include "Capture.h"
#include "Undistortion.h"
#include "Aruco.h"
#include "Menu.h"

namespace mfcapture
{
  ///
  /// Android レンダリングと画像処理を管理するエンジンクラス (Direct ANativeWindow CPU Blit)
  ///
  class NativeEngine
  {
    /// 排他制御用ミューテックス
    mutable std::mutex engineMutex;

    /// 設定情報
    std::unique_ptr<Config> config;

    /// キャプチャデバイス
    std::unique_ptr<Capture> capture;

    /// レンズ歪み補正処理
    std::unique_ptr<Undistortion> undistortion;

    /// ArUco Marker 認識処理
    std::unique_ptr<Aruco> aruco;

    /// メニュー（設定管理）
    std::unique_ptr<Menu> menu;

    /// 選択中の歪み補正方式（None または OpenCV）
    UndistortionMode undistortionMode{ UndistortionMode::None };

    /// 現在の描画フレームレート
    float currentFps{ 0.0f };

    /// レンダリング用スレッド
    std::thread renderThread;

    /// 動作中フラグ
    std::atomic<bool> isRunning{ false };

    /// 現在のネイティブウィンドウ
    std::atomic<ANativeWindow*> nativeWindow{ nullptr };

    /// 描画領域の幅
    std::atomic<int> windowWidth{ 0 };

    /// 描画領域の高さ
    std::atomic<int> windowHeight{ 0 };

    /// ウィンドウサイズ更新フラグ
    std::atomic<bool> sizeChanged{ false };

    ///
    /// レンダリングループ本体 (ANativeWindow 直接描画)
    ///
    void renderLoop();

  public:

    ///
    /// コンストラクタ
    ///
    NativeEngine();

    ///
    /// デストラクタ
    ///
    ~NativeEngine();

    ///
    /// 初期化（アセット展開および作業ディレクトリ設定）
    ///
    /// @param assetManager Android AssetManager
    /// @param internalPath アプリ内部ストレージのパス
    ///
    void init(AAssetManager* assetManager, const char* internalPath);

    ///
    /// Surface が生成されたときの処理
    ///
    /// @param window 対象の ANativeWindow
    ///
    void onSurfaceCreated(ANativeWindow* window);

    ///
    /// Surface のサイズが変更されたときの処理
    ///
    /// @param width 新しい幅
    /// @param height 新しい高さ
    ///
    void onSurfaceChanged(int width, int height);

    ///
    /// Surface が破棄されたときの処理
    ///
    void onSurfaceDestroyed();

    ///
    /// キャプチャを開始する
    ///
    /// @return 開始に成功したら true
    ///
    bool startCapture();

    ///
    /// キャプチャを停止する
    ///
    void stopCapture();

    ///
    /// キャプチャ中かどうかを判定する
    ///
    /// @return キャプチャ中なら true
    ///
    bool isCapturing() const;

    // --- ArUco Marker 認識 ---
    bool isDetectMarker() const;
    void setDetectMarker(bool enabled);

    float getMarkerLength() const;
    void setMarkerLength(float length);

    std::string getDictionaryName() const;
    void setDictionary(const std::string& name);

    int getDictionaryCount() const;
    std::string getDictionaryNameByIndex(int index) const;

    // --- 較正パラメータとレンズ歪み補正 ---
    bool loadCalibration(const std::string& filename);
    bool isCalibrationReady() const;

    int getUndistortionMode() const;
    void setUndistortionMode(int mode);

    // --- 一括状態取得 ---
    void getStatus(float* outStatus, int count) const;

    ///
    /// シングルトンインスタンスを取得
    ///
    static NativeEngine& getInstance();
  };
}

#endif // defined(__ANDROID__)
