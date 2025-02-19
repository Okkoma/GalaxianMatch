#pragma once

#include <Urho3D/Math/Matrix2.h>
#include <Urho3D/Math/Matrix3.h>
#include <Urho3D/Math/Matrix3x4.h>

#ifdef URHO3D_SSE
#include <emmintrin.h>
#endif

using namespace Urho3D;

/// 2x3 matrix for scene node transform calculations In Urho2D
class Matrix2x3
{
public:
    /// Construct an identity matrix.
    Matrix2x3()
       :m00_(1.0f),
        m01_(0.0f),
        m02_(0.0f),
        m10_(0.0f),
        m11_(1.0f),
        m12_(0.0f)
    {
    }

    /// Copy-construct from another matrix.
    Matrix2x3(const Matrix2x3& matrix)
       :m00_(matrix.m00_),
        m01_(matrix.m01_),
        m02_(matrix.m02_),
        m10_(matrix.m10_),
        m11_(matrix.m11_),
        m12_(matrix.m12_)
    {
    }

    /// Copy-construct from a 2x2 matrix and set the extra elements to identity.
    Matrix2x3(const Matrix2& matrix) :
        m00_(matrix.m00_),
        m01_(matrix.m01_),
        m02_(0.0f),
        m10_(matrix.m10_),
        m11_(matrix.m11_),
        m12_(0.0f)
    {
    }

    /// Copy-construct from a 3x3 matrix which is assumed to contain no projection.
    Matrix2x3(const Matrix3& matrix)
       :m00_(matrix.m00_),
        m01_(matrix.m01_),
        m02_(matrix.m02_),
        m10_(matrix.m10_),
        m11_(matrix.m11_),
        m12_(matrix.m12_)
    {
    }
    /// Copy-construct from a 3x4 matrix which is assumed to contain no projection.
    Matrix2x3(const Matrix3x4& matrix)
       :m00_(matrix.m00_),
        m01_(matrix.m01_),
        m02_(matrix.m03_), // position.x
        m10_(matrix.m10_),
        m11_(matrix.m11_),
        m12_(matrix.m13_)  // position.y
    {
    }
    /// Construct from values.
    Matrix2x3(float v00, float v01, float v02,
              float v10, float v11, float v12) :
        m00_(v00),
        m01_(v01),
        m02_(v02),
        m10_(v10),
        m11_(v11),
        m12_(v12)
    {
    }

    /// Construct from a float array.
    explicit Matrix2x3(const float* data)
       :m00_(data[0]),
        m01_(data[1]),
        m02_(data[2]),
        m10_(data[4]),
        m11_(data[5]),
        m12_(data[6])
    {
    }

    /// Construct from translation, rotation and uniform scale.
    Matrix2x3(const Vector2& translation, float rotation, float scale)
    {
        rotation *= M_DEGTORAD;
        float sinAngle = sinf(rotation);
        float cosAngle = cosf(rotation);
        SetRotation(Matrix2(cosAngle, -sinAngle, sinAngle, cosAngle)*scale);
        SetTranslation(translation);
    }

    /// Construct from translation, rotation and nonuniform scale.
    Matrix2x3(const Vector2& translation, float rotation, const Vector2& scale)
    {
        rotation *= M_DEGTORAD;
        float sinAngle = sinf(rotation);
        float cosAngle = cosf(rotation);
        SetRotation(Matrix2(cosAngle, -sinAngle, sinAngle, cosAngle).Scaled(scale));
        SetTranslation(translation);
    }

    /// Assign from another matrix.
    Matrix2x3& operator =(const Matrix2x3& rhs)
    {
        m00_ = rhs.m00_;
        m01_ = rhs.m01_;
        m02_ = rhs.m02_;
        m10_ = rhs.m10_;
        m11_ = rhs.m11_;
        m12_ = rhs.m12_;
        return *this;
    }

    /// Assign from a 2x2 matrix and set the extra elements to identity.
    Matrix2x3& operator =(const Matrix2& rhs)
    {
        m00_ = rhs.m00_;
        m01_ = rhs.m01_;
        m02_ = 0.0;
        m10_ = rhs.m10_;
        m11_ = rhs.m11_;
        m12_ = 0.0;
        return *this;
    }

    /// Assign from a 3x3 matrix which is assumed to contain no projection.
    Matrix2x3& operator =(const Matrix3& rhs)
    {
        m00_ = rhs.m00_;
        m01_ = rhs.m01_;
        m02_ = rhs.m02_;
        m10_ = rhs.m10_;
        m11_ = rhs.m11_;
        m12_ = rhs.m12_;
        return *this;
    }

