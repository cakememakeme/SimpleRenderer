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
    g_width = bufferWidth <= 0 ? 1 : bufferWidth;
    g_height = bufferHeight <= 0 ? 1 : bufferHeight;

    const int bufferSize = g_width * g_height;

    // 출력용 버퍼 초기화
    g_displayBuffer.resize(bufferSize);

    // 깊이 버퍼 초기화
    g_depthBuffer.resize(bufferSize, 0.0f);

    return true;
}

void CpuRenderPipeline::Reset()
{
    std::fill(g_displayBuffer.begin(), g_displayBuffer.end(), Vector4{ 0.45f, 0.5f, 0.45f, 1.0f }); // 국방색 배경
    
    // 깊이 버퍼 초기화
    g_depthBuffer.resize(g_displayBuffer.size());
    
    // 깊이는 최대 10.0f (-> 렌더 거리도 10.0f 이하)
    fill(g_depthBuffer.begin(), g_depthBuffer.end(), g_viewDistanceCulling);
}

void CpuRenderPipeline::SetObjects(const std::vector<std::shared_ptr<Object>>& inObjects)
{
    for (auto& object : inObjects)
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

    // 실제 파이프라인은
    // 1. Input assembler
    // 2. Vertex shader
    // 3. Tessellation
    // 4. Geometry shader
    // 5. Rasterization
    // 6. Fragment shader
    // 7. Color blending
    // 기준으로 되어 있다(Vulkan tutorial 기준)
    // 원래대로라면, Vertex buffer/Index buffer에 있는 버텍스를 3개로 묶어서(Input assemble) 
    // 묶인 단위만큼 파이프라인에 태워 보내는게 맞으나
    // 렌더링에 대해서 직관적으로 파악 가능하게끔, 버퍼 단위가 아닌 메시 단위로 나눠서 보낸다
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
            // 버텍스 transform에 무관한 값. vertex shader 연산에 필요한 경우 넘겨준다
            //vsInput.color = colorBuffer[i];
            //vsInput.uv = uvBuffer[i]; 

            // vertex shader 단계
            VsOutput vsOutput = CpuShader::CpuVertexShader(vsInput);

            g_vertexBuffer[i] = vsOutput.Position;
            g_normalBuffer[i] = vsOutput.normal;
            //colrBuffer[i] = vsOutput.color;
            //uvBuffer[i] = vsOutput.uv;
        }

        // rasterize 단계
        for (size_t i = 0; i < g_indexBuffer.size(); i += 3) // 3개 씩 묶어서 전달, (Input assemble) 원래 이게 vertex shader보다 먼저 이뤄져야 한다
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
