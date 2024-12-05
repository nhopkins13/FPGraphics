/*
*  CSCI 441, Computer Graphics, Fall 2024
 *
 *  Project: a3
 *  File: main.cpp
 *
 *  Description:
 *      This file contains the basic setup to work with GLSL shaders and
 *      implement diffuse lighting.
 *
 *  Author: Dr. Paone, Colorado School of Mines, 2024
 *
 */

#include "FPEngine.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int main() {

    auto mpEngine = new FPEngine();
    mpEngine->initialize();
    if (mpEngine->getError() == CSCI441::OpenGLEngine::OPENGL_ENGINE_ERROR_NO_ERROR) {
        mpEngine->run();
    }
    mpEngine->shutdown();
    delete mpEngine;

    return EXIT_SUCCESS;
}

