#include "CpuRasterizer.h"

#include <algorithm>
#include <array>
#include <iostream>

#include "CpuShader.h"

// GPU���� ���������� ����ϴ� �޸𸮶�� �����սô�.
ShaderConstants g_constants; // ���̴� ���
std::vector<size_t> g_indexBuffer;
std::shared_ptr<std::vector<DirectX::SimpleMath::Vector4>> g_displayBuffer;
std::vector<float> g_depthBuffer;
std::vector<DirectX::SimpleMath::Vector3> g_vertexBuffer;
std::vector<DirectX::SimpleMath::Vector3> g_normalBuffer;
std::vector<DirectX::SimpleMath::Vector3> g_colorBuffer;
std::vector<DirectX::SimpleMath::Vector2> g_uvBuffer;
// ~GPU���� ���������� ����ϴ� �޸𸮶�� �����սô�.

// ��Ÿ ����� ģ����(ó���� ���� �����ߴ� ���� �������� ���� �޶�����, ���п� ���� �̻��������ϴ�...)
Light g_light;
// Clockwise�� �ո�
bool g_cullBackface;
// ������(ortho) vs ����(perspective)����
bool g_bUsePerspectiveProjection;
// ���� ȭ���� �Ÿ� (���� ����)
float g_distEyeToScreen;
// ���� ����ϴ� ���� (0: directional, 1: point, 2: spot)
int g_lightType;
int g_width;
int g_height;
float g_leftClip;
float g_rightClip;
float g_topClip;
float g_bottomClip;
// ~��Ÿ ����� ģ����

using namespace DirectX::SimpleMath;
using namespace std;

bool fEqual(const float a, const float b)
{
    return fabs(a - b) < numeric_limits<float>::epsilon();
}

void CpuRasterizer::DrawIndexedTriangle(const size_t startIndex)
{
    // backface culling
    const size_t i0 = g_indexBuffer[startIndex];
    const size_t i1 = g_indexBuffer[startIndex + 1];
    const size_t i2 = g_indexBuffer[startIndex + 2];

    const Vector3 rootV0 = worldToClip(g_vertexBuffer[i0]);
    const Vector3 rootV1 = worldToClip(g_vertexBuffer[i1]);
    const Vector3 rootV2 = worldToClip(g_vertexBuffer[i2]);

    const Vector2 rootV0_screen = clipToScreen(rootV0);
    const Vector2 rootV1_screen = clipToScreen(rootV1);
    const Vector2 rootV2_screen = clipToScreen(rootV2);

    // �ﰢ�� ��ü ������ �� ��, ������ ���� ����
    const float area = edgeFunction(rootV0_screen, rootV1_screen, rootV2_screen);

    // �޸��� ���
    if (g_cullBackface && area < 0.0f)
    {
        return;
    }

    // clipping
    // stl ����Ʈ�� ��屸���� ������ ���� ������
    // sparse array�� ���ڴ� �̰� ������ �� �� ���Ƽ� �׳� ����Ʈ�� ����߽��ϴ�
    std::list<struct Triangle> triangles;
    triangles.push_back({ rootV0, rootV1, rootV2 });
    clipTriangle(triangles);

    /*const auto& c0 = g_colorBuffer[i0];
    const auto& c1 = g_colorBuffer[i1];
    const auto& c2 = g_colorBuffer[i2];*/

    const auto& uv0 = g_uvBuffer[i0];
    const auto& uv1 = g_uvBuffer[i1];
    const auto& uv2 = g_uvBuffer[i2];

    // draw internal
    for (const auto& triangle : triangles)
    {
        const Vector2 clipV0_Screen = clipToScreen(triangle.v0);
        const Vector2 clipV1_Screen = clipToScreen(triangle.v1);
        const Vector2 clipV2_Screen = clipToScreen(triangle.v2);

        const Vector2 bMin = Vector2::Min(Vector2::Min(clipV0_Screen, clipV1_Screen), clipV2_Screen);
        const Vector2 bMax = Vector2::Max(Vector2::Max(clipV0_Screen, clipV1_Screen), clipV2_Screen);

        const auto xMin = size_t(std::clamp(std::floor(bMin.x), 0.0f, float(g_width - 1)));
        const auto yMin = size_t(std::clamp(std::floor(bMin.y), 0.0f, float(g_height - 1)));
        const auto xMax = size_t(std::clamp(std::ceil(bMax.x), 0.0f, float(g_width - 1)));
        const auto yMax = size_t(std::clamp(std::ceil(bMax.y), 0.0f, float(g_height - 1)));

        // Primitive ���� �� Pixel(Fragment) Shader�� �ѱ��
        for (size_t y = yMin; y <= yMax; y++) 
        {
            for (size_t x = xMin; x <= xMax; x++) 
            {
                const Vector2 point = Vector2(float(x), float(y));

                // ������ ����� �ﰢ�� ��ü ���� area�� ����
                float w0 = edgeFunction(rootV1_screen, rootV2_screen, point) / area;
                float w1 = edgeFunction(rootV2_screen, rootV0_screen, point) / area;
                float w2 = edgeFunction(rootV0_screen, rootV1_screen, point) / area;

                if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) 
                {
                    // Perspective-Correct Interpolation
                    // OpenGL ����
                    // https://stackoverflow.com/questions/24441631/how-exactly-does-opengl-do-perspectively-correct-linear-interpolation

                    const float z0 = g_vertexBuffer[i0].z + g_distEyeToScreen;
                    const float z1 = g_vertexBuffer[i1].z + g_distEyeToScreen;
                    const float z2 = g_vertexBuffer[i2].z + g_distEyeToScreen;

                    const Vector3 p0 = g_vertexBuffer[i0];
                    const Vector3 p1 = g_vertexBuffer[i1];
                    const Vector3 p2 = g_vertexBuffer[i2];

                    // �޸��� ��쿡�� ���̵��� �����ϵ��� normal�� �ݴ��
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

                        vector<Vector4>& buffer = *g_displayBuffer.get();
                        buffer[x + g_width * y] = CpuPixelShader(psInput);
                    }
                }
            }
        }
    }
}

