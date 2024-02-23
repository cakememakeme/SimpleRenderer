#pragma once

#include <memory>
#include <list>
#include <algorithm>
#include <array>
#include <iostream>

#include "CpuShader.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace DirectX::PackedVector;

namespace CpuRasterizer
{

// 클리핑 용도로 만들어진 간단한 폴리곤 구조체
struct Triangle
{
    DirectX::SimpleMath::Vector3 v0;
    DirectX::SimpleMath::Vector3 v1;
    DirectX::SimpleMath::Vector3 v2;
};

enum class EPlaceFromPlane
{
    None = 0,
    Inside, //normal 방향
    Outside,//normal 반대방향
    Middle  //평면에 접함
};

bool fEqual(const float a, const float b)
{
    return fabs(a - b) < std::numeric_limits<float>::epsilon();
}

//public:
    // 삼각형을 하나만 그리는 함수
    // Rasterize!
    void DrawIndexedTriangle(const size_t startIndex);

//private:
    // view space -> clip space 변환
    DirectX::SimpleMath::Vector3 worldToClip(const DirectX::SimpleMath::Vector3& pointWorld);

    DirectX::SimpleMath::Vector3 worldToView(const DirectX::SimpleMath::Vector3& pointWorld);

    DirectX::SimpleMath::Vector3 viewToClip(const DirectX::SimpleMath::Vector3& pointView);

    // clip space -> screen space 변환
    DirectX::SimpleMath::Vector2 clipToScreen(const DirectX::SimpleMath::Vector3& pointClip);

    float edgeFunction(const DirectX::SimpleMath::Vector2& v0, const DirectX::SimpleMath::Vector2& v1, const DirectX::SimpleMath::Vector2& point);

    // 평면과 점의 교차
    float intersectPlaneAndVertex(const DirectX::SimpleMath::Vector4& plane, const DirectX::SimpleMath::Vector3& point);

    // 평면과 삼각형의 교차
    EPlaceFromPlane intersectPlaneAndTriangle(const DirectX::SimpleMath::Vector4& plane, const struct Triangle& triangle);

    // 평면과 선분의 교차
    bool intersectPlaneAndLine(DirectX::SimpleMath::Vector3& outIntersectPoint,
        const DirectX::SimpleMath::Vector4& plane, const DirectX::SimpleMath::Vector3& pointA, const DirectX::SimpleMath::Vector3& pointB);

    void clipTriangle(std::list<struct Triangle>& triangles);

    std::list<struct Triangle> splitTriangle(const DirectX::SimpleMath::Vector4& plane, const struct Triangle& triangle);

