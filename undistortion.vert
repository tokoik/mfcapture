#version 410

//
// レンズ歪み補正用バーテックスシェーダ
//
// 入力画像全体を表示する格子を作り、各フラグメントへ歪みのない画像上の
// テクスチャ座標を渡す。実際のレンズモデルの計算はフラグメントシェーダで行う。
//

// クリッピング空間を埋める格子の頂点間隔
uniform vec2 gap;

// 歪み補正前の出力位置を表すテクスチャ座標
out vec2 texcoord;

void main(void)
{
  // gl_VertexID と gl_InstanceID から三角形ストリップの格子座標を生成する。
  // CPU側に頂点配列を用意せず、既存のメッシュ描画方式をそのまま利用する。
  int x = gl_VertexID >> 1;
  int y = gl_InstanceID + 1 - (gl_VertexID & 1);
  vec2 position = vec2(x, y) * gap - 1.0;

  // 格子をクリッピング空間全体へ直接配置する。
  gl_Position = vec4(position, 0.0, 1.0);

  // [-1, 1] の頂点位置を、テクスチャ座標 [0, 1] へ変換する。
  texcoord = position * 0.5 + 0.5;
}
