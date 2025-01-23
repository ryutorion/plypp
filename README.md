# plypp
C++20でのシンプルなPLYローダー

[The PLY File Format](https://sites.cc.gatech.edu/projects/large_models/ply.html)を参考に，C++20を前提として実装したヘッダオンリーのPLYローダーです．  
[The Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/)にあるモデルでテストをしています．

## 使い方

具体的な使い方は[example.cpp](blob/main/example.cpp)を参考にしてください．

plypp.hで完結しているので，plypp.hを適当な場所に置いてインクルードすれば使えます．

### PLYのロード

plypp::load()にファイルパスを渡すと，std::optionalに入ったPLYPolygonが返されます．

```cpp
std::optional<plypp::PLYPolygon> result = plypp::load("path to the PLY file");
if(!result)
{
	cerr << "Failed to load the PLY file." << endl;
	return 1;
}
plypp::PLYPolygon& polygon = result.value();
```

### 要素の取得

PLYに含まれるポリゴンは，要素の集合です．それぞれの要素には名前があり，PLYPolygon::containsElement()に名前を渡すことで，必要な要素があるかを確認できます．

そして，PLYPolygon::getElement()に名前を渡すことで要素を取得できます．

```cpp
if(!polygon.containsElement("vertex"))
{
	cerr << "The PLY file does not contain the vertex element." << endl;
	return 1;
}

plypp::PLYElement& vertexElement = polygon.getElement("vertex");
```

その要素がいくつ存在するかは，PLYElement::size()で取得できます．

### プロパティの取得とアクセス

要素はプロパティの集合です．プロパティには，PLYValuePropertyとPLYListPropertyがあり，それぞれ整数または浮動小数点型のデータを保持しています．

プロパティのデータ型として使えるのは次の型です．

* int8_t
* uint8_t
* int16_t
* uint16_t
* int32_t
* uint32_t
* float
* double

#### PLYValuePropertyの取得

PLYValuePropertyは，プロパティ1つに付き1つの値を持ちます．

特定の名前，特定の型を持ったPLYValuePropertyを持っているかどうかは，PLYElement::containsValueProperty()を使ってチェックできます．

```cpp
plypp::PLYElement& vertexElement = polygon.getElement("vertex");
if(!vertexElement.containsValueProperty<float>("x"))
{
	cerr << "The vertex element does not contain the x property." << endl;
	return 1;
}
```

実際に取得するには，PLYElement::getValueProperty()を呼び出します．

```cpp
plypp::PLYElement& vertexElement = polygon.getElement("vertex");
PLYValueProperty<float>& propertyX = vertexElement.getValueProperty<float>("x");
```

#### PLYValuePropertyへのアクセス

N番目の要素のプロパティの値にアクセスする場合，若干直観に反するかもしれませんが，プロパティに対して
インデックスでアクセスします．

例として，各頂点の位置情報を取り出す処理を示します．

```cpp
struct Vertex
{
    float x;
    float y;
    float z;
};

plypp::PLYElement& vertexElement = polygon.getElement("vertex");
PLYValueProperty<float>& propertyX = vertexElement.getValueProperty<float>("x");
PLYValueProperty<float>& propertyY = vertexElement.getValueProperty<float>("y");
PLYValueProperty<float>& propertyZ = vertexElement.getValueProperty<float>("z");

std::vector<Vertex> vertices(vertexElement.size());
for(size_t i = 0; i < vertices.size(); ++i>)
{
    vertices[i].x = propertyX[i];
    vertices[i].y = propertyY[i];
    vertices[i].z = propertyZ[i];
}
```

#### PLYListPropertyの取得

PLYListPropertyは，プロパティ1つに対して複数の値をstd::vectorを使って保持しています．

特定の名前，特定の型のPLYListPropertyを持っているかどうかは，
PLYElement::containsListProperty()を使ってチェックできます．

```cpp
plypp::PLYElement& faceElement = polygon.getElement("face");
if(!faceElement.containsListProperty<int32_t>("vertex_indices"))
{
	cerr << "The face element does not contain the vertex_indices property." << endl;
	return 1;
}
```

実際に取得するには，PLYElement::getListProperty()を呼び出します．

```cpp
plypp::PLYElement& faceElement = polygon.getElement("face");
plypp::PLYListProperty<int32_t>& propertyIndices = faceElement.getListProperty<int32_t>("vertex_indices");
```

また，PLYListPropertyには，全データ数を表すPLYListProperty::totalSize()や，最小/最大の数を表すminSize()/maxSize()もあります．

#### PLYListPropertyへのアクセス

基本的なアクセス方法はPLYValuePropertyと同じく，プロパティ経由でN番目の要素にアクセスします．  
ただし，返されるのがstd::vectorで，std::vectorのサイズは固定とは限りません．

例えば，三角形を表す3頂点のデータのこともあれば，四角形を表す4頂点のこともあるかもしれないということです．

次の例では，三角形の頂点のインデックスのみが入っている前提で，全データを1つのstd::vectorに格納しています．

```cpp
plypp::PLYElement& faceElement = polygon.getElement("face");
plypp::PLYListProperty<int32_t>& propertyIndices = faceElement.getListProperty<int32_t>("vertex_indices");
std::vector<int32_t> indices;
indices.reserve(propertyIndices.totalSize());
for(size_t i = 0; i < faceElement.size(); ++i)
{
	auto& face = propertyIndices[i];
	if(face.size() != 3)
	{
		cerr << "The face element contains a face that is not a triangle." << endl;
		return 1;
	}
	indices.insert(indices.end(), face.begin(), face.end());
}
```