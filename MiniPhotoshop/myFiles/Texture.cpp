#include "Texture.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <array>
#include <omp.h>
#include <numeric>

Texture::Texture(const std::string& path) {
	loadFromFile(path);
}

Texture::Texture(const Texture& other)
	: width(other.width), height(other.height), nrChannel(other.nrChannel) {
	const size_t dataSize = width * height * nrChannel;
	data = new unsigned char[dataSize];
	std::memcpy(data, other.data, dataSize);

	glGenTextures(1, &textureId);
	glDisable(GL_MULTISAMPLE);

	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexImage2D(GL_TEXTURE_2D, 0,
		nrChannel == 4 ? GL_RGBA : GL_RGB,
		width, height, 0,
		nrChannel == 4 ? GL_RGBA : GL_RGB,
		GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
}

Texture& Texture::operator=(const Texture& other) {
	if (this != &other) {
		width = other.width;
		height = other.height;
		nrChannel = other.nrChannel;

		const size_t dataSize = width * height * nrChannel;
		data = new unsigned char[dataSize];
		std::memcpy(data, other.data, dataSize);

		glGenTextures(1, &textureId);
		glDisable(GL_MULTISAMPLE);

		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexImage2D(GL_TEXTURE_2D, 0,
			nrChannel == 4 ? GL_RGBA : GL_RGB,
			width, height, 0,
			nrChannel == 4 ? GL_RGBA : GL_RGB,
			GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	return *this;
}

Texture::~Texture() {
	delete[] data;
}

void Texture::loadFromFile(const std::string& path) {
	if (textureId != 0) {
		delete[] data;
	}

	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glDisable(GL_MULTISAMPLE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int w, h, channels;
	unsigned char* imgData = stbi_load(path.c_str(), &w, &h, &channels, 0);
	if (!imgData) {
		throw std::runtime_error("Failed to load texture: " + path);
	}

	if (channels != 3 && channels != 4 && channels != 1) {
		stbi_image_free(imgData);
		throw std::runtime_error("Unsupported number of channels: " + std::to_string(channels));
	}

	width = static_cast<unsigned int>(w);
	height = static_cast<unsigned int>(h);
	nrChannel = static_cast<unsigned int>(channels==1?3:channels);

	const size_t dataSize = width * height * nrChannel;
	data = new unsigned char[dataSize];

	if (channels == 1) {
		for (int i = 0; i < width * height; ++i) {
			unsigned char grayValue = imgData[i];
			data[i * 3] = grayValue;
			data[i * 3 + 1] = grayValue;
			data[i * 3 + 2] = grayValue;
		}

	}
	else {
		std::memcpy(data, imgData, dataSize);
	}

	GLenum format = GL_RGB;
	if (nrChannel == 4) {
		format = GL_RGBA;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(imgData);
}



void Texture::writeToFile(const char* path) const {
	if (!stbi_write_png(path, width, height, nrChannel, data, width * nrChannel)) {
		std::cout << "Cannot write file into path: " << path << std::endl;
	}
}

unsigned char* Texture::getData() const {
	return this->data;
}

unsigned int Texture::getHeight() const {
	return this->height;
}

unsigned int Texture::getNrChannel() const{
	return this->nrChannel;
}

unsigned int Texture::getWidth() const {
	return this->width;
}

unsigned int Texture::getTextureId() const {
	return this->textureId;
}

void Texture::updateTexture() {
	if (!data) {
		std::cout << "Pixel data is null" << std::endl;
		exit(-1);
	}

	glBindTexture(GL_TEXTURE_2D, textureId);
	glDisable(GL_MULTISAMPLE);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, nrChannel == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_2D, 0);
	glGenerateMipmap(GL_TEXTURE_2D);
	glFlush();

	calculateHistogram();
}

void Texture::applyGammaCorrection(float gamma) {
	std::vector<unsigned char> dataVector(data, data + width * height * nrChannel);

#pragma omp parallel for
	for (size_t i = 0; i < dataVector.size(); ++i) {
		if (i % nrChannel < 3) {
			dataVector[i] = static_cast<unsigned char>(std::clamp(std::pow(dataVector[i] / 255.0f, 1.0f / gamma) * 255.0f, 0.0f, 255.0f));
		}
	}

	std::memcpy(data, dataVector.data(), dataVector.size());
}

void Texture::applyLog(float c) {
	std::vector<unsigned char> dataVector(data, data + width * height * nrChannel);

#pragma omp parallel for
	for (size_t i = 0; i < dataVector.size(); ++i) {
		if (i % nrChannel < 3) {
			dataVector[i] = static_cast<unsigned char>(std::clamp(c * (float)std::log(1 + dataVector[i]), 0.0f, 255.0f));
		}
	}

	std::memcpy(data, dataVector.data(), dataVector.size());
}

void Texture::negate() {
	std::vector<unsigned char> dataVector(data, data + width * height * nrChannel);

#pragma omp parallel for
	for (size_t i = 0; i < dataVector.size(); ++i) {
		if (i % nrChannel < 3) {
			dataVector[i] = 255 - dataVector[i];
		}
	}

	std::memcpy(data, dataVector.data(), dataVector.size());
}

void Texture::toGray() {
#pragma omp parallel for
	for (int i = 0; i < width * height; ++i) {
		int pixelOffset = i * nrChannel;
		unsigned char r = data[pixelOffset];
		unsigned char g = data[pixelOffset + 1];
		unsigned char b = data[pixelOffset + 2];
		unsigned char gray = static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);
		data[pixelOffset] = data[pixelOffset + 1] = data[pixelOffset + 2] = gray;
	}
}

void Texture::applyColorHistogramEqualization() {
	if (!data || width == 0 || height == 0) throw std::runtime_error("Invalid textureé data");
	const size_t totalPixels = width * height;
	const int MAX_INTENSITY = 256;
	std::vector<std::vector<int>> histograms(3, std::vector<int>(MAX_INTENSITY, 0));
	std::vector<std::vector<int>> cdfs(3, std::vector<int>(MAX_INTENSITY, 0));

#pragma omp parallel for
	for (size_t idx = 0; idx < totalPixels * nrChannel; ++idx) {
		if (idx % nrChannel < 3) {
			unsigned char pixel = data[idx];
#pragma omp atomic
			histograms[idx % 3][pixel]++;
		}
	}

	for (int channel = 0; channel < 3; ++channel) {
		std::partial_sum(histograms[channel].begin(), histograms[channel].end(), cdfs[channel].begin());
	}

	std::vector<int> cdfMins(3, 0);
	for (int channel = 0; channel < 3; ++channel) {
		auto minIt = std::find_if(cdfs[channel].begin(), cdfs[channel].end(), [](int val) { return val > 0; });
		cdfMins[channel] = *minIt;
	}

	const float scale = static_cast<float>(MAX_INTENSITY - 1) / totalPixels;
	std::vector<std::vector<unsigned char>> lookupTables(3, std::vector<unsigned char>(MAX_INTENSITY));

#pragma omp parallel for
	for (int channel = 0; channel < 3; ++channel) {
		for (int i = 0; i < MAX_INTENSITY; ++i) {
			lookupTables[channel][i] = static_cast<unsigned char>(
				std::round(std::clamp((cdfs[channel][i] - cdfMins[channel]) * scale, 0.0f, 255.0f))
				);
		}
	}

#pragma omp parallel for
	for (size_t idx = 0; idx < totalPixels * nrChannel; ++idx) {
		if (idx % nrChannel < 3) {
			data[idx] = lookupTables[idx % 3][data[idx]];
		}
	}
}

void Texture::applyHistogramEqualization() {
	if (!data || width == 0 || height == 0) throw std::runtime_error("Invalid texture data");

	const size_t totalPixels = width * height;
	const int MAX_INTENSITY = 256;
	std::vector<unsigned char> grayValues(totalPixels);
	std::vector<int> histogram(MAX_INTENSITY, 0);

	for (size_t i = 0; i < totalPixels; ++i) {
		const size_t pixelOffset = i * nrChannel;
		unsigned char grayVal = static_cast<unsigned char>(0.299 * data[pixelOffset] + 0.587 * data[pixelOffset + 1] + 0.114 * data[pixelOffset + 2]);
		grayValues[i] = grayVal;
		histogram[grayVal]++;
	}

	std::vector<int> cdf(MAX_INTENSITY, 0);
	std::partial_sum(histogram.begin(), histogram.end(), cdf.begin());

	int cdfMin = *std::find_if(cdf.begin(), cdf.end(), [](int v) { return v > 0; });

	std::vector<unsigned char> lookupTable(MAX_INTENSITY);
	const float scale = static_cast<float>(MAX_INTENSITY - 1) / (totalPixels - cdfMin);
	for (size_t i = 0; i < MAX_INTENSITY; ++i) {
		lookupTable[i] = static_cast<unsigned char>(std::round(std::clamp((cdf[i] - cdfMin) * scale, 0.0f, 255.0f)));
	}

	for (size_t i = 0; i < totalPixels; ++i) {
		const size_t pixelOffset = i * nrChannel;
		unsigned char newValue = lookupTable[grayValues[i]];
		data[pixelOffset] = newValue;
		data[pixelOffset + 1] = newValue;
		data[pixelOffset + 2] = newValue;
		if (nrChannel == 4) data[pixelOffset + 3] = data[pixelOffset + 3];
	}
}

void Texture::applyBoxFilter(int size) {
	int halfKernel = size / 2;
	float kernelValue = 1.0f / (size * size);
	auto newData = std::make_unique<unsigned char[]>(width * height * nrChannel);

	auto clampCoords = [this](int coord, int max) {
		return std::clamp(coord, 0, max - 1);
		};

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			for (int c = 0; c < 3; ++c) {
				float sum = 0.0f;
				for (int ky = -halfKernel; ky <= halfKernel; ++ky) {
					for (int kx = -halfKernel; kx <= halfKernel; ++kx) {
						int nx = clampCoords(x + kx, width);
						int ny = clampCoords(y + ky, height);
						sum += data[(ny * width + nx) * nrChannel + c] * kernelValue;
					}
				}
				newData[(y * width + x) * nrChannel + c] = static_cast<unsigned char>(std::clamp(sum, 0.0f, 255.0f));
			}
		}
	}

	std::memcpy(data, newData.get(), width * height * nrChannel);
}

void Texture::applyGaussianFilter(int size) {
	int halfSize = size / 2;
	float sigma = 1.0f;
	std::vector<float> kernel(size * size);
	float sum = 0.0f;

	for (int y = -halfSize; y <= halfSize; ++y) {
		for (int x = -halfSize; x <= halfSize; ++x) {
			float exponent = -(x * x + y * y) / (2 * sigma * sigma);
			kernel[(y + halfSize) * size + (x + halfSize)] = exp(exponent) / (2 * 3.1415 * sigma * sigma);
			sum += kernel[(y + halfSize) * size + (x + halfSize)];
		}
	}

	for (float& val : kernel) {
		val /= sum;
	}

	auto newData = std::make_unique<unsigned char[]>(width * height * nrChannel);
	int halfKernel = size / 2;

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			for (int c = 0; c < 3; ++c) {
				float result = 0.0f;
				for (int ky = -halfKernel; ky <= halfKernel; ++ky) {
					for (int kx = -halfKernel; kx <= halfKernel; ++kx) {
						int nx = std::clamp(x + kx, 0, (int)width - 1);
						int ny = std::clamp(y + ky, 0, (int)height - 1);
						int pixelIndex = (ny * width + nx) * nrChannel + c;
						result += data[pixelIndex] * kernel[(ky + halfKernel) * size + (kx + halfKernel)];
					}
				}
				int newIndex = (y * width + x) * nrChannel + c;
				newData[newIndex] = static_cast<unsigned char>(std::clamp(result, 0.0f, 255.0f));
			}
		}
	}

	std::memcpy(data, newData.get(), width * height * nrChannel);
}

