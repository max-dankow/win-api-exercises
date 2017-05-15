#ifndef BRADLEYBINARIZATION_H
#define BRADLEYBINARIZATION_H

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <vector>
#include <assert.h>
#include <math.h>
#include <algorithm>

#include "iimagefilter.h"

struct CRect {
    int left, top, right, bottom;

    CRect Intersect(const CRect& other) const {
        auto newLeft = std::max<int>(this->left, other.left);
        auto newRight = std::min<int>(this->right, other.right);

        auto newTop = std::max<int>(this->top, other.top);
        auto newBottom = std::min<int>(this->bottom, other.bottom);
        
        if (newLeft > newRight || newTop > newBottom) {
            return { 0,0,0,0 };
        }
        return { newLeft, newTop, newRight, newBottom };
    }

    long Area() const {
        return (bottom - top) * (right - left);
    }
};

class BradleyBinarization : public IImageFilter {
public:
    CMatrix<CColor> apply(const CMatrix<CColor>& source) {
        auto height = source.GetHeight();
        auto width = source.GetWidth();
        CRect imageRect = { 0, 0, source.GetWidth(), source.GetHeight() };
        int wh = height / 16, ww = width / 16;
        CMatrix<CColor> outputData(height, width);

        computeIntegralImage(height, width, inputData);
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                CRect rect{ j - ww, i - wh, 2 * ww + 1, 2 * wh + 1 };
                float localMean = calculateLocalMean(rect, imageRect);
                float threshold = localMean * (1. - 0.15);
                if (inputData[i][j] >= threshold) {
                    outputData.SetAt({0, 0, 0}, i, j);
                } else {
                    outputData.SetAt({ 255, 255, 255 }, i, j);
                }
            }
        }
        return outputData;
    }
private:

    void computeIntegralImage(int height, int width, const std::vector< std::vector<float> > &input) {
        integral.assign(height, std::vector<float>(width, 0));
        integralSquares.assign(height, std::vector<float>(width, 0));
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                integral[i][j] = input[i][j];
                integralSquares[i][j] = input[i][j] * input[i][j];
                if (i == 0 && j == 0) {
                    continue;
                }
                if (i == 0) {
                    integral[i][j] += integral[i][j - 1];
                    integralSquares[i][j] += integralSquares[i][j - 1];
                    continue;
                }
                if (j == 0) {
                    integral[i][j] += integral[i - 1][j];
                    integralSquares[i][j] += integralSquares[i - 1][j];
                    continue;
                }
                integral[i][j] += integral[i][j - 1] + integral[i - 1][j] - integral[i - 1][j - 1];
                integralSquares[i][j] += integralSquares[i][j - 1] + integralSquares[i - 1][j] - integralSquares[i - 1][j - 1];
            }
        }
    }

    float computeSum(const CRect& area, const std::vector< std::vector<float> >& matrix) const {
        auto top = area.top,
                right = area.right,
                bottom = area.bottom,
                left = area.left;

        assert(top >= 0 && right < matrix.front().size()
               && bottom < matrix.size() && left >= 0);

        auto sum = matrix[bottom][right];
        if (top != 0 && left != 0) {
            return sum - matrix[top - 1][right] - matrix[bottom][left - 1] + matrix[top - 1][left - 1];
        }
        if (top != 0) {
            return sum - matrix[top - 1][right];
        }
        if (left != 0) {
            return sum - matrix[bottom][left - 1];
        }
        return sum;
    }

    float calculateLocalMean(const CRect& rect, const CRect& imageRect) const {
        CRect validArea = imageRect.Intersect(rect);
        auto area = validArea.Area();
        return computeSum(validArea, integral) / area;
    }

    float calculateLocalDeviation(const CRect& rect, const CRect& imageRect, float mean) {
        CRect validArea = imageRect.Intersect(rect);
        auto area = validArea.Area();
        float sum = computeSum(validArea, integralSquares);
        return sqrt(sum / area - mean * mean);
    }

    std::vector< std::vector<float> > integral, integralSquares;
    std::vector< std::vector<float> > inputData;
};

#endif // BRADLEYBINARIZATION_H