    /// Test for equality with another matrix without epsilon.
    bool operator ==(const Matrix2x3& rhs) const
    {
        const float* leftData = Data();
        const float* rightData = rhs.Data();

        for (unsigned i = 0; i < 6; ++i)
        {
            if (leftData[i] != rightData[i])
                return false;
        }

        return true;
    }

    /// Test for inequality with another matrix without epsilon.
    bool operator !=(const Matrix2x3& rhs) const { return !(*this == rhs); }

    /// Multiply a Vector2 which is assumed to represent position2D.
    Vector2 operator *(const Vector2& rhs) const
    {
        return Vector2(
            (m00_ * rhs.x_ + m01_ * rhs.y_ + m02_),
            (m10_ * rhs.x_ + m11_ * rhs.y_ + m12_)
        );
    }

    /// Multiply a Vector3 which is assumed to represent position3D and skip the z-coordinate.
    Vector3 operator *(const Vector3& rhs) const
    {
        return Vector3(
            (m00_ * rhs.x_ + m01_ * rhs.y_ + m02_),
            (m10_ * rhs.x_ + m11_ * rhs.y_ + m12_),
            rhs.z_
        );
    }

    /// Multiply a matrix 2x3
    Matrix2x3 operator *(const Matrix2x3& rhs) const
    {
        return Matrix2x3(
            m00_ * rhs.m00_ + m01_ * rhs.m10_,
            m00_ * rhs.m01_ + m01_ * rhs.m11_,
            m00_ * rhs.m02_ + m01_ * rhs.m12_ + m02_,
            m10_ * rhs.m00_ + m11_ * rhs.m10_,
            m10_ * rhs.m01_ + m11_ * rhs.m11_,
            m10_ * rhs.m02_ + m11_ * rhs.m12_ + m12_
        );
    }

    /// Add a matrix.
    Matrix2x3 operator +(const Matrix2x3& rhs) const
    {
        return Matrix2x3(
            m00_ + rhs.m00_,
            m01_ + rhs.m01_,
            m02_ + rhs.m02_,
            m10_ + rhs.m10_,
            m11_ + rhs.m11_,
            m12_ + rhs.m12_
        );
    }

    /// Subtract a matrix.
    Matrix2x3 operator -(const Matrix2x3& rhs) const
    {
        return Matrix2x3(
            m00_ - rhs.m00_,
            m01_ - rhs.m01_,
            m02_ - rhs.m02_,
            m10_ - rhs.m10_,
            m11_ - rhs.m11_,
            m12_ - rhs.m12_
        );
    }

    /// Multiply with a scalar.
    Matrix2x3 operator *(float rhs) const
    {
        return Matrix2x3(
            m00_ * rhs,
            m01_ * rhs,
            m02_ * rhs,
            m10_ * rhs,
            m11_ * rhs,
            m12_ * rhs
        );
    }

    /// Multiply a 3x3 matrix.
    Matrix3 operator *(const Matrix3& rhs) const
    {
        return Matrix3(
            m00_ * rhs.m00_ + m01_ * rhs.m10_ + m02_ * rhs.m20_,
            m00_ * rhs.m01_ + m01_ * rhs.m11_ + m02_ * rhs.m21_,
            m00_ * rhs.m02_ + m01_ * rhs.m12_ + m02_ * rhs.m22_,
            m10_ * rhs.m00_ + m11_ * rhs.m10_ + m12_ * rhs.m20_,
            m10_ * rhs.m01_ + m11_ * rhs.m11_ + m12_ * rhs.m21_,
            m10_ * rhs.m02_ + m11_ * rhs.m12_ + m12_ * rhs.m22_,
            rhs.m20_,
            rhs.m21_,
            rhs.m22_
        );
    }

    void Multiply(const Vector2& rhs, Vector3& value) const
    {
        value.x_ = m00_ * rhs.x_ + m01_ * rhs.y_ + m02_;
        value.y_ = m10_ * rhs.x_ + m11_ * rhs.y_ + m12_;
    }

    void Multiply(float x, float y, Vector3& value) const
    {
        value.x_ = m00_ * x + m01_ * y + m02_;
        value.y_ = m10_ * x + m11_ * y + m12_;
    }

    void Multiply(float x, float y, Vector2& value) const
    {
        value.x_ = m00_ * x + m01_ * y + m02_;
        value.y_ = m10_ * x + m11_ * y + m12_;
    }