void Texture::applySobelEdgeDetection() {
	std::vector<unsigned char> tempData(width * height * nrChannel);
	const float sobelX[9] = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };
	const float sobelY[9] = { -1, -2, -1, 0, 0, 0, 1, 2, 1 };

#pragma omp parallel for collapse(2)
	for (int y = 1; y < height - 1; ++y) {
		for (int x = 1; x < width - 1; ++x) {
			float gradX[3] = { 0.0f, 0.0f, 0.0f };
			float gradY[3] = { 0.0f, 0.0f, 0.0f };

			for (int ky = -1; ky <= 1; ++ky) {
				for (int kx = -1; kx <= 1; ++kx) {
					int srcIdx = ((y + ky) * width + (x + kx)) * nrChannel;
					int kernelIdx = (ky + 1) * 3 + (kx + 1);

					for (int c = 0; c < 3; ++c) {
						gradX[c] += data[srcIdx + c] * sobelX[kernelIdx];
						gradY[c] += data[srcIdx + c] * sobelY[kernelIdx];
					}
				}
			}

			int destIdx = (y * width + x) * nrChannel;
			for (int c = 0; c < 3; ++c) {
				tempData[destIdx + c] = static_cast<unsigned char>(std::clamp(std::sqrt(gradX[c] * gradX[c] + gradY[c] * gradY[c]), 0.0f, 255.0f));
			}
			if (nrChannel == 4) {
				tempData[destIdx + 3] = data[destIdx + 3];
			}
		}
	}

	std::memcpy(data, tempData.data(), width * height * nrChannel);
}

