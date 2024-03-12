#pragma once

#include <directxtk/SimpleMath.h>

#include <vector>
#include <memory>

enum class ELightType
{
    Directional = 0,
    Point,
    Spot,
};

class Object;
class Mesh;
class Light;

// CPU로 구현한 렌더 파이프라인
class CpuRenderPipeline : public std::enable_shared_from_this<CpuRenderPipeline>
{
private:
    // 로직 단순화를 위한 메시 모델을 여기에 묶어둔다
    // @todo. instancing, 버퍼만으로 연산(= 제거)
    std::vector<std::shared_ptr<Mesh>> meshes;

public:
    CpuRenderPipeline();

    bool Initialize(const int bufferWidth, const int bufferHeight);

    void Reset();

    void SetObjects(const std::vector<std::shared_ptr<Object>>& receivedObjects);

    void SetLightType(const ELightType lightType);

    void SetClippingPlane(const float leftClip, const float rightClip, const float topClip, const float bottomClip, const float nearClip);

    std::vector<DirectX::SimpleMath::Vector4> Process();

private:
    void drawMeshes();
    void copyToBuffer(const Mesh& mesh);
};

