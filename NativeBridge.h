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

    /// キャプチャフレームの幅
    std::atomic<int> frameWidth{ 1280 };

    /// キャプチャフレームの高さ
    std::atomic<int> frameHeight{ 720 };

    /// 画面破棄前にキャプチャ中だったか否か
    bool wasCapturingBeforeDestroy{ false };

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

    ///
    /// キャプチャフレームの幅を取得する
    ///
    /// @return キャプチャフレームの幅 (px)
    ///
    int getFrameWidth() const { return frameWidth.load(); }

    ///
    /// キャプチャフレームの高さを取得する
    ///
    /// @return キャプチャフレームの高さ (px)
    ///
    int getFrameHeight() const { return frameHeight.load(); }

    // --- カメラ解像度 ---

    ///
    /// 利用可能なカメラ解像度の総数を取得する
    ///
    /// @return 解像度の数
    ///
    int getResolutionCount() const;

    ///
    /// インデックス指定で利用可能なカメラ解像度を取得する
    ///
    /// @param index 解像度インデックス (0 <= index < getResolutionCount())
    /// @return 解像度文字列 (例: "1280 x 720"、範囲外なら空文字列)
    ///
    std::string getResolutionByIndex(int index) const;

    ///
    /// 現在選択されているカメラ解像度を取得する
    ///
    /// @return 現在の解像度文字列 (例: "1280 x 720")
    ///
    std::string getCurrentResolution() const;

    ///
    /// カメラ解像度を選択する
    ///
    /// @param resolution 選択する解像度文字列 (例: "1280 x 720")
    /// @return 変更に成功したら true
    ///
    bool selectResolution(const std::string& resolution);

    // --- ArUco Marker 認識 ---

    ///
    /// マーカー検出が有効かどうかを判定する
    ///
    /// @return マーカー検出が有効なら true
    ///
    bool isDetectMarker() const;

    ///
    /// マーカー検出の有効／無効を設定する
    ///
    /// @param enabled 有効にするなら true
    ///
    void setDetectMarker(bool enabled);

    ///
    /// ArUco Marker 一辺の長さを取得する
    ///
    /// @return マーカー一辺の長さ
    ///
    float getMarkerLength() const;

    ///
    /// ArUco Marker 一辺の長さを設定する
    ///
    /// @param length マーカー一辺の長さ
    ///
    void setMarkerLength(float length);

    ///
    /// 現在のマーカー辞書名を取得する
    ///
    /// @return マーカー辞書名
    ///
    std::string getDictionaryName() const;

    ///
    /// マーカー辞書を設定する
    ///
    /// @param name マーカー辞書名
    ///
    void setDictionary(const std::string& name);

    ///
    /// 利用可能なマーカー辞書の総数を取得する
    ///
    /// @return マーカー辞書の数
    ///
    int getDictionaryCount() const;

    ///
    /// インデックス指定でマーカー辞書名を取得する
    ///
    /// @param index 辞書インデックス (0 <= index < getDictionaryCount())
    /// @return 辞書名（範囲外の場合は空文字列）
    ///
    std::string getDictionaryNameByIndex(int index) const;

    // --- 較正パラメータとレンズ歪み補正 ---

    ///
    /// 較正ファイル（JSON）を読み込む
    ///
    /// @param filename 較正ファイルのパス
    /// @return 読み込みに成功したら true
    ///
    bool loadCalibration(const std::string& filename);

    ///
    /// 較正パラメータが利用可能かどうかを判定する
    ///
    /// @return 歪み補正が実行可能なら true
    ///
    bool isCalibrationReady() const;

    ///
    /// 現在の歪み補正方式を取得する
    ///
    /// @return 歪み補正方式 (0: なし, 1: OpenCV)
    ///
    int getUndistortionMode() const;

    ///
    /// 歪み補正方式を設定する
    ///
    /// @param mode 歪み補正方式 (0: なし, 1: OpenCV)
    ///
    void setUndistortionMode(int mode);

    ///
    /// 入力のフレームの解像度と較正時の解像度の関係を得る
    ///
    /// @return 較正値を読み込んでいなければ CalibrationSizeMatch::Unknown
    ///
    CalibrationSizeMatch getCalibrationSizeMatch() const;

    ///
    /// 較正時の解像度に関する警告の文言を得る
    ///
    /// @return 警告の文字列 (UTF-8), 警告が無ければ空文字列
    ///
    std::string getCalibrationWarning() const;

    // --- 一括状態取得 ---

    ///
    /// UI 表示用のステータス配列を一括取得する
    ///
    /// @param[out] outStatus 出力先バッファ
    ///   [0] キャプチャ中, [1] マーカー検出, [2] 較正済, [3] 歪み補正方式, [4] マーカー長,
    ///   [5] フレームレート, [6] フレームの幅, [7] フレームの高さ,
    ///   [8] 較正時の解像度に関する警告 (0: なし, 1: 換算して補正, 2: アスペクト比が異なる)
    /// @param count バッファの要素数 (6 以上)
    ///
    void getStatus(float* outStatus, int count) const;

    ///
    /// シングルトンインスタンスを取得
    ///
    static NativeEngine& getInstance();
  };
}

#endif // defined(__ANDROID__)
