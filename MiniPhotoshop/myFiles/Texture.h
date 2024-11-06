#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../libFiles/stb_image.h"
#include "../libFiles/stb_image_write.h"
#include <iostream>
#include <array>

class Texture {
public:
    explicit Texture(const std::string& path);
    Texture(const Texture& other);
    Texture& operator=(const Texture& other);
    ~Texture();

    void loadFromFile(const std::string& path);

    void applyGammaCorrection(float gamma);
    void applyLog(float c);
    void negate();
    void toGray();
    void applyColorHistogramEqualization();
    void applyHistogramEqualization();
    void applyBoxFilter(int size);
    void applyGaussianFilter(int size);
    void applySobelEdgeDetection();
    void applyLaplaceEdgeDetection();
    void applyPrewittFilter();
    void detectCornersHarris(float k, float threshold);
    void updateTexture();
    void writeToFile(const char* path) const;
    void calculateHistogram();

    unsigned char* getData() const;
    unsigned int getWidth()const;
    unsigned int getHeight() const;
    unsigned int getNrChannel() const;
    unsigned int getTextureId() const;

    std::array<int, 256> grayHistogram{0};

private:
    unsigned char* data;
    unsigned int textureId{ 0 };
    unsigned int width{ 0 };
    unsigned int height{ 0 };
    unsigned int nrChannel{ 0 };
};

#endif // TEXTURE_H