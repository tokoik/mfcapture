///
/// Android JNI ブリッジとレンダリングエンジンの実装
///
/// @file
/// @author Kohe Tokoi
/// @date March 2026
///
#if defined(__ANDROID__)

#include "NativeBridge.h"

#include <android/asset_manager.h>
#include <android/log.h>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <cmath>
#include <chrono>

#define LOG_TAG "mfcapture-jni"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace
{
  ///
  /// アセットを内部ストレージへ展開する
  ///
  bool extractSingleAsset(AAssetManager* mgr, const char* filename, const std::string& targetDir)
  {
    if (!mgr || !filename) return false;

    AAsset* asset{ AAssetManager_open(mgr, filename, AASSET_MODE_BUFFER) };
    if (!asset)
    {
      LOGW("Asset not found in APK: %s", filename);
      return false;
    }

    const off_t size{ AAsset_getLength(asset) };
    const std::string destPath{ targetDir + "/" + filename };

    bool needExtract{ true };
    std::ifstream check(destPath, std::ios::binary | std::ios::ate);
    if (check.is_open())
    {
      if (check.tellg() == size)
      {
        needExtract = false;
      }
      check.close();
    }

    if (needExtract)
    {
      std::ofstream out(destPath, std::ios::binary);
      if (!out.is_open())
      {
        LOGE("Failed to open destination for write: %s", destPath.c_str());
        AAsset_close(asset);
        return false;
      }

      std::vector<char> buffer(65536);
      int bytesRead{ 0 };
      while ((bytesRead = AAsset_read(asset, buffer.data(), static_cast<int>(buffer.size()))) > 0)
      {
        out.write(buffer.data(), bytesRead);
      }
      out.close();
      LOGI("Extracted asset: %s (%ld bytes)", filename, static_cast<long>(size));
    }

    AAsset_close(asset);
    return true;
  }
}

namespace mfcapture
{
  NativeEngine::NativeEngine() = default;

  NativeEngine::~NativeEngine()
  {
    onSurfaceDestroyed();
  }

  NativeEngine& NativeEngine::getInstance()
  {
    static NativeEngine instance;
    return instance;
  }

  void NativeEngine::init(AAssetManager* assetManager, const char* internalPath)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (!internalPath) return;

    LOGI("Setting working directory to: %s", internalPath);
    if (chdir(internalPath) != 0)
    {
      LOGW("Failed to chdir to: %s", internalPath);
    }

    if (!assetManager) return;

    // 必須アセットを展開
    static const char* const requiredAssets[]{
      "mfcapture_config.json",
      "castle.jpg",
      "initial.jpg"
    };

    const std::string pathStr{ internalPath };
    for (const auto* assetName : requiredAssets)
    {
      extractSingleAsset(assetManager, assetName, pathStr);
    }

    // 展開後に Config, Capture, Undistortion, Aruco, Menu を構築
    config = std::make_unique<Config>("mfcapture_config.json");
    capture = std::make_unique<Capture>();
    undistortion = std::make_unique<Undistortion>();
    aruco = std::make_unique<Aruco>(config->getSettings().dictionaryName);
    menu = std::make_unique<Menu>(*config, *capture, *undistortion, *aruco);

    // デフォルトでマーカー検出を ON に設定
    menu->detectMarker = true;