Vector3 CpuRasterizer::worldToClip(const Vector3& pointWorld)
{
    // @todo. ī�޶� ��� ������ ���� VertexShader�� �̵�
    Vector3 pointView = worldToView(pointWorld);
    Vector3 pointClip = viewToClip(pointView);
    return pointClip;
}

DirectX::SimpleMath::Vector2 CpuRasterizer::clipToScreen(const DirectX::SimpleMath::Vector3& pointClip)
{
    // ������ ��ǥ�� ���� [-0.5, width - 1 + 0.5] x [-0.5, height - 1 + 0.5]
    const float xScale = 2.0f / g_width;
    const float yScale = 2.0f / g_height;

    // NDC -> ������ ȭ�� ��ǥ��(screen space)
    // ����: y��ǥ ���Ϲ���
    return Vector2((pointClip.x + 1.0f) / xScale - 0.5f, (1.0f - pointClip.y) / yScale - 0.5f);
}

float CpuRasterizer::edgeFunction(const Vector2& v0, const Vector2& v1, const Vector2& point)
{
    const Vector2 a = v1 - v0;
    const Vector2 b = point - v0;
    return a.x * b.y - a.y * b.x;
}

float CpuRasterizer::intersectPlaneAndVertex(const DirectX::SimpleMath::Vector4& plane, const DirectX::SimpleMath::Vector3& point)
{
    const float dist = plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w;
    return dist;
}

EPlaceFromPlane CpuRasterizer::intersectPlaneAndTriangle(const DirectX::SimpleMath::Vector4& plane, const struct Triangle& triangle)
{
    const struct Triangle& tri = triangle;

    // ��å 64p, ��� ���� ����
    const float i0 = intersectPlaneAndVertex(plane, tri.v0);
    const float i1 = intersectPlaneAndVertex(plane, tri.v1);
    const float i2 = intersectPlaneAndVertex(plane, tri.v2);
    
    // �� ���� ��� ��� ���� ���� ��, inside�� ���� �Ǵ��ϸ� ������ ����ϴ�
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
    // �����ߴٴ� �Ǵ��� ���� ����(open interval)�̾�� �մϴ�. 
    // ���� �����̸� ���� ��鿡 ���� �� ������ ����ϴ�
    return EPlaceFromPlane::Middle;
}

bool CpuRasterizer::intersectPlaneAndLine(Vector3& outIntersectPoint, 
    const Vector4& plane, const Vector3& pointA, const Vector3& pointB)
{
    const Vector3 n(plane.x, plane.y, plane.z);
    const Vector3 t(pointB - pointA);
    const float dist = n.Dot(t);
    const EPlaceFromPlane place = findVertexPlace(dist);
    if (place == EPlaceFromPlane::Middle)
    {
        // �� ���Ͱ� �����ϴ� ��� (dot(n, t) == 0)
        return false;
    }
    if (std::fabs(dist) < std::numeric_limits<float>::epsilon())
    {
        // �̼��ϰ� split
        return false;
    }

    const float aDot = pointA.Dot(n);
    const float bDot = pointB.Dot(n);
    const float scale = (-plane.w - aDot) / (bDot - aDot);

    // ����������, *�����ߴٴ� �Ǵ�*�� ���� ����(open interval)�̾�� �մϴ�. 
    // ���� �����̸� ���� ��鿡 ���� �� ������ ����ϴ�
    // 0.0f < scale < 1.0f
    if (fEqual(scale, 0.0f) || scale < 0.0f)
    {
        return false;
    }
    if (fEqual(scale, 0.0f) || scale > 1.0f)
    {
        return false;
    }

    // ���� ������ ������ ���ϱ�
    outIntersectPoint = pointA + (scale * (pointB - pointA));
    return true;
}

