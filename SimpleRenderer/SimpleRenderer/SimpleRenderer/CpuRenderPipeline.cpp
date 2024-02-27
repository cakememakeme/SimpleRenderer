#include "CpuRenderPipeline.h"

#include <iostream>
#include <algorithm>

#include "CpuRasterizer.h"
#include "CpuShader.h"

using namespace DirectX::SimpleMath;
using namespace std;

CpuRenderPipeline::CpuRenderPipeline()
{
    g_cullBackface = true;
    g_bUsePerspectiveProjection = true;
    g_distEyeToScreen = 1.0f;
}

bool CpuRenderPipeline::Initialize(const int bufferWidth, const int bufferHeight)
{
    g_width = bufferWidth;
    g_height = bufferHeight;

    const int bufferSize = g_width * g_height;

    // ��¿� ���� �ʱ�ȭ(@todo. swapchain ����...? �Ƹ� ������ ������)
    g_displayBuffer.resize(bufferSize);

    // ���� ���� �ʱ�ȭ
    g_depthBuffer.resize(bufferSize, 0.0f);

    return true;
}

void CpuRenderPipeline::Reset()
{
    std::fill(g_displayBuffer.begin(), g_displayBuffer.end(), Vector4(0.45f, 0.5f, 0.45f, 1.0f)); // ����� ���
    
    // ���� ���� �ʱ�ȭ
    g_depthBuffer.resize(g_displayBuffer.size());
    
    // ���̴� �ִ� 10.0f (-> ���� �Ÿ��� 10.0f ����)
    fill(g_depthBuffer.begin(), g_depthBuffer.end(), g_viewDistanceCulling);
}

void CpuRenderPipeline::SetObjects(const std::vector<std::shared_ptr<Object>>& receivedObjects)
{
    for (auto& object : receivedObjects)
    {
        shared_ptr<Mesh> mesh = dynamic_pointer_cast<Mesh>(object);
        if (mesh)
        {
            meshes.push_back(mesh);
        }
        shared_ptr<Light> light = dynamic_pointer_cast<Light>(object);
        if(light)
        { 
            g_light = *light;
        }
    }
}

void CpuRenderPipeline::SetLightType(const ELightType lightType)
{
    g_lightType = static_cast<int>(lightType);
}

void CpuRenderPipeline::SetClippingPlane(const float leftClip, const float rightClip, const float topClip, const float bottomClip, const float nearClip)
{
    g_leftClip = leftClip;
    g_rightClip = rightClip;
    g_topClip = topClip;
    g_bottomClip = bottomClip;
    g_nearClip = nearClip;
}

std::vector<DirectX::SimpleMath::Vector4> CpuRenderPipeline::Process()
{
    Reset();

    drawMeshes();

    return g_displayBuffer;
}

void CpuRenderPipeline::drawMeshes()
{
    if (meshes.empty())
    {
        std::cout << "invalid meshes." << std::endl;
        return;
    }

    // ���� ������������
    // 1. Input assembler
    // 2. Vertex shader
    // 3. Tessellation
    // 4. Geometry shader
    // 5. Rasterization
    // 6. Fragment shader
    // 7. Color blending
    // �������� �Ǿ� �ִ�(Vulkan tutorial ����)
    // ������ζ��, Vertex buffer/Index buffer�� �ִ� ���ؽ��� 3���� ���(Input assemble) 
    // ���� ������ŭ ���������ο� �¿� �����°� ������
    // �������� ���ؼ� ���������� �ľ� �����ϰԲ�, ���� ������ �ƴ� �޽� ������ ������ ������
    for (const auto mesh : meshes)
    {
        if (!mesh)
        {
            continue;
        }

        copyToBuffer(*mesh);
        
        for (size_t i = 0; i < g_vertexBuffer.size(); ++i)
        {
            VsInput vsInput;
            vsInput.Position = g_vertexBuffer[i];
            vsInput.normal = g_normalBuffer[i];
            // ���ؽ� transform�� ������ ��. vertex shader ���꿡 �ʿ��� ��� �Ѱ��ش�
            //vsInput.color = colorBuffer[i];
            //vsInput.uv = uvBuffer[i]; 

            // vertex shader �ܰ�
            VsOutput vsOutput = CpuShader::CpuVertexShader(vsInput);

            g_vertexBuffer[i] = vsOutput.Position;
            g_normalBuffer[i] = vsOutput.normal;
            //colrBuffer[i] = vsOutput.color;
            //uvBuffer[i] = vsOutput.uv;
        }

        // rasterize �ܰ�
        for (size_t i = 0; i < g_indexBuffer.size(); i += 3) // 3�� �� ��� ����, (Input assemble) ���� �̰� vertex shader���� ���� �̷����� �Ѵ�
        {
            CpuRasterizer::DrawIndexedTriangle(i);
        }
    }

    meshes.clear();
}

void CpuRenderPipeline::copyToBuffer(const Mesh& mesh)
{
    const Matrix translation = Matrix::CreateTranslation(mesh.Transform.translation);
    const Matrix rotation = Matrix::CreateRotationX(mesh.Transform.rotationX) *
                            Matrix::CreateRotationY(mesh.Transform.rotationY) *
                            Matrix::CreateRotationZ(mesh.Transform.rotationZ);
    const Matrix scale = Matrix::CreateScale(mesh.Transform.scale);

    g_constants.worldMat = scale * rotation * translation;
    g_constants.worldMatInv = g_constants.worldMat;
    g_constants.worldMatInv.Translation(Vector3::Zero);
    g_constants.worldMatInv.Invert().Transpose();

    g_constants.material = mesh.Material;
    g_constants.light = g_light;
    g_constants.lightType = g_lightType;

    vector<Vector3> vertexBuffer;
    vertexBuffer.reserve(mesh.Vertices.size());
    vector<Vector3> normalBuffer;
    normalBuffer.reserve(mesh.Vertices.size());
    vector<Vector2> uvBuffer;
    uvBuffer.reserve(mesh.Vertices.size());
    for (const auto& vertex : mesh.Vertices)
    {
        vertexBuffer.push_back(vertex.Position);
        normalBuffer.push_back(vertex.Normal);
        uvBuffer.push_back(vertex.TexCoord);
    }
    g_vertexBuffer = vertexBuffer;
    g_normalBuffer = normalBuffer;
    g_uvBuffer = uvBuffer;
    g_indexBuffer = mesh.Indices;
    //colorBuffer.resize(mesh.Vertices.size());
}