    // calibration.json が内部ストレージに存在すれば自動読み込み
    if (undistortion->load("calibration.json"))
    {
      LOGI("Automatically loaded calibration.json");
    }
  }



  void NativeEngine::onSurfaceCreated(ANativeWindow* window)
  {
    LOGI("onSurfaceCreated");
    onSurfaceDestroyed();

    nativeWindow = window;
    isRunning = true;
    renderThread = std::thread(&NativeEngine::renderLoop, this);
  }

  void NativeEngine::onSurfaceChanged(int width, int height)
  {
    LOGI("onSurfaceChanged: %d x %d", width, height);
    windowWidth = width;
    windowHeight = height;
    sizeChanged = true;
  }

  void NativeEngine::onSurfaceDestroyed()
  {
    LOGI("onSurfaceDestroyed");
    isRunning = false;
    if (renderThread.joinable())
    {
      renderThread.join();
    }
    if (nativeWindow)
    {
      ANativeWindow_release(nativeWindow);
      nativeWindow = nullptr;
    }
  }

  bool NativeEngine::startCapture()
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->startCapture() : false;
  }

  void NativeEngine::stopCapture()
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (capture)
    {
      capture->stop();
      capture->close();
    }
  }

  bool NativeEngine::isCapturing() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return capture ? bool(*capture) : false;
  }

  bool NativeEngine::isDetectMarker() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->detectMarker : false;
  }

  void NativeEngine::setDetectMarker(bool enabled)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (menu) menu->detectMarker = enabled;
  }

  float NativeEngine::getMarkerLength() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getMarkerLength() : 0.0f;
  }

  void NativeEngine::setMarkerLength(float length)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (menu)
    {
      menu->getSettings().markerLength = length;
    }
  }

  std::string NativeEngine::getDictionaryName() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getSettings().dictionaryName : "";
  }

  void NativeEngine::setDictionary(const std::string& name)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (menu && aruco)
    {
      menu->getSettings().dictionaryName = name;
      aruco->setDictionary(name);
    }
  }

  int NativeEngine::getDictionaryCount() const
  {
    return static_cast<int>(Aruco::dictionaryList.size());
  }

  std::string NativeEngine::getDictionaryNameByIndex(int index) const
  {
    if (index < 0 || index >= static_cast<int>(Aruco::dictionaryList.size())) return "";
    auto it = Aruco::dictionaryList.begin();
    std::advance(it, index);
    return it->first;
  }

  bool NativeEngine::loadCalibration(const std::string& filename)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (!undistortion) return false;
    const bool ok = undistortion->load(filename);
    if (ok)
    {
      LOGI("Successfully loaded calibration: %s", filename.c_str());
    }
    else
    {
      LOGE("Failed to load calibration: %s", filename.c_str());
      undistortionMode = UndistortionMode::None;
    }
    return ok;
  }

  bool NativeEngine::isCalibrationReady() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return undistortion ? undistortion->ready() : false;
  }

  int NativeEngine::getUndistortionMode() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return (undistortionMode == UndistortionMode::OpenCV) ? 1 : 0;
  }

  void NativeEngine::setUndistortionMode(int mode)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (mode == 1 && undistortion && undistortion->ready())
    {
      undistortionMode = UndistortionMode::OpenCV;
    }
    else
    {
      undistortionMode = UndistortionMode::None;
    }
  }

  void NativeEngine::getStatus(float* outStatus, int count) const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (!outStatus || count < 6) return;
    outStatus[0] = (capture && bool(*capture)) ? 1.0f : 0.0f;
    outStatus[1] = (menu && menu->detectMarker) ? 1.0f : 0.0f;
    outStatus[2] = (undistortion && undistortion->ready()) ? 1.0f : 0.0f;
    outStatus[3] = (undistortionMode == UndistortionMode::OpenCV) ? 1.0f : 0.0f;
    outStatus[4] = menu ? menu->getMarkerLength() : 0.0f;
    outStatus[5] = currentFps;
    if (count >= 8)
    {
      outStatus[6] = static_cast<float>(frameWidth.load());
      outStatus[7] = static_cast<float>(frameHeight.load());
    }
  }

  int NativeEngine::getResolutionCount() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getResolutionCount() : 0;
  }

  std::string NativeEngine::getResolutionByIndex(int index) const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getResolutionByIndex(index) : "";
  }

  std::string NativeEngine::getCurrentResolution() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getCurrentResolution() : "";
  }

  bool NativeEngine::selectResolution(const std::string& resolution)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (!menu) return false;
    return menu->selectResolution(resolution);
  }

  void NativeEngine::renderLoop()
  {
    LOGI("renderLoop started");

    ANativeWindow* currentWin{ nullptr };
    int lastW{ 0 };
    int lastH{ 0 };

    {
      std::lock_guard<std::mutex> lock(engineMutex);
      if (!config || !capture || !undistortion || !aruco || !menu)
      {
        LOGE("Engine components are not initialized");
        return;
      }

      config->initialize();

      // 背面カメラの自動検索・開始
      bool cameraStarted{ false };
      const auto& deviceList{ config->getDeviceList() };
      int backCameraIndex{ -1 };

      for (int i = 0; i < static_cast<int>(deviceList.size()); ++i)
      {
        if (deviceList[i].find("Back") != std::string::npos || deviceList[i].find("back") != std::string::npos)
        {
          backCameraIndex = i;
          break;
        }
      }

      if (backCameraIndex < 0 && !deviceList.empty())
      {
        backCameraIndex = 0;
      }

      if (backCameraIndex >= 0)
      {
        menu->setDeviceNumber(backCameraIndex);
        cameraStarted = menu->startCapture();
      }

      if (!cameraStarted)
      {
        if (capture->openImage(config->getInitialImage()))
        {
          capture->start();
          menu->initializeInputIntrinsics(capture->getSize());
        }
      }
    }

    cv::Mat cpuFrame;
    cv::Mat correctedFrame;
    cv::Mat displayFrame;

    auto lastFpsTime{ std::chrono::steady_clock::now() };
    int frameCount{ 0 };

    while (isRunning)
    {
      ANativeWindow* win{ nativeWindow.load() };
      if (win != currentWin)
      {
        currentWin = win;
        lastW = 0;
        lastH = 0;
      }

      if (!currentWin)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        continue;
      }

      // FPS 計測 (1秒ごと)
      ++frameCount;
      const auto now{ std::chrono::steady_clock::now() };
      const auto elapsed{ std::chrono::duration<float>(now - lastFpsTime).count() };
      if (elapsed >= 1.0f)
      {
        currentFps = static_cast<float>(frameCount) / elapsed;
        frameCount = 0;
        lastFpsTime = now;
      }

      // フレーム取得と画像処理 (ゼロストール)
      {
        std::lock_guard<std::mutex> lock(engineMutex);

        if (capture && *capture)
        {
          const bool hasNewFrame{ capture->retrieve(cpuFrame) };
          if (hasNewFrame && !cpuFrame.empty())
          {
            frameWidth = cpuFrame.cols;
            frameHeight = cpuFrame.rows;
            cv::Mat targetFrame;

            // 1. OpenCV 歪み補正 (較正データがロードされている場合)
            if (undistortionMode == UndistortionMode::OpenCV && undistortion->ready())
            {
              undistortion->apply(cpuFrame, correctedFrame);
              targetFrame = correctedFrame;
            }
            else
            {
              targetFrame = cpuFrame;
            }

            // 2. ArUco Marker 認識および姿勢推定
            if (menu->detectMarker)
            {
              if (undistortion->ready())
              {
                // 較正パラメータがある場合：
                // 既に歪み補正済みの場合は歪み係数ゼロ（空行列）、補正なしの場合は歪み係数を渡す
                const cv::Mat& distCoeffs = (undistortionMode == UndistortionMode::OpenCV)
                  ? cv::Mat{} : undistortion->getDistortion();
                aruco->detectMarkers(targetFrame, menu->getMarkerLength(),
                  undistortion->getCameraMatrix(), distCoeffs);
              }
              else
              {
                // 較正データがない場合はマーカー枠と ID を描画
                aruco->detectMarkers(targetFrame, menu->getMarkerLength(),
                  cv::Mat{}, cv::Mat{});
              }
            }

            // 3. ウィンドウバッファサイズ設定（解像度変更時）
            if (lastW != targetFrame.cols || lastH != targetFrame.rows)
            {
              ANativeWindow_setBuffersGeometry(currentWin, targetFrame.cols, targetFrame.rows, WINDOW_FORMAT_RGBA_8888);
              lastW = targetFrame.cols;
              lastH = targetFrame.rows;
              LOGI("ANativeWindow buffers geometry set to: %d x %d", lastW, lastH);
            }

            // 4. ANativeWindow へ直接描画 (BGRA -> RGBA)
            ANativeWindow_Buffer winBuf;
            if (ANativeWindow_lock(currentWin, &winBuf, nullptr) == 0)
            {
              if (targetFrame.channels() == 4)
              {
                cv::cvtColor(targetFrame, displayFrame, cv::COLOR_BGRA2RGBA);
              }
              else if (targetFrame.channels() == 3)
              {
                cv::cvtColor(targetFrame, displayFrame, cv::COLOR_BGR2RGBA);
              }
              else
              {
                displayFrame = targetFrame;
              }

              const int copyRows{ std::min(winBuf.height, displayFrame.rows) };
              const int srcRowBytes{ displayFrame.cols * 4 };
              const int dstStrideBytes{ winBuf.stride * 4 };
              const uint8_t* srcBits{ displayFrame.data };
              uint8_t* dstBits{ static_cast<uint8_t*>(winBuf.bits) };

              if (winBuf.stride == displayFrame.cols)
              {
                std::memcpy(dstBits, srcBits, srcRowBytes * copyRows);
              }
              else
              {
                for (int y = 0; y < copyRows; ++y)
                {
                  std::memcpy(dstBits + y * dstStrideBytes, srcBits + y * srcRowBytes, srcRowBytes);
                }
              }

              ANativeWindow_unlockAndPost(currentWin);
            }
          }
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    LOGI("renderLoop finished");
  }
}

//
// JNI 関数定義
//
extern "C"
{
  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeInit(
    JNIEnv* env, jclass, jobject assetManager, jstring internalPath)
  {
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    const char* path = env->GetStringUTFChars(internalPath, nullptr);
    mfcapture::NativeEngine::getInstance().init(mgr, path);
    env->ReleaseStringUTFChars(internalPath, path);
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeSurfaceCreated(
    JNIEnv* env, jclass, jobject surface)
  {
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    mfcapture::NativeEngine::getInstance().onSurfaceCreated(window);
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeSurfaceChanged(
    JNIEnv*, jclass, jint width, jint height)
  {
    mfcapture::NativeEngine::getInstance().onSurfaceChanged(width, height);
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeSurfaceDestroyed(
    JNIEnv*, jclass)
  {
    mfcapture::NativeEngine::getInstance().onSurfaceDestroyed();
  }

  JNIEXPORT jboolean JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeStartCapture(
    JNIEnv*, jclass)
  {
    return mfcapture::NativeEngine::getInstance().startCapture() ? JNI_TRUE : JNI_FALSE;
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeStopCapture(
    JNIEnv*, jclass)
  {
    mfcapture::NativeEngine::getInstance().stopCapture();
  }

  JNIEXPORT jboolean JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeIsCapturing(
    JNIEnv*, jclass)
  {
    return mfcapture::NativeEngine::getInstance().isCapturing() ? JNI_TRUE : JNI_FALSE;
  }

  JNIEXPORT jboolean JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeIsDetectMarker(
    JNIEnv*, jclass)
  {
    return mfcapture::NativeEngine::getInstance().isDetectMarker() ? JNI_TRUE : JNI_FALSE;
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeSetDetectMarker(
    JNIEnv*, jclass, jboolean enabled)
  {
    mfcapture::NativeEngine::getInstance().setDetectMarker(enabled == JNI_TRUE);
  }

  JNIEXPORT jfloat JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeGetMarkerLength(
    JNIEnv*, jclass)
  {
    return mfcapture::NativeEngine::getInstance().getMarkerLength();
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeSetMarkerLength(
    JNIEnv*, jclass, jfloat length)
  {
    mfcapture::NativeEngine::getInstance().setMarkerLength(length);
  }

  JNIEXPORT jstring JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeGetDictionaryName(
    JNIEnv* env, jclass)
  {
    return env->NewStringUTF(mfcapture::NativeEngine::getInstance().getDictionaryName().c_str());
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeSetDictionary(
    JNIEnv* env, jclass, jstring name)
  {
    const char* str = env->GetStringUTFChars(name, nullptr);
    mfcapture::NativeEngine::getInstance().setDictionary(str);
    env->ReleaseStringUTFChars(name, str);
  }

  JNIEXPORT jint JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeGetDictionaryCount(
    JNIEnv*, jclass)
  {
    return mfcapture::NativeEngine::getInstance().getDictionaryCount();
  }

  JNIEXPORT jstring JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeGetDictionaryNameByIndex(
    JNIEnv* env, jclass, jint index)
  {
    return env->NewStringUTF(mfcapture::NativeEngine::getInstance().getDictionaryNameByIndex(index).c_str());
  }

  JNIEXPORT jboolean JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeLoadCalibration(
    JNIEnv* env, jclass, jstring filename)
  {
    if (!filename) return JNI_FALSE;
    const char* str = env->GetStringUTFChars(filename, nullptr);
    bool ok = mfcapture::NativeEngine::getInstance().loadCalibration(str);
    env->ReleaseStringUTFChars(filename, str);
    return ok ? JNI_TRUE : JNI_FALSE;
  }

  JNIEXPORT jboolean JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeIsCalibrationReady(
    JNIEnv*, jclass)
  {
    return mfcapture::NativeEngine::getInstance().isCalibrationReady() ? JNI_TRUE : JNI_FALSE;
  }

  JNIEXPORT jint JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeGetUndistortionMode(
    JNIEnv*, jclass)
  {
    return mfcapture::NativeEngine::getInstance().getUndistortionMode();
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeSetUndistortionMode(
    JNIEnv*, jclass, jint mode)
  {
    mfcapture::NativeEngine::getInstance().setUndistortionMode(mode);
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeGetStatus(
    JNIEnv* env, jclass, jfloatArray outStatus)
  {
    if (!outStatus) return;
    jsize len = env->GetArrayLength(outStatus);
    if (len < 6) return;
    const int count{ std::min(static_cast<int>(len), 8) };
    jfloat buf[8]{};
    mfcapture::NativeEngine::getInstance().getStatus(buf, count);
    env->SetFloatArrayRegion(outStatus, 0, count, buf);
  }

  JNIEXPORT jint JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeGetFrameWidth(
    JNIEnv*, jclass)
  {
    return mfcapture::NativeEngine::getInstance().getFrameWidth();
  }

  JNIEXPORT jint JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeGetFrameHeight(
    JNIEnv*, jclass)
  {
    return mfcapture::NativeEngine::getInstance().getFrameHeight();
  }

  JNIEXPORT jint JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeGetResolutionCount(
    JNIEnv*, jclass)
  {
    return mfcapture::NativeEngine::getInstance().getResolutionCount();
  }

  JNIEXPORT jstring JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeGetResolutionByIndex(
    JNIEnv* env, jclass, jint index)
  {
    const std::string res{ mfcapture::NativeEngine::getInstance().getResolutionByIndex(index) };
    return env->NewStringUTF(res.c_str());
  }

  JNIEXPORT jstring JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeGetCurrentResolution(
    JNIEnv* env, jclass)
  {
    const std::string res{ mfcapture::NativeEngine::getInstance().getCurrentResolution() };
    return env->NewStringUTF(res.c_str());
  }

  JNIEXPORT jboolean JNICALL Java_net_wakayama_1u_tokoi_mfcapture_NativeBridge_nativeSelectResolution(
    JNIEnv* env, jclass, jstring resStr)
  {
    if (!resStr) return JNI_FALSE;
    const char* res{ env->GetStringUTFChars(resStr, nullptr) };
    const bool ok{ mfcapture::NativeEngine::getInstance().selectResolution(res) };
    env->ReleaseStringUTFChars(resStr, res);
    return ok ? JNI_TRUE : JNI_FALSE;
  }
}

#endif // defined(__ANDROID__)
