///
/// キャプチャデバイスの構成データクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date January 23, 2025
///
#include "Preference.h"

//
// キャプチャデバイスの構成データのデフォルトコンストラクタ
//
Preference::Preference()
  : Preference{ "Default", "orthographic.vert", "normal.frag" }
{
}

//
// シェーダのソースファイルを指定するときに使う
// キャプチャデバイスの構成データのコンストラクタ
//
Preference::Preference(const std::string& description,
  const std::string& vert, const std::string& frag,
  const Intrinsics& intrinsics)
  : description{ description }
  , source{ vert, frag }
  , undistortionSource{ "undistortion.vert", "undistortion.frag" }
  , intrinsics{ intrinsics }
  , shader{ nullptr }
  , undistortionShader{ nullptr }
{
  // 構成に undistortion ノードがない場合も、標準補正シェーダを使用できるようにする。
}

//
// 構成ファイルを読み込むときに使う
// キャプチャデバイスの構成データのコンストラクタ
//
Preference::Preference(const picojson::object& object)
  : undistortionSource{ "undistortion.vert", "undistortion.frag" }
  , intrinsics{ object }
  , shader{ nullptr }
  , undistortionShader{ nullptr }
{
  // 説明の文字列
  getString(object, "description", description);

  // 展開用シェーダのファイル名
  getString(object, "shader", source);

  // undistortion ノードがあれば、既定の補正シェーダ名を構成値で置き換える。
  getString(object, "undistortion", undistortionSource);
}

//
// 構成データのデストラクタ
//
Preference::~Preference()
{
  // static メンバにしている std::map の中身を先に消去しておく
  shaderList.clear();
  undistortionShaderList.clear();
}

//
// シェーダをビルドする
//
void Preference::buildShader()
{
  // ソースファイル名をつないでシェーダを検索するキーを作る
  const auto key{ source[0] + "\t" + source[1] };

  // キーがシェーダリストに無ければシェーダを構築して追加する
  shader = &shaderList.try_emplace(key, source[0], source[1]).first->second;

  // 同じ組み合わせの補正シェーダを構成ごとに重複コンパイルしないよう、
  // 2つのファイル名をキーにして共有キャッシュを検索する。
  const auto undistortionKey{
    undistortionSource[0] + "\t" + undistortionSource[1] };

  // 未作成の場合だけコンパイルし、この構成が参照するシェーダを記録する。
  undistortionShader = &undistortionShaderList.try_emplace(undistortionKey,
    undistortionSource[0], undistortionSource[1]).first->second;
}

//
// 構成を JSON オブジェクトに格納する
//
void Preference::setPreference(picojson::object& object) const
{
  // 説明の文字列
  setString(object, "description", description);

  // キャプチャデバイスのレンズの縦横の画角
  setValue(object, "fov", intrinsics.fov);

  // キャプチャデバイスのレンズの中心（主点）位置
  setValue(object, "center", intrinsics.center);

  // キャプチャデバイスの解像度
  setValue(object, "resolution", intrinsics.size);

  // キャプチャデバイスのフレームレート
  setValue(object, "fps", intrinsics.fps);

  // 展開用シェーダのファイル名
  setString(object, "shader", source);

  // 保存後も同じ補正シェーダを再構築できるよう、2つのソース名を書き出す。
  setString(object, "undistortion", undistortionSource);
}

// すべての構成のシェーダーのリスト
std::map<std::string, Expand> Preference::shaderList;
std::map<std::string, Expand> Preference::undistortionShaderList;
