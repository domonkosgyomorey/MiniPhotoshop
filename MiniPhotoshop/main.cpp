#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Shader.h"
#include "Texture.h"

#include <filesystem>
#include "nfd.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

unsigned int texture1;
unsigned int texture2;
float scale = 1.0f;

void drawHistogram(const char* label, const std::array<int, 256>& values, int maxValue, ImVec4 color) {
    ImGui::PushID(label);

    const float height = 80.0f;
    const float width = 200.0f;
    ImVec2 pos = ImGui::GetCursorScreenPos();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRect(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(255, 255, 255, 255));

    if (maxValue == 0) {
        maxValue = 1;

        for (int i = 0; i < 256; i++) {
            float normalized = std::min(1.0f, values[i] / static_cast<float>(maxValue));
            float x1 = pos.x + (width * i / 256.0f);
            float x2 = pos.x + (width * (i + 1) / 256.0f);
            float y1 = pos.y + height;
            float y2 = pos.y + height - (normalized * height);

            ImU32 col = ImGui::ColorConvertFloat4ToU32(color);
            draw_list->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), col);
        }
    }
    ImGui::Dummy(ImVec2(width, height));
    ImGui::PopID();
}

void renderImageProcessingUI(Texture & modifiedTexture, Texture & originalTexture, GLFWwindow * window) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)width, (float)height));

    ImGui::Begin("Image Processing", nullptr, ImGuiWindowFlags_MenuBar);

    ImGui::BeginChild("Controls", ImVec2(400, 0), true);

    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.2f, 0.1f, 0.8f));
    ImGui::GetStyle().AntiAliasedLines = false;
    ImGui::GetStyle().AntiAliasedFill = false;
    ImGui::Text("Gray Histogram");
    modifiedTexture.calculateHistogram();
    int maxValue = *std::max_element(modifiedTexture.grayHistogram.begin(), modifiedTexture.grayHistogram.end());
    drawHistogram("Current", modifiedTexture.grayHistogram, maxValue, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
    ImGui::PopStyleColor(2);

    ImGui::Separator();
    if (ImGui::Button("Reset to Original", ImVec2(-1, 0))) {
        modifiedTexture = originalTexture;
    }

    if (ImGui::BeginTabBar("ProcessingTabs")) {

        if (ImGui::BeginTabItem("Basic Operations")) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.9f, 0.4f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
            ImGui::BeginTable("BasicOps", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (ImGui::Button("Negate Colors", ImVec2(-1, 0))) {
                modifiedTexture.negate();
                modifiedTexture.updateTexture();
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (ImGui::Button("Convert to Grayscale", ImVec2(-1, 0))) {
                modifiedTexture.toGray();
                modifiedTexture.updateTexture();
            }

            ImGui::EndTable();
            ImGui::PopStyleColor(2);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Adjustments")) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.4f, 0.9f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.5f, 0.5f, 1.0f, 1.0f));
            static float gammaValue = 1.0f;
            static float logScale = 1.0f;

            ImGui::BeginTable("Adjustments", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Gamma Correction");
            ImGui::SliderFloat("Value##gamma", &gammaValue, 0.1f, 5.0f, "%.2f");
            if (ImGui::Button("Apply Gamma", ImVec2(-1, 0))) {
                modifiedTexture.applyGammaCorrection(gammaValue);
                modifiedTexture.updateTexture();
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Log Transform");
            ImGui::SliderFloat("Scale##log", &logScale, 0.1f, 10.0f, "%.2f");
            if (ImGui::Button("Apply Log", ImVec2(-1, 0))) {
                modifiedTexture.applyLog(logScale);
                modifiedTexture.updateTexture();
            }

            ImGui::EndTable();
            ImGui::PopStyleColor(2);
            ImGui::Spacing();
            ImGui::Text("Histogram Equalization");
            ImGui::BeginTable("HistogramEq", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (ImGui::Button("Grayscale Equalization", ImVec2(-1, 0))) {
                modifiedTexture.applyHistogramEqualization();
                modifiedTexture.updateTexture();
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (ImGui::Button("Color Equalization", ImVec2(-1, 0))) {
                modifiedTexture.applyColorHistogramEqualization();
                modifiedTexture.updateTexture();
            }

            ImGui::EndTable();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Filters")) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.6f, 0.8f, 0.6f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.8f, 1.0f, 0.8f, 1.0f));
            static int boxSize = 3;
            static int gaussianSize = 5;

            ImGui::BeginTable("Filters", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Box Filter");
            ImGui::SliderInt("Size##box", &boxSize, 1, 15);
            if (ImGui::Button("Apply Box", ImVec2(-1, 0))) {
                modifiedTexture.applyBoxFilter(boxSize);
                modifiedTexture.updateTexture();
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Gaussian Filter");
            ImGui::SliderInt("Size##gaussian", &gaussianSize, 3, 15);
            if (ImGui::Button("Apply Gaussian", ImVec2(-1, 0))) {
                modifiedTexture.applyGaussianFilter(gaussianSize);
                modifiedTexture.updateTexture();
            }

            ImGui::EndTable();
            ImGui::PopStyleColor(2);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Edge Detection")) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.9f, 0.9f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 1.0f, 0.5f, 1.0f));
            ImGui::BeginTable("EdgeDetection", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (ImGui::Button("Sobel Edge Detection", ImVec2(-1, 0))) {
                modifiedTexture.applySobelEdgeDetection();
                modifiedTexture.updateTexture();
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (ImGui::Button("Laplace Edge Detection", ImVec2(-1, 0))) {
                modifiedTexture.applyLaplaceEdgeDetection();
                modifiedTexture.updateTexture();
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (ImGui::Button("Prewitt Edge Detection", ImVec2(-1, 0))) {
                modifiedTexture.applyPrewittFilter();
                modifiedTexture.updateTexture();
            }

            ImGui::EndTable();
            ImGui::PopStyleColor(2);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Feature Detection")) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.7f, 0.7f, 0.9f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.9f, 0.9f, 1.0f, 1.0f));
            static float cornerThreshold = 0;
            static float k = 0.04f;
            ImGui::Text("Harris Corner Detection");
            ImGui::SliderFloat("Threshold##harris", &cornerThreshold, 0, 100, "%.1f");
            ImGui::SliderFloat("K##harris", &k, 0.04f, 0.06f, "%.4f");
            if (ImGui::Button("Detect Corners", ImVec2(-1, 0))) {
                modifiedTexture.detectCornersHarris(k, cornerThreshold);
            }

            ImGui::PopStyleColor(2);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();

        ImGui::Text("Read image");
        if (ImGui::Button("Read Image", ImVec2(-1, 0))) {
            nfdchar_t* outPath = NULL;
            nfdresult_t result = NFD_OpenDialog("png,jpg", NULL, &outPath);

            if (result == NFD_OKAY) {
                modifiedTexture.loadFromFile(outPath);
                modifiedTexture.calculateHistogram();
                originalTexture.loadFromFile(outPath);
                originalTexture.updateTexture();
                modifiedTexture.updateTexture();
            }
            else if (result == NFD_ERROR) {
                throw new std::exception("Read image failed");
            }
        }

        ImGui::Text("Write image");
        if (ImGui::Button("Write Image", ImVec2(-1, 0))) {
            nfdchar_t* savePath = NULL;
            nfdresult_t result = NFD_SaveDialog("png,jpg", NULL, &savePath);
            if (result == NFD_OKAY){
                modifiedTexture.writeToFile(savePath);
                free(savePath);
            }
            else if (result == NFD_ERROR) {
                throw new std::exception("Read image failed");
            }
        }
        
    }

    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("ImageView", ImVec2(0, 0), true);

    float aspectRatio = static_cast<float>(modifiedTexture.getWidth()) / static_cast<float>(modifiedTexture.getHeight());
    ImVec2 available = ImGui::GetContentRegionAvail();

    float displayWidth = available.x * 0.48f;
    float displayHeight = displayWidth / aspectRatio;

    if (displayHeight > available.y) {
        displayHeight = available.y;
        displayWidth = displayHeight * aspectRatio;
    }

    ImGui::Text("Modified Image");

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.0f, 5.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    ImGui::Image((ImTextureID)modifiedTexture.getTextureId(), ImVec2(displayWidth, displayHeight));
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();

    ImGui::EndChild();
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main() {

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int windowWidth = 900, windowHeight=600;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Mini Photoshop", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }


    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, 720, 360);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    Texture originalTexture("city.jpg");
    Texture modifiedTexture("city.jpg");

    float aspectRatio1 = modifiedTexture.getWidth() / (float)modifiedTexture.getHeight();
    float aspectRatio2 = originalTexture.getWidth() / (float)originalTexture.getHeight();

while (!glfwWindowShouldClose(window)) {

        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        renderImageProcessingUI(modifiedTexture, originalTexture, window);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}