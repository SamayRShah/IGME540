#include "Transform.h"

using namespace DirectX;

Transform::Transform() :
    position(0, 0, 0),
    pitchYawRoll(0, 0, 0),
    scale(1, 1, 1),
    dirty(false),
    qRotation(XMQuaternionIdentity())
{
    XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
}

// getters
const DirectX::XMFLOAT3 Transform::GetPosition() const { return position; }
const DirectX::XMFLOAT3 Transform::GetRotation() const { return pitchYawRoll; }
const DirectX::XMFLOAT3 Transform::GetScale() const { return scale; }

// set positions & overloads
void Transform::SetPosition(float x, float y, float z)
{
    SetPosition(XMFLOAT3(x, y, z));
}
void Transform::SetPosition(const XMFLOAT3& pos) {
    this->position = pos;
    dirty = true;
}

// set rotation & overloads
void Transform::SetRotation(float p, float y, float r)
{
    SetRotation(XMFLOAT3(p, y, r));  
}
void Transform::SetRotation(const XMFLOAT3& rotation)
{
    pitchYawRoll = rotation;
    UpdateQuaternion();
}

// set scale & overloads
void Transform::SetScale(float x, float y, float z)
{
    SetScale(XMFLOAT3(x, y, z));
}
void Transform::SetScale(const XMFLOAT3& scale)
{
    this->scale = scale;
    dirty = true;
}

// Transforms & overloads
// Move Absolute
void Transform::MoveAbsolute(float x, float y, float z)
{
    MoveAbsolute(XMFLOAT3(x, y, z));
}
void Transform::MoveAbsolute(const DirectX::XMFLOAT3& offset)
{
    DirectX::XMStoreFloat3(&position, XMLoadFloat3(&position) + XMLoadFloat3(&offset));
    dirty = true;
}

// Move Relative
void Transform::MoveRelative(float x, float y, float z)
{
    MoveRelative(DirectX::XMFLOAT3(x, y, z));  
}
void Transform::MoveRelative(const DirectX::XMFLOAT3& offset)
{
    // TODO
    return;
}

// Rotate
void Transform::Rotate(float x, float y, float z)
{
    Rotate(DirectX::XMFLOAT3(x, y, z));
}
void Transform::Rotate(const DirectX::XMFLOAT3& rotation)
{
    DirectX::XMStoreFloat3(&pitchYawRoll, XMLoadFloat3(&pitchYawRoll) + XMLoadFloat3(&rotation));
    UpdateQuaternion();
}

// Scale
void Transform::Scale(float x, float y, float z)
{
    Scale(DirectX::XMFLOAT3(x, y, z)); 
}
void Transform::Scale(const DirectX::XMFLOAT3& scaleFactor)
{
    DirectX::XMStoreFloat3(&scale, XMLoadFloat3(&scale) * XMLoadFloat3(&scaleFactor));
    dirty = true;
}

// update quaternion
void Transform::UpdateQuaternion() {
    // load pitch yaw roll into xmvector and convert to radians
    XMVECTOR euler = XMLoadFloat3(&pitchYawRoll);
    euler *= (XM_PI/180.0f);

    // convert to quaternion
    qRotation = XMQuaternionRotationRollPitchYawFromVector(euler);
    dirty = true;
}

const DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
    if (dirty) {
        // make translation, rotation, and scale matrices
        XMMATRIX mTranslation = XMMatrixTranslation(position.x, position.y, position.z);
        XMMATRIX mRotation = XMMatrixRotationQuaternion(qRotation);
        XMMATRIX mScale = XMMatrixScaling(scale.x, scale.y, scale.z);
        XMMATRIX mWorld = mScale * mRotation * mTranslation;

        // world matrix as result of transformation
        XMStoreFloat4x4(&worldMatrix, mWorld);

        dirty = false;
    }

    return worldMatrix;
}
