#pragma once

#include <directxtk/SimpleMath.h>

struct Transformation 
{
    DirectX::SimpleMath::Vector3 scale = DirectX::SimpleMath::Vector3(1.0f);
    DirectX::SimpleMath::Vector3 translation = DirectX::SimpleMath::Vector3(0.0f);
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
};

// ���� ���� �ö� �� �ִ� ��ü�� �ش�˴ϴ�
// @todo. �̸� ���� �ʿ� -> ex) RenderObject / PrimitiveObject ��
class Object
{
public:
    virtual ~Object();

    // ��� ���ؽ��� �������� ����Ǵ� ��ȯ(Transformations)
    Transformation Transform;
};

