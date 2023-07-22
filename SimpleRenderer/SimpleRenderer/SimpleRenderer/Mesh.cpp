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
    vector<Vector2> texcoords; // �ؽ��� ��ǥ

    const float scale = 0.7f;

    // ����
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

    // �Ʒ���
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

    //// �ո�
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

    //// �޸�
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

    //// ����
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

    //// ������
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
        0,  1,  2,//  0,  2,  3,  // ����
        //4,  5,  6,  4,  6,  7,  // �Ʒ���
        //8,  9,  10, 8,  10, 11, // �ո�
        //12, 13, 14, 12, 14, 15, // �޸�
        //16, 17, 18, 16, 18, 19, // ����
        //20, 21, 22, 20, 22, 23  // ������
    };
}
