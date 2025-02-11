#include "Transform.h"

using namespace DirectX;

Transform::Transform() :
    position(0, 0, 0),
    pitchYawRoll(0, 0, 0),
    scale(1, 1, 1),
    dirty(false)
{
    XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
}

void Transform::SetPosition(float x, float y, float z)
{
    position.x = x;
    position.y = y;
    position.z = z;
    dirty = true;
}

void Transform::SetRotation(float p, float y, float r)
{
    pitchYawRoll.x = p;
    pitchYawRoll.y = y;
    pitchYawRoll.z = r;
    dirty = true;
}

void Transform::SetScale(float x, float y, float z)
{
    scale.x = x;
    scale.y = y;
    scale.z = z;
    dirty = true;
}

void Transform::MoveAbsolute(float x, float y, float z)
{
    XMStoreFloat3(&position, XMLoadFloat3(&position) + XMVectorSet(x, y, z, 0.0f));
}

void Transform::MoveRelative(float x, float y, float z)
{
    XMStoreFloat3(&position, XMLoadFloat3(&position) + XMVectorSet(x, y, z, 0.0f));
}

void Transform::Rotate(float p, float y, float r)
{
    XMStoreFloat3(&pitchYawRoll, XMLoadFloat3(&pitchYawRoll) + XMVectorSet(p, y, r, 0.0f));
}

void Transform::Scale(float x, float y, float z)
{
    XMStoreFloat3(&scale, XMLoadFloat3(&scale) + XMVectorSet(x, y, z, 0.0f));
}

DirectX::XMFLOAT3 Transform::GetPosition()
{
    return position;
}

DirectX::XMFLOAT3 Transform::GetRotation()
{
    return pitchYawRoll;
}

DirectX::XMFLOAT3 Transform::GetScale()
{
    return scale;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
    if (dirty) {
        // make translation, rotation, and scale matrices
        XMMATRIX trMatrix = XMMatrixTranslation(position.x, position.y, position.z);
        XMMATRIX rotMatrix = XMMatrixRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);
        XMMATRIX scMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);

        // world matrix as result of transformation
        XMStoreFloat4x4(&worldMatrix, scMatrix * rotMatrix * trMatrix);

        dirty = false;
    }

    return worldMatrix;
}