void Texture::applyLaplaceEdgeDetection() {
	float kernel[3][3] = { {0, 1, 0}, {1, -4, 1}, {0, 1, 0} };
	int numPixels = width * height;
	std::vector<unsigned char> grayValues(numPixels);

#pragma omp parallel for
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int pixelOffset = (y * width + x) * nrChannel;
			grayValues[y * width + x] = static_cast<unsigned char>(
				0.299f * data[pixelOffset] + 0.587f * data[pixelOffset + 1] + 0.114f * data[pixelOffset + 2]);
		}
	}

#pragma omp parallel for collapse(2)
	for (int y = 1; y < height - 1; ++y) {
		for (int x = 1; x < width - 1; ++x) {
			int grad = 0;
			for (int ky = -1; ky <= 1; ++ky) {
				for (int kx = -1; kx <= 1; ++kx) {
					int grayValue = grayValues[(y + ky) * width + (x + kx)];
					grad += kernel[ky + 1][kx + 1] * grayValue;
				}
			}
			int laplaceValue = std::clamp(std::abs(grad), 0, 255);
			int pixelOffset = (y * width + x) * nrChannel;
			data[pixelOffset] = laplaceValue;
			data[pixelOffset + 1] = laplaceValue;
			data[pixelOffset + 2] = laplaceValue;
		}
	}
}

