#include "Mesh.h"

using namespace DirectX::SimpleMath;
using namespace std;

Mesh::~Mesh()
{
}

void Mesh::TestBox()
{
    vector<Vector3> positions;
    vector<Vector3> colors;
    vector<Vector3> normals;
    vector<Vector2> texcoords; // 텍스춰 좌표

    const float scale = 0.7f;

    // 윗면
    positions.push_back(Vector3(-1.0f, 1.0f, -1.0f) * scale);
    positions.push_back(Vector3(-1.0f, 1.0f, 1.0f) * scale);
    positions.push_back(Vector3(1.0f, 1.0f, 1.0f) * scale);
    //positions.push_back(Vector3(1.0f, 1.0f, -1.0f) * scale);
    colors.push_back(Vector3(1.0f, 0.0f, 0.0f));
    colors.push_back(Vector3(1.0f, 0.0f, 0.0f));
    colors.push_back(Vector3(1.0f, 0.0f, 0.0f));
    //colors.push_back(Vector3(1.0f, 0.0f, 0.0f));
    normals.push_back(Vector3(0.0f, 1.0f, 0.0f));
    normals.push_back(Vector3(0.0f, 1.0f, 0.0f));
    normals.push_back(Vector3(0.0f, 1.0f, 0.0f));
    //normals.push_back(Vector3(0.0f, 1.0f, 0.0f));
    texcoords.push_back(Vector2(0.0f, 0.0f));
    texcoords.push_back(Vector2(1.0f, 0.0f));
    texcoords.push_back(Vector2(1.0f, 1.0f));
    //texcoords.push_back(Vector2(0.0f, 1.0f));

    // 아랫면
    //positions.push_back(Vector3(-1.0f, -1.0f, -1.0f) * scale);
    //positions.push_back(Vector3(1.0f, -1.0f, -1.0f) * scale);
    //positions.push_back(Vector3(1.0f, -1.0f, 1.0f) * scale);
    //positions.push_back(Vector3(-1.0f, -1.0f, 1.0f) * scale);
    //colors.push_back(Vector3(0.0f, 1.0f, 0.0f));
    //colors.push_back(Vector3(0.0f, 1.0f, 0.0f));
    //colors.push_back(Vector3(0.0f, 1.0f, 0.0f));
    //colors.push_back(Vector3(0.0f, 1.0f, 0.0f));
    //normals.push_back(Vector3(0.0f, -1.0f, 0.0f));
    //normals.push_back(Vector3(0.0f, -1.0f, 0.0f));
    //normals.push_back(Vector3(0.0f, -1.0f, 0.0f));
    //normals.push_back(Vector3(0.0f, -1.0f, 0.0f));
    //texcoords.push_back(Vector2(0.0f, 0.0f));
    //texcoords.push_back(Vector2(1.0f, 0.0f));
    //texcoords.push_back(Vector2(1.0f, 1.0f));
    //texcoords.push_back(Vector2(0.0f, 1.0f));

    //// 앞면
    //positions.push_back(Vector3(-1.0f, -1.0f, -1.0f) * scale);
    //positions.push_back(Vector3(-1.0f, 1.0f, -1.0f) * scale);
    //positions.push_back(Vector3(1.0f, 1.0f, -1.0f) * scale);
    //positions.push_back(Vector3(1.0f, -1.0f, -1.0f) * scale);
    //colors.push_back(Vector3(0.0f, 0.0f, 1.0f));
    //colors.push_back(Vector3(0.0f, 0.0f, 1.0f));
    //colors.push_back(Vector3(0.0f, 0.0f, 1.0f));
    //colors.push_back(Vector3(0.0f, 0.0f, 1.0f));
    //normals.push_back(Vector3(0.0f, 0.0f, -1.0f));
    //normals.push_back(Vector3(0.0f, 0.0f, -1.0f));
    //normals.push_back(Vector3(0.0f, 0.0f, -1.0f));
    //normals.push_back(Vector3(0.0f, 0.0f, -1.0f));
    //texcoords.push_back(Vector2(0.0f, 0.0f));
    //texcoords.push_back(Vector2(1.0f, 0.0f));
    //texcoords.push_back(Vector2(1.0f, 1.0f));
    //texcoords.push_back(Vector2(0.0f, 1.0f));

    //// 뒷면
    //positions.push_back(Vector3(-1.0f, -1.0f, 1.0f) * scale);
    //positions.push_back(Vector3(1.0f, -1.0f, 1.0f) * scale);
    //positions.push_back(Vector3(1.0f, 1.0f, 1.0f) * scale);
    //positions.push_back(Vector3(-1.0f, 1.0f, 1.0f) * scale);
    //colors.push_back(Vector3(0.0f, 1.0f, 1.0f));
    //colors.push_back(Vector3(0.0f, 1.0f, 1.0f));
    //colors.push_back(Vector3(0.0f, 1.0f, 1.0f));
    //colors.push_back(Vector3(0.0f, 1.0f, 1.0f));
    //normals.push_back(Vector3(0.0f, 0.0f, 1.0f));
    //normals.push_back(Vector3(0.0f, 0.0f, 1.0f));
    //normals.push_back(Vector3(0.0f, 0.0f, 1.0f));
    //normals.push_back(Vector3(0.0f, 0.0f, 1.0f));
    //texcoords.push_back(Vector2(0.0f, 0.0f));
    //texcoords.push_back(Vector2(1.0f, 0.0f));
    //texcoords.push_back(Vector2(1.0f, 1.0f));
    //texcoords.push_back(Vector2(0.0f, 1.0f));

    //// 왼쪽
    //positions.push_back(Vector3(-1.0f, -1.0f, 1.0f) * scale);
    //positions.push_back(Vector3(-1.0f, 1.0f, 1.0f) * scale);
    //positions.push_back(Vector3(-1.0f, 1.0f, -1.0f) * scale);
    //positions.push_back(Vector3(-1.0f, -1.0f, -1.0f) * scale);
    //colors.push_back(Vector3(1.0f, 1.0f, 0.0f));
    //colors.push_back(Vector3(1.0f, 1.0f, 0.0f));
    //colors.push_back(Vector3(1.0f, 1.0f, 0.0f));
    //colors.push_back(Vector3(1.0f, 1.0f, 0.0f));
    //normals.push_back(Vector3(-1.0f, 0.0f, 0.0f));
    //normals.push_back(Vector3(-1.0f, 0.0f, 0.0f));
    //normals.push_back(Vector3(-1.0f, 0.0f, 0.0f));
    //normals.push_back(Vector3(-1.0f, 0.0f, 0.0f));
    //texcoords.push_back(Vector2(0.0f, 0.0f));
    //texcoords.push_back(Vector2(1.0f, 0.0f));
    //texcoords.push_back(Vector2(1.0f, 1.0f));
    //texcoords.push_back(Vector2(0.0f, 1.0f));

    //// 오른쪽
    //positions.push_back(Vector3(1.0f, -1.0f, 1.0f) * scale);
    //positions.push_back(Vector3(1.0f, -1.0f, -1.0f) * scale);
    //positions.push_back(Vector3(1.0f, 1.0f, -1.0f) * scale);
    //positions.push_back(Vector3(1.0f, 1.0f, 1.0f) * scale);
    //colors.push_back(Vector3(1.0f, 0.0f, 1.0f));
    //colors.push_back(Vector3(1.0f, 0.0f, 1.0f));
    //colors.push_back(Vector3(1.0f, 0.0f, 1.0f));
    //colors.push_back(Vector3(1.0f, 0.0f, 1.0f));
    //normals.push_back(Vector3(1.0f, 0.0f, 0.0f));
    //normals.push_back(Vector3(1.0f, 0.0f, 0.0f));
    //normals.push_back(Vector3(1.0f, 0.0f, 0.0f));
    //normals.push_back(Vector3(1.0f, 0.0f, 0.0f));
    //texcoords.push_back(Vector2(0.0f, 0.0f));
    //texcoords.push_back(Vector2(1.0f, 0.0f));
    //texcoords.push_back(Vector2(1.0f, 1.0f));
    //texcoords.push_back(Vector2(0.0f, 1.0f));

    for (size_t i = 0; i < positions.size(); i++) 
    {
        Vertex v;
        v.Position = positions[i];
        v.Normal = normals[i];
        v.TexCoord = texcoords[i];
        Vertices.push_back(v);
    }

    Indices = 
    {
        0,  1,  2,//  0,  2,  3,  // 윗면
        //4,  5,  6,  4,  6,  7,  // 아랫면
        //8,  9,  10, 8,  10, 11, // 앞면
        //12, 13, 14, 12, 14, 15, // 뒷면
        //16, 17, 18, 16, 18, 19, // 왼쪽
        //20, 21, 22, 20, 22, 23  // 오른쪽
    };
}
