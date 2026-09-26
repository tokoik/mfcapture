///
/// Microsoft Media Foundation によるビデオキャプチャ
///
/// @file
/// @author Kohe Tokoi
/// @date March 6, 2024
///

// ウィンドウ関連の処理
#include "GgApp.h"

// 構成データ
#include "Config.h"

// キャプチャデバイス
#include "Capture.h"

// フレームバッファオブジェクト
#include "Framebuffer.h"

// メニュー
#include "Menu.h"

// ArUco Marker 認識
#include "Aruco.h"

// 構成ファイル名
#define CONFIG_FILE PROJECT_NAME "_config.json"

//
// アプリケーション本体
//
int GgApp::main(int argc, const char* const* argv)
{
  // 構成ファイルを読み込む
  Config config{ CONFIG_FILE };

  // 構成にもとづいてウィンドウを作成する
  GgApp::Window window{ config.getTitle(), config.getWidth(), config.getHeight() };

  // 開いたウィンドウに対して初期化処理を実行する
  config.initialize();

  // キャプチャデバイスを作る
  Capture capture;

  // calib が出力したカメラ内部パラメータと歪み係数
  Undistortion undistortion;

  // ArUco Marker 認識
  Aruco aruco{ config.getSettings().dictionaryName };

  // メニューを作る
  Menu menu{ config, capture, undistortion, aruco };

  // キャプチャデバイスで初期画像を開く
  if (!capture.openImage(config.getInitialImage())) throw std::runtime_error("Cannot open initial image.");

  // 実解像度と焦点距離から初期画角を設定する
  menu.initializeInputIntrinsics(capture.getSize());

  // キャプチャしたフレームを保持するテクスチャ
  Texture frame;

  // OpenGL 歪み補正時に使用する中間フレームバッファオブジェクト
  Framebuffer undistortedFramebuffer;

  // OpenCV 補正時に使用する CPU フレーム
  cv::Mat sourceFrame;
  cv::Mat correctedFrame;

  // 画像の展開に用いるフレームバッファオブジェクトのサイズを初期ウィンドウに合わせる
  Framebuffer framebuffer{ config.getWidth(), config.getHeight() };

  // ウィンドウが開いている間繰り返す
  while (window && menu)
  {
    // メニューを表示して設定を更新する
    menu.draw();

    // 展開パスへ渡すフレームへのポインタ
    const Texture* processedFrame{ &frame };

    if (menu.getUndistortionMode() == UndistortionMode::OpenCV)
    {
      // OpenCV方式では、GPUへ送る前のフレームをCPUメモリへ取得する。
      // この位置で補正すれば、テクスチャをGPUから読み戻す余分な転送が発生しない。
      if (capture.retrieve(sourceFrame))
      {
        // 較正値から作成した座標マップでレンズ歪みを補正する。
        undistortion.apply(sourceFrame, correctedFrame);

        // ArUco Marker を検出するなら
        if (menu.detectMarker)
        {
          // 較正後画像に対してマーカー認識を行う。
          // 既に歪み補正済みのため歪み係数は渡さない（空行列）。
          aruco.detectMarkers(correctedFrame, menu.getMarkerLength(),
            undistortion.ready() ? undistortion.getCameraMatrix() : cv::Mat{},
            cv::Mat{});
        }

        // 補正済みのCPU画像を表示用テクスチャへアップロードする。
        frame.drawPixels(correctedFrame.cols, correctedFrame.rows,
          correctedFrame.channels(), correctedFrame.data);
      }
    }
    else if (menu.getUndistortionMode() == UndistortionMode::OpenGL)
    {
      // 補正なし／OpenGL方式ではCPU処理が不要なので、高速なPBO経路を維持する。
      if (capture.retrieve(frame)) frame.drawPixels();

      // 第1パス：OpenGLによるGPUレンズ歪み補正
      undistortedFramebuffer.resize(frame);
      const auto&& undistortSize{ menu.setupUndistortion(undistortedFramebuffer.getAspect()) };
      undistortedFramebuffer.update(undistortSize, frame);

      // ArUco Marker を検出するなら
      if (menu.detectMarker)
      {
        // 較正後画像（undistortedFramebuffer）の内容をピクセルバッファオブジェクトに転送する
        undistortedFramebuffer.readPixels();

        // 較正後画像のサイズを調べる
        const auto size{ cv::Size{ undistortedFramebuffer.getWidth(), undistortedFramebuffer.getHeight() } };

        // ピクセルバッファオブジェクトを CPU のメモリ空間にマップする
        cv::Mat image{ size, CV_8UC(undistortedFramebuffer.getChannels()), undistortedFramebuffer.map() };

        // 較正後画像に対してマーカー認識を行う。
        // 既に歪み補正済みのため歪み係数は渡さない（空行列）。
        aruco.detectMarkers(image, menu.getMarkerLength(),
          undistortion.ready() ? undistortion.getCameraMatrix() : cv::Mat{},
          cv::Mat{});

        // ピクセルバッファオブジェクトのマップを解除する
        undistortedFramebuffer.unmap();

        // ピクセルバッファオブジェクトの内容をフレームバッファオブジェクトに書き戻す
        undistortedFramebuffer.drawPixels();
      }

      // 補正結果のフレームを展開パスの入力とする
      processedFrame = &undistortedFramebuffer;
    }
    else
    {
      // ArUco Marker を検出するなら
      if (menu.detectMarker)
      {
        if (capture.retrieve(sourceFrame))
        {
          // 補正前の生画像に対してマーカー認識を行う。
          // 較正パラメータがある場合は歪み係数も渡して座標軸を描く。
          aruco.detectMarkers(sourceFrame, menu.getMarkerLength(),
            undistortion.ready() ? undistortion.getCameraMatrix() : cv::Mat{},
            undistortion.ready() ? undistortion.getDistortion() : cv::Mat{});

          // マーカー描画済みの生画像を表示用テクスチャへアップロードする。
          frame.drawPixels(sourceFrame.cols, sourceFrame.rows,
            sourceFrame.channels(), sourceFrame.data);
        }
      }
      else
      {
        // 補正なし・認識なしではCPU処理が不要なので、高速なPBO経路を維持する。
        if (capture.retrieve(frame)) frame.drawPixels();
      }
    }

    // 第2パス：フレームバッファオブジェクトのサイズを補正後のフレームに合わせる
    framebuffer.resize(*processedFrame);

    // 投影方式に応じた展開シェーダの設定を行う
    const auto&& size{ menu.setup(framebuffer.getAspect()) };

    // フレームバッファオブジェクトにフレームを展開する
    framebuffer.update(size, *processedFrame);

    // 表示するウィンドウのビューポートを再設定する
    window.setMenubarHeight(menu.getMenubarHeight());

    // フレームバッファオブジェクトの内容を表示する
    //framebuffer.show(window.getWidth(), window.getHeight());
    framebuffer.draw(window.getFboWidth(), window.getFboHeight());

    // カラーバッファを入れ替えてイベントを取り出す
    window.swapBuffers();
  }

  return 0;
}