    EPlaceFromPlane findVertexPlace(const float distance);

//////////// Implementation ////////////

Vector3 worldToView(const Vector3& pointWorld)
{
    // 월드 좌표계의 원점이 우리가 보는 화면의 중심이라고 가정(world->view transformation 생략)
    return pointWorld;
}

DirectX::SimpleMath::Vector3 viewToClip(const Vector3& pointView)
{
    // 정투영(Orthographic projection)
    Vector3 pointProj = Vector3(pointView.x, pointView.y, pointView.z);

    // 원근투영(Perspective projection)
    // 원근투영도 행렬로 표현할 수 있습니다.
    if (g_bUsePerspectiveProjection)
    {
        const float scale = g_distEyeToScreen / (g_distEyeToScreen + pointView.z);
        pointProj = Vector3(pointView.x * scale, pointView.y * scale, pointView.z);
    }

    const float aspect = static_cast<float>(g_width) / g_height;
    const Vector3 pointNDC = Vector3(pointProj.x / aspect, pointProj.y, pointProj.z);

    return pointNDC;
}

Vector3 worldToClip(const Vector3& pointWorld)
{
    // @todo. 카메라 기능 개선이 들어가면 VertexShader로 이동
    Vector3 pointView = worldToView(pointWorld);
    Vector3 pointClip = viewToClip(pointView);
    return pointClip;
}

DirectX::SimpleMath::Vector2 clipToScreen(const DirectX::SimpleMath::Vector3& pointClip)
{
    // 레스터 좌표의 범위 [-0.5, width - 1 + 0.5] x [-0.5, height - 1 + 0.5]
    const float xScale = 2.0f / g_width;
    const float yScale = 2.0f / g_height;

    // NDC -> 레스터 화면 좌표계(screen space)
    // 주의: y좌표 상하반전
    return Vector2((pointClip.x + 1.0f) / xScale - 0.5f, (1.0f - pointClip.y) / yScale - 0.5f);
}

float edgeFunction(const Vector2& v0, const Vector2& v1, const Vector2& point)
{
    const Vector2 a = v1 - v0;
    const Vector2 b = point - v0;
    return a.x * b.y - a.y * b.x;
}

float intersectPlaneAndVertex(const DirectX::SimpleMath::Vector4& plane, const DirectX::SimpleMath::Vector3& point)
{
    const float mul = plane.x * point.x + plane.y * point.y + plane.z * point.z;
    const float dist = mul + plane.w;
    return dist;
}

EPlaceFromPlane intersectPlaneAndTriangle(const DirectX::SimpleMath::Vector4& plane, const struct Triangle& triangle)
{
    const struct Triangle& tri = triangle;

    // 용책 64p, 평면 구축 참고
    const float i0 = intersectPlaneAndVertex(plane, tri.v0);
    const float i1 = intersectPlaneAndVertex(plane, tri.v1);
    const float i2 = intersectPlaneAndVertex(plane, tri.v2);

    // 세 점이 모두 평면 위에 있을 때, inside를 먼저 판단하면 문제가 생깁니다
    // outside

    if ((fEqual(i0, 0.0f) || i0 < 0.0f) && (fEqual(i1, 0.0f) || i1 < 0.0f) && (fEqual(i2, 0.0f) || i2 < 0.0f))
    {
        return EPlaceFromPlane::Outside;
    }

    // inside
    if ((fEqual(i0, 0.0f) || i0 > 0.0f) && (fEqual(i1, 0.0f) || i1 > 0.0f) && (fEqual(i2, 0.0f) || i2 >= 0.0f))
    {
        return EPlaceFromPlane::Inside;
    }

    // split
    // 교차했다는 판단은 열린 구간(open interval)이어야 합니다. 
    // 닫힌 구간이면 점이 평면에 접할 때 문제가 생깁니다
    return EPlaceFromPlane::Middle;
}

EPlaceFromPlane findVertexPlace(const float distance)
{
    if (std::fabs(distance) < std::numeric_limits<float>::epsilon())
    {
        return EPlaceFromPlane::Middle;
    }
    else if (distance < 0.0f)
    {
        return EPlaceFromPlane::Outside;
    }

    return EPlaceFromPlane::Inside;
}

bool intersectPlaneAndLine(Vector3& outIntersectPoint,
    const Vector4& plane, const Vector3& pointA, const Vector3& pointB)
{
    const Vector3 n(plane.x, plane.y, plane.z);
    const Vector3 t(pointB - pointA);
    const float dist = n.Dot(t);
    const EPlaceFromPlane place = findVertexPlace(dist);
    if (place == EPlaceFromPlane::Middle)
    {
        // 두 벡터가 수직하는 경우 (dot(n, t) == 0)
        return false;
    }
    if (std::fabs(dist) < std::numeric_limits<float>::epsilon())
    {
        // 미세하게 split
        return false;
    }

    const float aDot = pointA.Dot(n);
    const float bDot = pointB.Dot(n);
    const float scale = (-plane.w - aDot) / (bDot - aDot);

    // 마찬가지로, *교차했다는 판단*은 열린 구간(open interval)이어야 합니다. 
    // 닫힌 구간이면 점이 평면에 접할 때 문제가 생깁니다
    // 0.0f < scale < 1.0f
    if (fEqual(scale, 0.0f) || scale < 0.0f)
    {
        return false;
    }
    if (fEqual(scale, 0.0f) || scale > 1.0f)
    {
        return false;
    }

    // 평면과 직선의 교차점 구하기
    outIntersectPoint = pointA + (scale * (pointB - pointA));
    return true;
}

std::list<struct Triangle> splitTriangle(const DirectX::SimpleMath::Vector4& plane, const Triangle& triangle)
{
    // 주의: 버텍스의 시계방향 순서(CW)는 유지되나, 좌하단 -> 좌상단 -> 우상단의 순서는 깨지게 됩니다
    // 한 칸씩 밀립니다
    const struct Triangle& tri = triangle;
    std::array<Vector3, 3> tris = { tri.v0, tri.v1, tri.v2 };
    std::vector<Vector3> splitTri_inside;
    splitTri_inside.reserve(4);
    std::vector<Vector3> splitTri_outside;
    splitTri_outside.reserve(4);

    switch (findVertexPlace(intersectPlaneAndVertex(plane, tris[0])))
    {
    case EPlaceFromPlane::Inside:
    {
        splitTri_inside.push_back(tris[0]);
    }
    break;
    case EPlaceFromPlane::Outside:
    {
        splitTri_outside.push_back(tris[0]);
    }
    break;
    case EPlaceFromPlane::Middle:
    {
        splitTri_inside.push_back(tris[0]);
        splitTri_outside.push_back(tris[0]);
    }
    break;
    default:
    {

    }
    }

    for (size_t i = 1; i <= tris.size(); ++i)
    {
        const size_t currIdx = i % 3;
        const Vector3& prevVert = tris[i - 1];
        const Vector3& currVert = tris[currIdx];

        const float dist = intersectPlaneAndVertex(plane, currVert);
        const EPlaceFromPlane currPlace = findVertexPlace(dist);
        if (currPlace == EPlaceFromPlane::Middle)
        {
            splitTri_inside.push_back(currVert);
            splitTri_outside.push_back(currVert);
        }
        else
        {
            Vector3 intersectPoint = Vector3::Zero;
            if (intersectPlaneAndLine(intersectPoint, plane, prevVert, currVert))
            {
                if (currPlace == EPlaceFromPlane::Inside)
                {
                    splitTri_outside.push_back(intersectPoint);
                    splitTri_inside.push_back(intersectPoint);
                    if (currIdx != 0)
                    {
                        splitTri_inside.push_back(currVert);
                    }
                }
                if (currPlace == EPlaceFromPlane::Outside)
                {
                    splitTri_inside.push_back(intersectPoint);
                    splitTri_outside.push_back(intersectPoint);
                    if (currIdx != 0)
                    {
                        splitTri_outside.push_back(currVert);
                    }
                }
            }
            else
            {
                if (currPlace == EPlaceFromPlane::Inside)
                {
                    splitTri_inside.push_back(currVert);
                }
                if (currPlace == EPlaceFromPlane::Outside)
                {
                    splitTri_outside.push_back(currVert);
                }
            }
        }
    }

    /*
    if(splitTri_inside.size() < 3)
    {
        const EPlaceFromPlane top = intersectPlaneAndTriangle(plane, tri);
        int bp = 0;
    }
    if (splitTri_inside.size() > 4 || splitTri_outside.size() > 5)
    {
        const EPlaceFromPlane top = intersectPlaneAndTriangle(plane, tri);
        int bp = 0;
    }
    std::cout << splitTri_inside.size() << ' ' << splitTri_outside.size() << '\n';
    */
    std::list<struct Triangle> insideTris;
    if (splitTri_inside.size() == splitTri_outside.size())
    {
        Triangle insideTri;
        insideTri.v0 = splitTri_inside[0];
        insideTri.v1 = splitTri_inside[1];
        insideTri.v2 = splitTri_inside[2];
        insideTris.push_back(insideTri);

        /*Triangle outsideTri;
        outsideTri.v0 = splitTri_outside[0];
        outsideTri.v1 = splitTri_outside[1];
        outsideTri.v2 = splitTri_outside[2];*/
    }
    else if (splitTri_inside.size() > splitTri_outside.size())
    {
        Triangle insideTri0;
        insideTri0.v0 = splitTri_inside[0];
        insideTri0.v1 = splitTri_inside[1];
        insideTri0.v2 = splitTri_inside[2];
        insideTris.push_back(insideTri0);

        Triangle insideTri1;
        insideTri1.v0 = splitTri_inside[0];
        insideTri1.v1 = splitTri_inside[2];
        insideTri1.v2 = splitTri_inside[3];
        insideTris.push_back(insideTri1);

        /*Triangle outsideTri;
        outsideTri.v0 = splitTri_outside[0];
        outsideTri.v1 = splitTri_outside[1];
        outsideTri.v2 = splitTri_outside[2];*/
    }
    else
    {
        /*Triangle outsideTri0;
        outsideTri0.v0 = splitTri_outside[0];
        outsideTri0.v1 = splitTri_outside[1];
        outsideTri0.v2 = splitTri_outside[2];

        Triangle outsideTri1;
        outsideTri1.v0 = splitTri_outside[0];
        outsideTri1.v1 = splitTri_outside[2];
        outsideTri1.v2 = splitTri_outside[3];*/

        Triangle insideTri;
        insideTri.v0 = splitTri_inside[0];
        insideTri.v1 = splitTri_inside[1];
        insideTri.v2 = splitTri_inside[2];
        insideTris.push_back(insideTri);
    }

    return insideTris;
}

void clipTriangle(std::list<struct Triangle>& triangles)
{
    using namespace std;

    list<struct Triangle>::iterator eraseMark = triangles.end();
    for (auto triangle = triangles.begin(); triangle != triangles.end(); ++triangle)
    {
        if (eraseMark != triangles.end())
        {
            triangles.erase(eraseMark);
            eraseMark = triangles.end();
        }

        const struct Triangle& tri = *triangle;

        // far plane 제외하고 클리핑을 수행
        const Vector4 nearClippingPlane = Vector4(0.0f, 0.0f, g_distEyeToScreen, -g_nearClip);
        const EPlaceFromPlane nearPlane = intersectPlaneAndTriangle(nearClippingPlane, tri);
        if (nearPlane != EPlaceFromPlane::Inside)
        {
            eraseMark = triangle;
            if (nearPlane == EPlaceFromPlane::Middle)
            {
                triangles.splice(triangles.end(), splitTriangle(nearClippingPlane, tri));
            }
            continue;
        }

        const Vector4 leftClippingPlane = Vector4(1.0f, 0.0f, 0.0f, g_leftClip);
        const EPlaceFromPlane left = intersectPlaneAndTriangle(leftClippingPlane, tri);
        if (left != EPlaceFromPlane::Inside)
        {
            eraseMark = triangle;
            if (left == EPlaceFromPlane::Middle)
            {
                triangles.splice(triangles.end(), splitTriangle(leftClippingPlane, tri));
            }
            continue;
        }

        const Vector4 rightClippingPlane = Vector4(-1.0f, 0.0f, 0.0f, g_rightClip);
        const EPlaceFromPlane right = intersectPlaneAndTriangle(rightClippingPlane, tri);
        if (right != EPlaceFromPlane::Inside)
        {
            eraseMark = triangle;
            if (right == EPlaceFromPlane::Middle)
            {
                triangles.splice(triangles.end(), splitTriangle(rightClippingPlane, tri));
            }
            continue;
        }
        const Vector4 topClippingPlane = Vector4(0.0f, -1.0f, 0.0f, g_topClip);
        const EPlaceFromPlane top = intersectPlaneAndTriangle(topClippingPlane, tri);
        if (top != EPlaceFromPlane::Inside)
        {
            eraseMark = triangle;
            if (top == EPlaceFromPlane::Middle)
            {
                triangles.splice(triangles.end(), splitTriangle(topClippingPlane, tri));
            }
            continue;
        }
        const Vector4 bottomClippingPlane = Vector4(0.0f, 1.0f, 0.0f, g_bottomClip);
        const EPlaceFromPlane bottom = intersectPlaneAndTriangle(bottomClippingPlane, tri);
        if (bottom != EPlaceFromPlane::Inside)
        {
            eraseMark = triangle;
            if (bottom == EPlaceFromPlane::Middle)
            {
                triangles.splice(triangles.end(), splitTriangle(bottomClippingPlane, tri));
            }
            continue;
        }
    }

    if (eraseMark != triangles.end())
    {
        triangles.erase(eraseMark);
        eraseMark = triangles.end();
    }
}

void DrawIndexedTriangle(const size_t startIndex)
{
    // backface culling
    const size_t i0 = g_indexBuffer[startIndex];
    const size_t i1 = g_indexBuffer[startIndex + 1];
    const size_t i2 = g_indexBuffer[startIndex + 2];

    const Vector3 v0_clip = worldToClip(g_vertexBuffer[i0]);
    const Vector3 v1_clip = worldToClip(g_vertexBuffer[i1]);
    const Vector3 v2_clip = worldToClip(g_vertexBuffer[i2]);

    const Vector2 v0_screen = clipToScreen(v0_clip);
    const Vector2 v1_screen = clipToScreen(v1_clip);
    const Vector2 v2_screen = clipToScreen(v2_clip);

    // 삼각형 전체 넓이의 두 배, 음수일 수도 있음
    const float area = edgeFunction(v0_screen, v1_screen, v2_screen);

    // 뒷면일 경우
    if (g_cullBackface && area < 0.0f)
    {
        return;
    }

    // clipping
    // 이터레이터 보장되는게 이놈밖에 없나...
    std::list<struct Triangle> triangles;
    triangles.push_back({ v0_clip, v1_clip, v2_clip });
    clipTriangle(triangles);

    /*const auto& c0 = g_colorBuffer[i0];
    const auto& c1 = g_colorBuffer[i1];
    const auto& c2 = g_colorBuffer[i2];*/

    const Vector2& uv0 = g_uvBuffer[i0];
    const Vector2& uv1 = g_uvBuffer[i1];
    const Vector2& uv2 = g_uvBuffer[i2];

    // draw internal
    for (const auto& triangle : triangles)
    {
        const Vector2 clipV0_screen = clipToScreen(triangle.v0);
        const Vector2 clipV1_screen = clipToScreen(triangle.v1);
        const Vector2 clipV2_screen = clipToScreen(triangle.v2);

        const Vector2 bMin = Vector2::Min(Vector2::Min(clipV0_screen, clipV1_screen), clipV2_screen);
        const Vector2 bMax = Vector2::Max(Vector2::Max(clipV0_screen, clipV1_screen), clipV2_screen);

        const auto xMin = size_t(std::clamp(std::floor(bMin.x), 0.0f, float(g_width - 1)));
        const auto yMin = size_t(std::clamp(std::floor(bMin.y), 0.0f, float(g_height - 1)));
        const auto xMax = size_t(std::clamp(std::ceil(bMax.x), 0.0f, float(g_width - 1)));
        const auto yMax = size_t(std::clamp(std::ceil(bMax.y), 0.0f, float(g_height - 1)));

        // Primitive 보간 후 Pixel(Fragment) Shader로 넘긴다
        for (size_t y = yMin; y <= yMax; y++)
        {
            for (size_t x = xMin; x <= xMax; x++)
            {
                const Vector2 point = Vector2(float(x), float(y));

                // 위에서 계산한 삼각형 전체 넓이 area를 재사용
                float w0 = edgeFunction(v1_screen, v2_screen, point) / area;
                float w1 = edgeFunction(v2_screen, v0_screen, point) / area;
                float w2 = edgeFunction(v0_screen, v1_screen, point) / area;

                if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
                {
                    // Perspective-Correct Interpolation
                    // OpenGL 구현
                    // https://stackoverflow.com/questions/24441631/how-exactly-does-opengl-do-perspectively-correct-linear-interpolation

                    const float z0 = g_vertexBuffer[i0].z + g_distEyeToScreen;
                    const float z1 = g_vertexBuffer[i1].z + g_distEyeToScreen;
                    const float z2 = g_vertexBuffer[i2].z + g_distEyeToScreen;

                    const Vector3 p0 = g_vertexBuffer[i0];
                    const Vector3 p1 = g_vertexBuffer[i1];
                    const Vector3 p2 = g_vertexBuffer[i2];

                    // 뒷면일 경우에도 쉐이딩이 가능하도록 normal을 반대로
                    /*const Vector3 n0 = area < 0.0f ? -g_normalBuffer[i0] : g_normalBuffer[i0];
                    const Vector3 n1 = area < 0.0f ? -g_normalBuffer[i1] : g_normalBuffer[i1];
                    const Vector3 n2 = area < 0.0f ? -g_normalBuffer[i2] : g_normalBuffer[i2];*/
                    const Vector3 n0 = g_normalBuffer[i0];
                    const Vector3 n1 = g_normalBuffer[i1];
                    const Vector3 n2 = g_normalBuffer[i2];

                    if (g_bUsePerspectiveProjection)
                    {
                        w0 /= z0;
                        w1 /= z1;
                        w2 /= z2;

                        const float wSum = w0 + w1 + w2;

                        w0 /= wSum;
                        w1 /= wSum;
                        w2 /= wSum;
                    }

                    const float depth = w0 * z0 + w1 * z1 + w2 * z2;
                    // const Vector3 color = w0 * c0 + w1 * c1 + w2 * c2;
                    const Vector2 uv = w0 * uv0 + w1 * uv1 + w2 * uv2;

                    if (depth < g_depthBuffer[x + g_width * y])
                    {
                        g_depthBuffer[x + g_width * y] = depth;

                        PsInput psInput;
                        psInput.Position = w0 * p0 + w1 * p1 + w2 * p2;
                        psInput.normal = w0 * n0 + w1 * n1 + w2 * n2;
                        // psInput.color = color;
                        psInput.uv = uv;

                        std::vector<Vector4>& buffer = g_displayBuffer;
                        buffer[x + g_width * y] = CpuShader::CpuPixelShader(psInput);
                    }
                }
            }
        }
    }
}

}