void Texture::detectCornersHarris(float k, float threshold) {
	std::vector<unsigned char> originalData(data, data + width * height * nrChannel);
	toGray();

	std::vector<float> gradX(width * height), gradY(width * height), cornerResponse(width * height);
	const float sobelX[9] = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };
	const float sobelY[9] = { -1, -2, -1, 0, 0, 0, 1, 2, 1 };

#pragma omp parallel for collapse(2)
	for (int y = 1; y < height - 1; ++y) {
		for (int x = 1; x < width - 1; ++x) {
			float gx = 0.0f, gy = 0.0f;
			for (int ky = -1; ky <= 1; ++ky) {
				for (int kx = -1; kx <= 1; ++kx) {
					int idx = ((y + ky) * width + (x + kx)) * nrChannel;
					float val = static_cast<float>(data[idx]);
					gx += val * sobelX[(ky + 1) * 3 + (kx + 1)];
					gy += val * sobelY[(ky + 1) * 3 + (kx + 1)];
				}
			}
			gradX[y * width + x] = gx;
			gradY[y * width + x] = gy;
		}
	}

#pragma omp parallel for collapse(2)
	for (int y = 1; y < height - 1; ++y) {
		for (int x = 1; x < width - 1; ++x) {
			float sumXX = 0.0f, sumYY = 0.0f, sumXY = 0.0f;
			for (int wy = -1; wy <= 1; ++wy) {
				for (int wx = -1; wx <= 1; ++wx) {
					int idx = (y + wy) * width + (x + wx);
					float gx = gradX[idx], gy = gradY[idx];
					sumXX += gx * gx;
					sumYY += gy * gy;
					sumXY += gx * gy;
				}
			}
			float det = sumXX * sumYY - sumXY * sumXY;
			float trace = sumXX + sumYY;
			cornerResponse[y * width + x] = det - k * trace * trace;
		}
	}

	std::memcpy(data, originalData.data(), width * height * nrChannel);

	const int radius = 1;
