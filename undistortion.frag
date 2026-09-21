#version 330

//
// OpenCV の5係数レンズモデルを使う歪み補正用フラグメントシェーダ
//
// 出力側の歪みのない画素位置から、元画像上の歪んだ座標を計算してサンプリングする。
// これにより画像をCPUで作り直さず、表示時にGPUだけでレンズ歪みを補正できる。
//

// 補正前の入力画像
uniform sampler2D image;

// 計算した入力座標が画像外に出たときの色
uniform vec4 border;

// カメラ行列の主要成分 (fx, fy, cx, cy)
uniform vec4 camera;

// OpenCV と同じ順序の歪み係数 (k1, k2, p1, p2, k3)
uniform float distortion[5];

// 入力画像の解像度（幅、高さ）
uniform vec2 resolution;

// 歪みのない出力画像上のテクスチャ座標
in vec2 texcoord;

// 補正後のフラグメント色
layout (location = 0) out vec4 fc;

void main(void)
{
  // OpenGLの正規化座標を、OpenCVと同じ左上原点の画素座標へ変換する。
  vec2 pixel = vec2(texcoord.x * resolution.x,
    (1.0 - texcoord.y) * resolution.y);

  // カメラ行列 K の逆変換に相当する計算で、画素座標を正規化カメラ座標へ移す。
  vec2 normalized = (pixel - camera.zw) / camera.xy;

  // 正規化座標と原点からの距離の二乗を求める。
  float x = normalized.x;
  float y = normalized.y;
  float r2 = x * x + y * y;

  // k1、k2、k3による半径方向歪みの倍率を計算する。
  float radial = 1.0 + distortion[0] * r2
    + distortion[1] * r2 * r2
    + distortion[4] * r2 * r2 * r2;

  // 半径方向歪みに、p1、p2による接線方向歪みを加える。
  vec2 distorted;
  distorted.x = x * radial + 2.0 * distortion[2] * x * y
    + distortion[3] * (r2 + 2.0 * x * x);
  distorted.y = y * radial + distortion[2] * (r2 + 2.0 * y * y)
    + 2.0 * distortion[3] * x * y;

  // 歪んだ正規化座標をカメラ行列 K で入力画像の画素座標へ戻す。
  vec2 sourcePixel = distorted * camera.xy + camera.zw;

  // texture() が使う [0, 1] 座標へ変換し、OpenGLのy軸へ戻す。
  vec2 source = vec2(sourcePixel.x / resolution.x,
    1.0 - sourcePixel.y / resolution.y);

  // 入力画像外は境界色、画像内は計算した位置の画素を線形補間して表示する。
  vec4 bounds = vec4(source, 1.0 - source);
  fc = all(greaterThanEqual(bounds, vec4(0.0)))
    ? texture(image, source)
    : border.bgra;
}
