#pragma once

#include <Windows.h>
#include <directxtk/SimpleMath.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include "Light.h"
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace DirectX::PackedVector;

struct ShaderConstants
{
    DirectX::SimpleMath::Matrix worldMat;
    DirectX::SimpleMath::Matrix worldMatInv;

    Material material;
    Light light;
    int lightType = 0;
};

struct VsInput
{
    // CPU���� GPU�� ������ �����ʹ� ���� ���� ���� ������
    // Vector4 ��ſ� Vector3 ���
    DirectX::SimpleMath::Vector3 Position;
    DirectX::SimpleMath::Vector3 normal;
    //DirectX::SimpleMath::Vector3 color;
    DirectX::SimpleMath::Vector2 uv;
};

struct VsOutput
{
    DirectX::SimpleMath::Vector3 Position;
    DirectX::SimpleMath::Vector3 normal;
    //DirectX::SimpleMath::Vector3 color;
    DirectX::SimpleMath::Vector2 uv;
};

struct PsInput
{
    DirectX::SimpleMath::Vector3 Position;
    DirectX::SimpleMath::Vector3 normal;
    //DirectX::SimpleMath::Vector3 color;
    DirectX::SimpleMath::Vector2 uv;
};

// GPU���� ���������� ����ϴ� �޸𸮶�� �����սô�.

ShaderConstants g_constants; // ���̴� ���
std::vector<size_t> g_indexBuffer;
std::vector<DirectX::SimpleMath::Vector4> g_displayBuffer;
std::vector<float> g_depthBuffer;
std::vector<DirectX::SimpleMath::Vector3> g_vertexBuffer;
std::vector<DirectX::SimpleMath::Vector3> g_normalBuffer;
std::vector<DirectX::SimpleMath::Vector3> g_colorBuffer;
std::vector<DirectX::SimpleMath::Vector2> g_uvBuffer;

// ~GPU���� ���������� ����ϴ� �޸𸮶�� �����սô�.

// gui���� ���� ������ �Ķ���� ���

Light g_light;
// Clockwise�� �ո�
bool g_cullBackface;
// ������(ortho) vs ����(perspective)����
bool g_bUsePerspectiveProjection;
// ���� ȭ���� �Ÿ� (���� ����)
float g_distEyeToScreen;
float g_viewDistanceCulling = 10.0f;
// ���� ����ϴ� ���� (0: directional, 1: point, 2: spot)
int g_lightType;
int g_width;
int g_height;
float g_leftClip;
float g_rightClip;
float g_topClip;
float g_bottomClip;
float g_nearClip;

// ~gui���� ���� ������ �Ķ���� ���

namespace CpuShader
{

float Saturate(float x)
{
    return std::max(0.0f, std::min(1.0f, x));
}

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    return Saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

Vector3 BlinnPhong(Vector3 lightStrength, Vector3 lightVec, Vector3 normal, Vector3 toEye, Material mat, Vector2 uv)
{
    Vector3 halfway = toEye + lightVec;
    halfway.Normalize();
    Vector3 specular = mat.Specular * pow(std::max(halfway.Dot(normal), 0.0f), mat.Shininess);

    Vector3 irradiance = mat.diffuseTex ? (mat.diffuseTex->SampleLinear(uv) + specular) : mat.Diffuse + specular;
    irradiance *= lightStrength;
    return mat.Ambient + irradiance;
}

Vector3 ComputeDirectionalLight(Light L, Material mat, Vector3 normal, Vector3 toEye, Vector2 uv)
{
    Vector3 lightVec = -L.Direction;

    float ndotl = std::max(lightVec.Dot(normal), 0.0f);
    Vector3 lightStrength = L.Strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat, uv);
}