#pragma omp parallel for collapse(2)
	for (int y = radius; y < height - radius; ++y) {
		for (int x = radius; x < width - radius; ++x) {
			if (cornerResponse[y * width + x] > threshold) {
				bool isMax = true;
				for (int wy = -1; wy <= 1 && isMax; ++wy) {
					for (int wx = -1; wx <= 1 && isMax; ++wx) {
						if (wx == 0 && wy == 0) continue;
						if (cornerResponse[(y + wy) * width + (x + wx)] >= cornerResponse[y * width + x]) {
							isMax = false;
						}
					}
				}

				if (isMax) {
					for (int cy = -radius; cy <= radius; ++cy) {
						for (int cx = -radius; cx <= radius; ++cx) {
							if (cx * cx + cy * cy <= radius * radius) {
								int px = x + cx, py = y + cy;
								if (px >= 0 && px < width && py >= 0 && py < height) {
									int idx = (py * width + px) * nrChannel;
									data[idx] = 255; 
									data[idx + 1] = 0;
									data[idx + 2] = 0;
								}
							}
						}
					}
				}
			}
		}
	}

	updateTexture();  // Frissítjük a textúrát
}

void Texture::applyPrewittFilter() {
	const int prewittX[3][3] = {
		{-1, 0, 1},
		{-1, 0, 1},
		{-1, 0, 1}
	};

	const int prewittY[3][3] = {
		{-1, -1, -1},
		{0, 0, 0},
		{1, 1, 1}
	};

	int numPixels = width * height;
	std::vector<unsigned char> newData(numPixels * nrChannel);
	std::memcpy(newData.data(), data, numPixels * nrChannel);

	std::vector<unsigned char> grayValues(numPixels);
#pragma omp parallel for
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int pixelOffset = (y * width + x) * nrChannel;
			grayValues[y * width + x] = static_cast<unsigned char>(
				0.299f * data[pixelOffset] +
				0.587f * data[pixelOffset + 1] +
				0.114f * data[pixelOffset + 2]);
		}
	}

#pragma omp parallel for collapse(2)
	for (int y = 1; y < height - 1; ++y) {
		for (int x = 1; x < width - 1; ++x) {
			int gradX = 0, gradY = 0;
			for (int ky = -1; ky <= 1; ++ky) {
				for (int kx = -1; kx <= 1; ++kx) {
					int grayValue = grayValues[(y + ky) * width + (x + kx)];
					gradX += prewittX[ky + 1][kx + 1] * grayValue;
					gradY += prewittY[ky + 1][kx + 1] * grayValue;
				}
			}

			int magnitude = static_cast<int>(std::sqrt(gradX * gradX + gradY * gradY));
			unsigned char newGrayValue = std::clamp(magnitude, 0, 255);

			int newIdx = (y * width + x) * nrChannel;
			newData[newIdx] = newGrayValue;
			newData[newIdx + 1] = newGrayValue;
			newData[newIdx + 2] = newGrayValue;
		}
	}

	std::memcpy(data, newData.data(), numPixels * nrChannel);
}

void Texture::calculateHistogram() {
	grayHistogram.fill(0);

#pragma omp parallel for
	for (unsigned int i = 0; i < width * height; i++) {
		const unsigned char r = data[i * nrChannel];
		const unsigned char g = data[i * nrChannel + 1];
		const unsigned char b = data[i * nrChannel + 2];

		const unsigned char gray = static_cast<unsigned char>(
			0.299f * r + 0.587f * g + 0.114f * b
			);
#pragma omp atomic
		grayHistogram[gray]++;
	}
}