#pragma once

#include <memory>
#include <list>

#include <directxtk/SimpleMath.h>

class CpuRenderPipeline;

struct Triangle
{
    // Ŭ���� �뵵�� ������� ������ ������ ����ü
    DirectX::SimpleMath::Vector3 v0;
    DirectX::SimpleMath::Vector3 v1;
    DirectX::SimpleMath::Vector3 v2;
};

enum class EPlaceFromPlane
{
    None = 0,
    Inside, //normal ����
    Outside,//normal �ݴ����
    Middle  //��鿡 ����
};

class CpuRasterizer
{
public:
    // �ﰢ���� �ϳ��� �׸��� �Լ�
    // Rasterize!
    static void DrawIndexedTriangle(const size_t startIndex);

private:
    // view space -> clip space ��ȯ
    static DirectX::SimpleMath::Vector3 worldToClip(const DirectX::SimpleMath::Vector3& pointWorld);

    static DirectX::SimpleMath::Vector3 worldToView(const DirectX::SimpleMath::Vector3& pointWorld);

    static DirectX::SimpleMath::Vector3 viewToClip(const DirectX::SimpleMath::Vector3& pointView);

    // clip space -> screen space ��ȯ
    static DirectX::SimpleMath::Vector2 clipToScreen(const DirectX::SimpleMath::Vector3& pointClip);

    static float edgeFunction(const DirectX::SimpleMath::Vector2& v0, const DirectX::SimpleMath::Vector2& v1, const DirectX::SimpleMath::Vector2& point);

    // ���� ���� ����
    static float intersectPlaneAndVertex(const DirectX::SimpleMath::Vector4& plane, const DirectX::SimpleMath::Vector3& point);

    // ���� �ﰢ���� ����
    static EPlaceFromPlane intersectPlaneAndTriangle(const DirectX::SimpleMath::Vector4& plane, const struct Triangle& triangle);

    // ���� ������ ����
    static bool intersectPlaneAndLine(DirectX::SimpleMath::Vector3& outIntersectPoint, 
        const DirectX::SimpleMath::Vector4& plane, const DirectX::SimpleMath::Vector3& pointA, const DirectX::SimpleMath::Vector3& pointB);

    static void clipTriangle(std::list<struct Triangle>& triangles);

    static std::list<struct Triangle> splitTriangle(const DirectX::SimpleMath::Vector4& plane, const struct Triangle& triangle);

    static EPlaceFromPlane findVertexPlace(const float distance);
};