Vector3 ComputePointLight(Light L, Material mat, Vector3 pos, Vector3 normal, Vector3 toEye, Vector2 uv)
{
    Vector3 lightVec = L.Position - pos;

    // ���̵��� �������� ��������� �Ÿ� ���
    float d = lightVec.Length();

    // �ʹ� �ָ� ������ ������� ����
    if (d > L.FallOffEnd)
        return Vector3(0.0f);

    lightVec /= d;

    Vector3 norm = lightVec;
    norm.Normalize();

    float ndotl = std::max(lightVec.Dot(normal), 0.0f);
    Vector3 lightStrength = L.Strength * ndotl;

    float att = CalcAttenuation(d, L.FallOffStart, L.FallOffEnd);
    lightStrength *= att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat, uv);
}

Vector3 ComputeSpotLight(Light L, Material mat, Vector3 pos, Vector3 normal, Vector3 toEye, Vector2 uv)
{
    Vector3 lightVec = L.Position - pos;

    // ���̵��� �������� ��������� �Ÿ� ���
    float d = lightVec.Length();

    // �ʹ� �ָ� ������ ������� ����
    if (d > L.FallOffEnd)
        return Vector3(0.0f);

    lightVec /= d;

    float ndotl = std::max(lightVec.Dot(normal), 0.0f);
    Vector3 lightStrength = L.Strength * ndotl;

    float att = CalcAttenuation(d, L.FallOffStart, L.FallOffEnd);
    lightStrength *= att;

    float spotFactor = pow(std::max(-lightVec.Dot(L.Direction), 0.0f), L.SpotPower);
    lightStrength *= spotFactor;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat, uv);
}

VsOutput CpuVertexShader(VsInput vsInput)
{
    VsOutput vsOutput;

    // �������� 1.0f �߰�
    Vector4 point = Vector4(vsInput.Position.x, vsInput.Position.y, vsInput.Position.z, 1.0f);

    Vector4 worldMat = Vector4::Transform(point, g_constants.worldMat);
    vsOutput.Position = Vector3(worldMat.x, worldMat.y, worldMat.z);

    // �������� 0.0f �߰�
    Vector4 normal = Vector4(vsInput.normal.x, vsInput.normal.y, vsInput.normal.z, 0.0f);
    normal = Vector4::Transform(normal, g_constants.worldMatInv);
    normal.Normalize();
    vsOutput.normal = Vector3(normal.x, normal.y, normal.z);

    // ���� ��ȯ�� ���� �����̽������� ����
    // view space ��ȯ�� ��ǥ��� ���������� �����ϱⰡ ���� �ʾƼ�, ����� �ϱⰡ ������
    // view space -> clip space -> screen space�� ��ȯ�� ProjectWorldToRaster()���� �ѹ��� �����Ѵ�
    // @todo. local -> world -> view ��ȯ�� ����(ī�޶� ���� �߰����� ��� ������ ���� �ȴٸ�)
    return vsOutput;
}

Vector4 CpuPixelShader(const PsInput psInput)
{
    Vector3 eye = Vector3(0.0f, 0.0f, -1.0f); // -distEyeToScreen
    Vector3 toEye = eye - psInput.Position;
    toEye.Normalize();

    Vector3 color;

    // üũ����, ������
    /*if (!g_constants.material.diffuseTex)
    {
        float Size = 0.1f;
        Vector2 Pos = DirectX::XMVectorFloor(psInput.uv / Size);
        float PatternMask = (static_cast<int>(Pos.x) + (static_cast<int>(Pos.y) % 2)) % 2;
        return PatternMask * Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    }*/

    if (g_constants.lightType == 0)
    {
        color = ComputeDirectionalLight(g_constants.light, g_constants.material, psInput.normal, toEye, psInput.uv);
    }
    else if (g_constants.lightType == 1)
    {
        color = ComputePointLight(g_constants.light, g_constants.material, psInput.Position, psInput.normal, toEye, psInput.uv);
    }
    else
    {
        color = ComputeSpotLight(g_constants.light, g_constants.material, psInput.Position, psInput.normal, toEye, psInput.uv);
    }

    return Vector4(color.x, color.y, color.z, 1.0f);
}

}