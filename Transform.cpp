#include "Transform.h"

using namespace DirectX;

Transform::Transform() :
    position(0, 0, 0),
    pitchYawRoll(0, 0, 0),
    scale(1, 1, 1),
    up(0, 1, 0),
    right(1, 0, 0),
    forward(0, 0, 1),
    bMatricesDirty(false),
    bVectorsDirty(false),
    qRotation(XMQuaternionIdentity())
{
    XMStoreFloat4x4(&mWorld, XMMatrixIdentity());
    XMStoreFloat4x4(&mWorldInverseTranspose, XMMatrixIdentity());
}

// getters
const DirectX::XMFLOAT3 Transform::GetPosition() const { return position; }
const DirectX::XMFLOAT3 Transform::GetRotation() const { return pitchYawRoll; }
const DirectX::XMFLOAT3 Transform::GetScale() const { return scale; }
const DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
    UpdateMatrices();
    return mWorld;
}
const DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix() {
    UpdateMatrices();
    return mWorldInverseTranspose;
}

// set positions & overloads
void Transform::SetPosition(float x, float y, float z)
{
    SetPosition(XMFLOAT3(x, y, z));
}
void Transform::SetPosition(const XMFLOAT3& pos) {
    this->position = pos;
    bMatricesDirty = true;
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
    bMatricesDirty = true;
}

// Transforms & overloads
// Move Absolute
void Transform::MoveAbsolute(float x, float y, float z)
{
    MoveAbsolute(XMFLOAT3(x, y, z));
}
void Transform::MoveAbsolute(const DirectX::XMFLOAT3& offset)
{
    UpdateVectors();
    DirectX::XMStoreFloat3(&position, XMLoadFloat3(&position) + XMLoadFloat3(&offset));
    bMatricesDirty = true;
}

// Move Relative
void Transform::MoveRelative(float x, float y, float z)
{
    MoveRelative(DirectX::XMFLOAT3(x, y, z));  
}
void Transform::MoveRelative(const DirectX::XMFLOAT3& offset)
{
    XMVECTOR dir = XMVector3Rotate(XMLoadFloat3(&offset), qRotation);
    XMStoreFloat3(&position, XMLoadFloat3(&position) + dir);
    bMatricesDirty = true;
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
    bMatricesDirty = true;
}

// update quaternion
void Transform::UpdateQuaternion() {
    // load pitch yaw roll into xmvector and convert to radians
    XMVECTOR euler = XMLoadFloat3(&pitchYawRoll);
    euler *= (XM_PI/180.0f);

    // convert to quaternion
    qRotation = XMQuaternionRotationRollPitchYawFromVector(euler);
    bMatricesDirty = true;
    bVectorsDirty = true;
}

void Transform::UpdateMatrices() {
    if (!bMatricesDirty) return;

    // make translation, rotation, and scale matrices
    XMMATRIX mT = XMMatrixTranslation(position.x, position.y, position.z);
    XMMATRIX mR = XMMatrixRotationQuaternion(qRotation);
    XMMATRIX mS = XMMatrixScaling(scale.x, scale.y, scale.z);
    XMMATRIX mW = mS * mR * mT;

    // world matrix as result of transformation
    XMStoreFloat4x4(&mWorld, mW);
    XMStoreFloat4x4(&mWorldInverseTranspose, XMMatrixInverse(0, XMMatrixTranspose(mW)));

    bMatricesDirty = false;
}

void Transform::UpdateVectors() {
    if (!bVectorsDirty) return;

    XMStoreFloat3(&up, XMVector3Rotate(XMVectorSet(0, 1, 0, 0), qRotation));
    XMStoreFloat3(&right, XMVector3Rotate(XMVectorSet(1, 0, 0, 0), qRotation));
    XMStoreFloat3(&forward, XMVector3Rotate(XMVectorSet(0, 0, 1, 0), qRotation));
    bVectorsDirty = false;
}
