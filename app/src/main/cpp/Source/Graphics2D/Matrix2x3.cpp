#include "Matrix2x3.h"

#include <cstdio>

const Matrix2x3 Matrix2x3::ZERO(
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f);

const Matrix2x3 Matrix2x3::IDENTITY;

void Matrix2x3::Decompose(Vector2& translation, float& rotation, Vector2& scale) const
{
    translation.x_ = m02_;
    translation.y_ = m12_;

    scale.x_ = sqrtf(m00_ * m00_ + m10_ * m10_);
    scale.y_ = sqrtf(m01_ * m01_ + m11_ * m11_);

    rotation = Rotation();
}

Matrix2x3 Matrix2x3::Inverse() const
{
    float det = m00_ * m11_ -
                m01_ * m10_;

    float invDet = 1.0f / det;

    Matrix2x3 ret;
    ret.m00_ = m11_ * invDet;
    ret.m01_ = -m01_ * invDet;
    ret.m02_ = -(m02_ * ret.m00_ + m12_ * ret.m01_);
    ret.m10_ = -m10_ * invDet;
    ret.m11_ = m00_ * invDet;
    ret.m12_ = -(m02_ * ret.m10_ + m12_ * ret.m11_);

    return ret;
}

String Matrix2x3::ToString() const
{
    char tempBuffer[MATRIX_CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%g %g %g %g %g %g", m00_, m01_, m02_, m10_, m11_, m12_);
    return String(tempBuffer);
}