    void Set(const Vector2& translation, float rotation, const Vector2& scale=Vector2::ONE)
	{
        SetRotation(rotation, scale);
        SetTranslation(translation);
	}

	/// Set translation, scale and rotation (with angle.x_ must be the cos and angle.y_ must be sin)
    void Set(const Vector2& translation, const Vector2& angle, const Vector2& scale=Vector2::ONE)
	{
        m00_ = angle.x_  * scale.x_;
        m01_ = -angle.y_ * scale.y_;
        m10_ = angle.y_  * scale.x_;
        m11_ = angle.x_  * scale.y_;
        m02_ = translation.x_;
        m12_ = translation.y_;
	}

    /// Set translation elements.
    void SetTranslation(const Vector2& translation)
    {
        m02_ = translation.x_;
        m12_ = translation.y_;
    }

    /// Set rotation elements from a 3x3 matrix.
    void SetRotation(const Matrix3& rotation)
    {
        m00_ = rotation.m00_;
        m01_ = rotation.m01_;
        m10_ = rotation.m10_;
        m11_ = rotation.m11_;
    }

    /// Set rotation elements from a 2x2 matrix.
    void SetRotation(const Matrix2& rotation)
    {
        m00_ = rotation.m00_;
        m01_ = rotation.m01_;
        m10_ = rotation.m10_;
        m11_ = rotation.m11_;
    }
    /// Set rotation from angle.
    void SetRotation(float rotation, const Vector2& scale=Vector2::ONE)
    {
        rotation *= M_DEGTORAD;
        float a = cosf(rotation);
        m00_ = a * scale.x_;
        m11_ = a * scale.y_;
        a = sinf(rotation);
        m10_ = a * scale.x_;
        m01_ = -a * scale.y_;
    }
    /// Set scaling elements.
    void SetScale(const Vector2& scale)
    {
        m00_ = scale.x_;
        m11_ = scale.y_;
    }

    /// Set uniform scaling elements.
    void SetScale(float scale)
    {
        m00_ = scale;
        m11_ = scale;
    }

    /// Return the combined rotation and scaling matrix.
    Matrix2 ToMatrix2() const
    {
        return Matrix2(
            m00_,
            m01_,
            m10_,
            m11_
        );
    }

    /// Return the rotation matrix with scaling removed.
    Matrix2 RotationMatrix() const
    {
        Vector2 invScale(
            1.0f / sqrtf(m00_ * m00_ + m10_ * m10_),
            1.0f / sqrtf(m01_ * m01_ + m11_ * m11_)
        );

        return ToMatrix2().Scaled(invScale);
    }

    /// Return the translation part.
    Vector2 Translation() const
    {
        return Vector2(
            m02_,
            m12_
        );
    }

    /// Return the rotation part (in degree)
    float Rotation() const
    {
        return Acos(RotationMatrix().m00_);
    }

    /// Return the scaling part.
    Vector2 Scale() const
    {
        return Vector2(
            sqrtf(m00_ * m00_ + m10_ * m10_),
            sqrtf(m01_ * m01_ + m11_ * m11_)
        );
    }

    /// Return the scaling part with the sign. Reference rotation matrix is required to avoid ambiguity.
    Vector2 SignedScale(const Matrix2& rotation) const
    {
        return Vector2(
            rotation.m00_ * m00_ + rotation.m10_ * m10_,
            rotation.m01_ * m01_ + rotation.m11_ * m11_
        );
    }

    /// Test for equality with another matrix with epsilon.
    bool Equals(const Matrix2x3& rhs) const
    {
        const float* leftData = Data();
        const float* rightData = rhs.Data();

        for (unsigned i = 0; i < 6; ++i)
        {
            if (!Urho3D::Equals(leftData[i], rightData[i]))
                return false;
        }

        return true;
    }

    /// Return decomposition to translation, rotation and scale.
    void Decompose(Vector2& translation, float& rotation, Vector2& scale) const;
    /// Return inverse.
    Matrix2x3 Inverse() const;

    /// Return float data.
    const float* Data() const { return &m00_; }

    /// Return as string.
    String ToString() const;

    float m00_;
    float m01_;
    float m02_;
    float m10_;
    float m11_;
    float m12_;

    /// Zero matrix.
    static const Matrix2x3 ZERO;
    /// Identity matrix.
    static const Matrix2x3 IDENTITY;
};

/// Multiply a 2x3 matrix with a scalar.
inline Matrix2x3 operator *(float lhs, const Matrix2x3& rhs) { return rhs * lhs; }

