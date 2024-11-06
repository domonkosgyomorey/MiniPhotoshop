#ifndef IMAGE_TRANSFORMATION_H
#include<iostream>

void applyGammaCorrection(unsigned char* imageData, int width, int height, float gamma) {
    int numPixels = width * height;
    for (int i = 0; i < numPixels * 4; i += 4) {
        for (int j = 0; j < 3; ++j) {
            float normalizedValue = imageData[i + j] / 255.0f;
            float gammaCorrectedValue = std::pow(normalizedValue, gamma);
            imageData[i + j] = static_cast<unsigned char>(gammaCorrectedValue * 255);
        }
    }
}

void applyNegation(unsigned char* imageData, int width, int height) {
    int numPixels = width * height;
    for (int i = 0; i < numPixels * 4; i += 4) {
        for (int j = 0; j < 3; ++j) {
            imageData[i + j] = 255 - imageData[i + j];
        }
    }
}

void applyLog(unsigned char* imageData, int width, int height, float c) {
    int numPixels = width * height;
    for (int i = 0; i < numPixels * 4; i += 4) {
        for (int j = 0; j < 3; ++j) {
            imageData[i + j] = c*std::log(1+imageData[i + j]);
        }
    }
}

void transformGray(unsigned char* imageData, int width, int height) {
    int numPixels = width * height;
    for (int i = 0; i < numPixels * 4; i += 4) {
        const char gray = 0.299 * imageData[i + 0] + 0.587 * imageData[i + 1] + 0.114 * imageData[i + 2];
        imageData[i + 0] = gray;
        imageData[i + 1] = gray;
        imageData[i + 2] = gray;
    }
}

void createHistogram(unsigned char* imageData, int width, int height, unsigned long long* histogram) {
    int numPixels = width * height;
    std::fill(histogram, histogram + 256, 0);

    for (int i = 0; i < numPixels * 4; i += 4) {
        unsigned char gray = static_cast<unsigned char>(0.299 * imageData[i] + 0.587 * imageData[i + 1] + 0.114 * imageData[i + 2]);
        histogram[gray]++;
    }
}

void applyHistogramEqualization(unsigned char* imageData, int width, int height) {
    unsigned long long histogram[256];
    createHistogram(imageData, width, height, histogram);

    unsigned long long cumulativeHistogram[256];
    cumulativeHistogram[0] = histogram[0];
    for (int i = 1; i < 256; ++i) {
        cumulativeHistogram[i] = cumulativeHistogram[i - 1] + histogram[i];
    }

    int numPixels = width * height;
    float scale = 255.0f / numPixels;

    unsigned char lut[256];
    for (int i = 0; i < 256; ++i) {
        lut[i] = static_cast<unsigned char>(std::min(255.0f, scale * cumulativeHistogram[i]));
    }

    for (int i = 0; i < numPixels * 4; i += 4) {
        for (int j = 0; j < 3; ++j) {
            imageData[i + j] = lut[imageData[i + j]];
        }
    }
}

#endif // !IMAGE_TRANSFORMATION_H
