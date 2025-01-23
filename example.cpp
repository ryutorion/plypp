#include <iostream>
#include <plypp.h>

using namespace std;

struct vertex
{
	float x;
	float y;
	float z;
};

int main()
{
	auto result = plypp::load("path to the PLY file");
	if(!result)
	{
		cerr << "Failed to load the PLY file." << endl;
		return 1;
	}
	else
	{
		cout << "Successfully loaded the PLY file." << endl;
	}

	auto& polygon = result.value();
	if(!polygon.containsElement("vertex"))
	{
		cerr << "The PLY file does not contain the vertex element." << endl;
		return 1;
	}

	if(!polygon.containsElement("face"))
	{
		cerr << "The PLY file does not contain the face element." << endl;
		return 1;
	}

	auto& vertexElement = polygon.getElement("vertex");
	if(!vertexElement.containsValueProperty<float>("x"))
	{
		cerr << "The vertex element does not contain the x property." << endl;
		return 1;
	}

	if(!vertexElement.containsValueProperty<float>("y"))
	{
		cerr << "The vertex element does not contain the y property." << endl;
		return 1;
	}

	if(!vertexElement.containsValueProperty<float>("z"))
	{
		cerr << "The vertex element does not contain the z property." << endl;
		return 1;
	}

	auto& faceElement = polygon.getElement("face");
	if(!faceElement.containsListProperty<int32_t>("vertex_indices"))
	{
		cerr << "The face element does not contain the vertex_indices property." << endl;
		return 1;
	}

	std::vector<vertex> vertices(vertexElement.size());
	auto& propertyX = vertexElement.getValueProperty<float>("x");
	auto& propertyY = vertexElement.getValueProperty<float>("y");
	auto& propertyZ = vertexElement.getValueProperty<float>("z");

	for(size_t i = 0; i < vertexElement.size(); ++i)
	{
		vertices[i].x = propertyX[i];
		vertices[i].y = propertyY[i];
		vertices[i].z = propertyZ[i];
	}

	auto& propertyIndices = faceElement.getListProperty<int32_t>("vertex_indices");
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

	return 0;
}