void CpuRasterizer::clipTriangle(std::list<struct Triangle>& triangles)
{
    std::list<struct Triangle>::iterator eraseMark = triangles.end();
    for (auto triangle = triangles.begin(); triangle != triangles.end(); ++triangle)
    {
        if (eraseMark != triangles.end())
        {
            triangles.erase(eraseMark);
            eraseMark = triangles.end();
        }

        const struct Triangle tri = *triangle;

        // far plane �����ϰ� Ŭ������ ����
        const Vector4 leftPlane = Vector4(1.0f, 0.0f, 0.0f, g_leftClip);
        const EPlaceFromPlane leftPlace = intersectPlaneAndTriangle(leftPlane, tri);
        if (leftPlace != EPlaceFromPlane::Inside)
        {
            eraseMark = triangle;
            if (leftPlace == EPlaceFromPlane::Middle)
            {
                triangles.splice(triangles.end(), splitTriangle(leftPlane, tri));
            }
            continue;
        }

        const Vector4 rightPlane = Vector4(-1.0f, 0.0f, 0.0f, g_rightClip);
        const EPlaceFromPlane rightPlace = intersectPlaneAndTriangle(rightPlane, tri);
        if (rightPlace != EPlaceFromPlane::Inside)
        {
            eraseMark = triangle;
            if (rightPlace == EPlaceFromPlane::Middle)
            {
                triangles.splice(triangles.end(), splitTriangle(rightPlane, tri));
            }
            continue;
        }
        const Vector4 topPlane = Vector4(0.0f, -1.0f, 0.0f, g_topClip);
        const EPlaceFromPlane topPlace = intersectPlaneAndTriangle(topPlane, tri);
        if (topPlace != EPlaceFromPlane::Inside)
        {
            eraseMark = triangle;
            if (topPlace == EPlaceFromPlane::Middle)
            {
                triangles.splice(triangles.end(), splitTriangle(topPlane, tri));
            }
            continue;
        }
        const Vector4 bottomPlane = Vector4(0.0f, 1.0f, 0.0f, g_bottomClip);
        const EPlaceFromPlane bottomPlace = intersectPlaneAndTriangle(bottomPlane, tri);
        if (bottomPlace != EPlaceFromPlane::Inside)
        {
            eraseMark = triangle;
            if (bottomPlace == EPlaceFromPlane::Middle)
            {
                triangles.splice(triangles.end(), splitTriangle(bottomPlane, tri));
            }
            continue;
        }
        // ����� Ŭ���ο� ��¦ �̽��� �־, ���� �����Դϴ�
        const Vector4 nearPlane = Vector4(0.0f, 0.0f, 1.0f, 1.0f);
        const EPlaceFromPlane nearPlace = intersectPlaneAndTriangle(nearPlane, tri);
        if (nearPlace != EPlaceFromPlane::Inside)
        {
            eraseMark = triangle;
            if (nearPlace == EPlaceFromPlane::Middle)
            {
                triangles.splice(triangles.end(), splitTriangle(nearPlane, tri));
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

list<struct Triangle> CpuRasterizer::splitTriangle(const DirectX::SimpleMath::Vector4& plane, const Triangle& triangle)
{
    // ����: ���ؽ��� �ð���� ����(CW)�� �����ǳ�, ���ϴ� -> �»�� -> ������ ������ ������ �˴ϴ�
    // �� ĭ�� �и��ϴ�
    const struct Triangle& tri = triangle;
    array<Vector3, 3> tris = { tri.v0, tri.v1, tri.v2 };
    vector<Vector3> splitTri_inside;
    splitTri_inside.reserve(4);
    vector<Vector3> splitTri_outside;
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

    for(size_t i = 1; i <= tris.size(); ++i)
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
        const EPlaceFromPlane topPlace = intersectPlaneAndTriangle(plane, tri);
        int bp = 0;
    }
    if (splitTri_inside.size() > 4 || splitTri_outside.size() > 5)
    {
        const EPlaceFromPlane topPlace = intersectPlaneAndTriangle(plane, tri);
        int bp = 0;
    }
    std::cout << splitTri_inside.size() << ' ' << splitTri_outside.size() << '\n';
    */
    list<struct Triangle> insideTris;
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

EPlaceFromPlane CpuRasterizer::findVertexPlace(const float distance)
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

Vector3 CpuRasterizer::worldToView(const Vector3& pointWorld)
{
    // ���� ��ǥ���� ������ �츮�� ���� ȭ���� �߽��̶�� ����(world->view transformation ����, ���� ���� ����)
    return pointWorld;
}

DirectX::SimpleMath::Vector3 CpuRasterizer::viewToClip(const Vector3& pointView)
{
    // ������(Orthographic projection)
    Vector3 pointProj = Vector3(pointView.x, pointView.y, pointView.z);

    // ��������(Perspective projection)
    // ���������� ��ķ� ǥ���� �� �ֽ��ϴ�.
    if (g_bUsePerspectiveProjection)
    {
        const float scale = g_distEyeToScreen / (g_distEyeToScreen + pointView.z);
        pointProj = Vector3(pointView.x * scale, pointView.y * scale, pointView.z);
    }

    const float aspect = static_cast<float>(g_width) / g_height;
    const Vector3 pointNDC = Vector3(pointProj.x / aspect, pointProj.y, pointProj.z);

    return pointNDC;
}
