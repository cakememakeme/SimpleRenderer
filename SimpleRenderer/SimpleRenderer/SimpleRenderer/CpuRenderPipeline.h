#pragma once

#include <directxtk/SimpleMath.h>

#include <vector>
#include <memory>

#include "CpuRasterizer.h"

enum class ELightType
{
    Directional = 0,
    Point,
    Spot,
};

class Object;
class Mesh;
class Light;

// CPU�� ������ ���� ����������, @todo. Ÿ�ϸ޸𸮸� �־ ����� ���� ������������ �����ϴ� �͵� �غ��� �� ��
class CpuRenderPipeline : public std::enable_shared_from_this<CpuRenderPipeline>
{
    friend class CpuRasterizer;

private:
    // ���� �ܼ�ȭ�� ���� �޽� ���� ���⿡ ����д�
    // @todo. instancing, ���۸����� ����(= ����)
    std::vector<std::shared_ptr<Mesh>> meshes;

public:
    CpuRenderPipeline();

    bool Initialize(const int bufferWidth, const int bufferHeight);

    void Reset();

    void SetObjects(std::shared_ptr<std::vector<std::shared_ptr<Object>>> receivedObjects);

    void SetLightType(const ELightType lightType);

    void SetClippingPlane(const float leftClip, const float rightClip, const float topClip, const float bottomClip);

    std::shared_ptr<std::vector<DirectX::SimpleMath::Vector4>> Process();

private:
    void drawMeshes();
    void copyToBuffer(const Mesh& mesh);
};

