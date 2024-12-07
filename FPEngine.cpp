#include "FPEngine.h"
#include <stb_image.h>

#ifndef M_PI
#define M_PI 3.14159265f
#endif

float getRand() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

FPEngine::FPEngine()
    : CSCI441::OpenGLEngine(4, 1,
                                 1280, 720, // Increased window size for better view
                                 "FP - The Grey Havens"),
        _pFreeCam(nullptr),
        _pArcballCam(nullptr),
        _pFPCam(nullptr),
        _pVehicle(nullptr),
        _pTPCam(nullptr),
        _animationTime(0.0f),
        _groundVAO(0),
        _numGroundPoints(0),
        _marbleVBO(0),
        _marbleVAO(0)
{
    for(auto& key : _keys) key = GL_FALSE;

    _mousePosition = glm::vec2(MOUSE_UNINITIALIZED, MOUSE_UNINITIALIZED );
    _leftMouseButtonState = GLFW_RELEASE;
    _enemies = new std::vector<Marble>();
}

FPEngine::~FPEngine() {
    delete _lightingShaderProgram;
    delete _textureShaderProgram;
    delete _pFreeCam;
    delete _pArcballCam;
    delete _pFPCam;
    delete _pTPCam;
    delete _pVehicle;
    delete _enemies;
    if (_marbleVBO) {
        glDeleteBuffers(1, &_marbleVBO);
        _marbleVBO = 0; // Reset to prevent dangling references
    }

    if (_marbleVAO) {
        glDeleteVertexArrays(1, &_marbleVAO);
        _marbleVAO = 0;
    }
}

void FPEngine::mSetupTextures() {
    glActiveTexture(GL_TEXTURE0);
    _texHandles[TEXTURE_ID::RUG] = _loadAndRegisterTexture("images/groundImage.png");
}

GLuint FPEngine::_loadAndRegisterTexture(const char* FILENAME) {
    GLuint textureHandle = 0;
    stbi_set_flip_vertically_on_load(true); // Flip image vertically

    GLint width, height, channels;
    unsigned char* data = stbi_load(FILENAME, &width, &height, &channels, 0);

    if (data) {
        glGenTextures(1, &textureHandle);
        if (textureHandle == 0) {
            fprintf(stderr, "Failed to generate texture handle for %s\n", FILENAME);
            stbi_image_free(data);
            return 0;
        }

        glBindTexture(GL_TEXTURE_2D, textureHandle);
        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
        stbi_image_free(data);
    } else {
        fprintf(stderr, "Failed to load texture: %s\n", FILENAME);
    }

    return textureHandle;
}


bool FPEngine::checkCollision(const glm::vec3& pos1, float radius1, const glm::vec3& pos2, float radius2) {
    float distanceSquared = glm::dot(pos1 - pos2, pos1 - pos2); // Squared distance
    float combinedRadii = radius1 + radius2;
    return distanceSquared <= (combinedRadii * combinedRadii);
}

bool FPEngine::isMovementValid(const glm::vec3& newPosition) const {
    float vehicleRadius = _pVehicle->getBoundingRadius();
    const float TREE_RADIUS = 0.5f; // Adjust based on the actual size of the tree model
    const float LAMP_RADIUS = 0.5f; // Adjust based on the actual size of the lamp model

    glm::vec3 currentPosition = _pVehicle->getPosition();

    // Check collision with trees
    for (const TreeData& tree : _trees) {
        glm::vec3 treePosition(tree.modelMatrixTrunk[3]); // Extract tree trunk position
        if (checkCollision(newPosition, vehicleRadius, treePosition, TREE_RADIUS)) {
            glm::vec3 bounceDirection = glm::normalize(currentPosition - treePosition);
            _pVehicle->setPosition(currentPosition + bounceDirection * 0.2f); // Small bounce backward
            return false;
        }
    }

    // Check collision with lamps
    for (const LampData& lamp : _lamps) {
        if (checkCollision(newPosition, vehicleRadius, lamp.position, LAMP_RADIUS)) {
            glm::vec3 bounceDirection = glm::normalize(currentPosition - lamp.position);
            _pVehicle->setPosition(currentPosition + bounceDirection * 0.2f); // Small bounce backward
            return false;
        }
    }

    return true; // No collision detected
}

glm::vec3 FPEngine::_evalBezierCurve(const glm::vec3 P0, const glm::vec3 P1, const glm::vec3 P2, const glm::vec3 P3, const GLfloat T) const{
    // TODO #01: solve the curve equation
    GLfloat oneMinusT = 1.0f - T;
    GLfloat b0 = oneMinusT * oneMinusT * oneMinusT;
    GLfloat b1 = 3 * oneMinusT * oneMinusT * T;
    GLfloat b2 = 3 * oneMinusT * T * T;
    GLfloat b3 = T * T * T;

    // Compute the point on the curve
    glm::vec3 bezierPoint = b0 * P0 + b1 * P1 + b2 * P2 + b3 * P3;

    return bezierPoint;
}

void FPEngine::_createCurve(GLuint vao, GLuint vbo, GLsizei &numVAOPoints) const {
    int resolution = 3; // Hardcode resolution to 3

    numVAOPoints  = _bezierCurve.numCurves * (resolution + 1);
    std::vector<glm::vec3> curvePoints(numVAOPoints);

    for (int i = 0; i < _bezierCurve.numCurves; ++i) {
        glm::vec3 p0 = _bezierCurve.controlPoints[i * 3 + 0];
        glm::vec3 p1 = _bezierCurve.controlPoints[i * 3 + 1];
        glm::vec3 p2 = _bezierCurve.controlPoints[i * 3 + 2];
        glm::vec3 p3 = _bezierCurve.controlPoints[i * 3 + 3];

        for (int j = 0; j <= resolution; ++j) {
            float t = static_cast<float>(j) / static_cast<float>(resolution);
            glm::vec3 point = _evalBezierCurve(p0, p1, p2, p3, t);
            curvePoints[i * (resolution + 1) + j] = point;
        }
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, curvePoints.size() * sizeof(glm::vec3), curvePoints.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    fprintf(stdout, "[INFO]: Bézier curve created with fixed resolution 3\n");
}

void FPEngine::handleKeyEvent(GLint key, GLint action, GLint mods) {
    if (key >= 0 && key < NUM_KEYS) {
        _keys[key] = ((action == GLFW_PRESS) || (action == GLFW_REPEAT));
    }

    if (action == GLFW_PRESS || action == GLFW_REPEAT) { // Handle key press
        switch (key) {
            // Quit!
            case GLFW_KEY_Q:
            case GLFW_KEY_ESCAPE:
                setWindowShouldClose();
                break;

                // Zoom In/Out with Space
            case GLFW_MOD_SHIFT:
                if (currCamera == CameraType::ARCBALL) { // Only zoom if in Arcball mode
                    if (mods & GLFW_KEY_SPACE) {
                        _pArcballCam->zoomIn();
                    } else {
                        _pArcballCam->zoomOut();
                    }
                }
                break;
            case GLFW_KEY_SPACE:
                if (!_isJumping) {
                    _isJumping = true;
                    _jumpProgress = 0.0f;

                    // Set control points for the jump
                    _jumpStartPosition = _pVehicle->getPosition();
                    // Vehicle heading vector
                    glm::vec3 headingVector = glm::vec3(sin(_pVehicle->getHeading()), 0.0f, cos(_pVehicle->getHeading()));

                    // Control points for the jump in the heading direction
                    _jumpControlPoints[0] = _jumpStartPosition;
                    _jumpControlPoints[1] = _jumpStartPosition + headingVector * 3.0f + glm::vec3(0.0f, 5.0f, 0.0f); // Apex control
                    _jumpControlPoints[2] = _jumpStartPosition + headingVector * 7.5f + glm::vec3(0.0f, 5.0f, 0.0f); // Mid control
                    _jumpControlPoints[3] = _jumpStartPosition + headingVector * 10.0f; // End position

                }
            break;

            case GLFW_KEY_3:
                currCamera = CameraType::THIRDPERSON;
                if (_pTPCam == nullptr) {
                    _pTPCam = new TPCamera(); // Adjust distance and heightOffset as needed
                }
                _pTPCam->update(_pVehicle->getPosition(), _pVehicle->getHeading());
                break;
            break;

            case GLFW_KEY_4:
                currCamera = CameraType::ARCBALL; // Switch to Arcball immediately when UFO is selected
                break;

            case GLFW_KEY_5:
                currCamera = CameraType::FREECAM; // Switch to Freecam immediately
                break;
            case GLFW_KEY_6:
                currCamera = CameraType::FIRSTPERSON; // Switch to First Person view

                if (_pFPCam == nullptr) {
                    _pFPCam = new FPCamera(2.0f);
                }

            _pFPCam->updatePositionAndOrientation(_pVehicle->getPosition(), _pVehicle->getHeading());
                break;

            default:
                break;
        }
    }
}




void FPEngine::handleMouseButtonEvent(GLint button, GLint action, GLint mods) {
    // if the event is for the left mouse button
    if( button == GLFW_MOUSE_BUTTON_LEFT ) {
        // update the left mouse button's state
        _leftMouseButtonState = action;

        if(action == GLFW_PRESS) {
            _shiftPressed = (mods & GLFW_MOD_SHIFT) != 0;

            if(_shiftPressed) {
                _zooming = true;
            }
        }
        else if(action == GLFW_RELEASE) {
            _zooming = false;
        }
    }
}

void FPEngine::handleCursorPositionEvent(glm::vec2 currMousePosition) {
    // if mouse hasn't moved in the window, prevent camera from flipping out
    if(_mousePosition.x == MOUSE_UNINITIALIZED) {
        _mousePosition = currMousePosition;
        return;
    }

    // Calculate mouse movement deltas
    float deltaX = currMousePosition.x - _mousePosition.x;
    float deltaY = currMousePosition.y - _mousePosition.y;

    // Update the mouse position
    _mousePosition = currMousePosition;

    if(_leftMouseButtonState == GLFW_PRESS) {
        if(_zooming) {
            // Perform zooming based on vertical mouse movement
            // Dragging up (deltaY < 0) should zoom in (decrease radius)
            // Dragging down (deltaY > 0) should zoom out (increase radius)
            float deltaZoom = -deltaY * _zoomSensitivity;
            _pArcballCam->zoom(deltaZoom);
        }
        else {
            // Perform rotation based on mouse movement
            float deltaTheta = -deltaX * 0.005f;
            float deltaPhi = -deltaY * 0.005f;

            if(currCamera == CameraType::ARCBALL) {
                _pArcballCam->rotate(deltaTheta, deltaPhi);
            }
            else if(currCamera == CameraType::FREECAM) {
                _pFreeCam->rotate(-deltaTheta, deltaPhi);
            }
        }
    }
}

//*************************************************************************************
//
//engine Setup

void FPEngine::mSetupGLFW() {
    CSCI441::OpenGLEngine::mSetupGLFW();

    // set our callbacks
    glfwSetKeyCallback(mpWindow, mp_engine_keyboard_callback);
    glfwSetMouseButtonCallback(mpWindow, mp_engine_mouse_button_callback);
    glfwSetCursorPosCallback(mpWindow, mp_engine_cursor_callback);

    // Set the user pointer to this instance
    glfwSetWindowUserPointer(mpWindow, this);
}

void FPEngine::mSetupOpenGL() {
    glEnable( GL_DEPTH_TEST );                        // enable depth testing
    glDepthFunc( GL_LESS );                           // use less than depth test

    glEnable(GL_BLEND);                                // enable blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);// use one minus blending equation

//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void FPEngine::mSetupShaders() {
    _lightingShaderProgram = new CSCI441::ShaderProgram("shaders/lighting.vs.glsl", "shaders/lighting.fs.glsl");
    // Retrieve uniform locations
    _lightingShaderUniformLocations.mvpMatrix = _lightingShaderProgram->getUniformLocation("mvpMatrix");
    _lightingShaderUniformLocations.normalMatrix = _lightingShaderProgram->getUniformLocation("normalMatrix");
    _lightingShaderUniformLocations.modelMatrix = _lightingShaderProgram->getUniformLocation("modelMatrix");
    _lightingShaderUniformLocations.viewPos = _lightingShaderProgram->getUniformLocation("viewPos");

    // Material properties
    _lightingShaderUniformLocations.materialAmbient = _lightingShaderProgram->getUniformLocation("material.ambient");
    _lightingShaderUniformLocations.materialDiffuse = _lightingShaderProgram->getUniformLocation("material.diffuse");
    _lightingShaderUniformLocations.materialSpecular = _lightingShaderProgram->getUniformLocation("material.specular");
    _lightingShaderUniformLocations.materialShininess = _lightingShaderProgram->getUniformLocation("material.shininess");

    // Directional Light
    _lightingShaderUniformLocations.dirLightDirection = _lightingShaderProgram->getUniformLocation("dirLight.direction");
    _lightingShaderUniformLocations.dirLightColor = _lightingShaderProgram->getUniformLocation("dirLight.color");

    // Point Lights
    _lightingShaderUniformLocations.numPointLights = _lightingShaderProgram->getUniformLocation("numPointLights");
    _lightingShaderUniformLocations.pointLightPositions = _lightingShaderProgram->getUniformLocation("pointLightPositions");
    _lightingShaderUniformLocations.pointLightColors = _lightingShaderProgram->getUniformLocation("pointLightColors");
    _lightingShaderUniformLocations.pointLightConstants = _lightingShaderProgram->getUniformLocation("pointLightConstants");
    _lightingShaderUniformLocations.pointLightLinears = _lightingShaderProgram->getUniformLocation("pointLightLinears");
    _lightingShaderUniformLocations.pointLightQuadratics = _lightingShaderProgram->getUniformLocation("pointLightQuadratics");

    // Spot Light
    _lightingShaderUniformLocations.spotLightDirection = _lightingShaderProgram->getUniformLocation("spotLightDirection");
    _lightingShaderUniformLocations.spotLightPosition = _lightingShaderProgram->getUniformLocation("spotLightPosition");
    _lightingShaderUniformLocations.spotLightWidth = _lightingShaderProgram->getUniformLocation("spotLightWidth");
    _lightingShaderUniformLocations.spotLightColor = _lightingShaderProgram->getUniformLocation("spotLightColor");

    // Attribute locations
    _lightingShaderAttributeLocations.vPos = _lightingShaderProgram->getAttributeLocation("vPos");
    _lightingShaderAttributeLocations.vNormal = _lightingShaderProgram->getAttributeLocation("vNormal");

    _textureShaderProgram = new CSCI441::ShaderProgram("shaders/texture.vs.glsl", "shaders/texture.fs.glsl");
    _textureShaderUniformLocations.mvpMatrix = _textureShaderProgram->getUniformLocation("mvpMatrix");
    _textureShaderUniformLocations.aTextMap = _textureShaderProgram->getUniformLocation("textureMap");

    _textureShaderAttributeLocations.vPos = _textureShaderProgram->getAttributeLocation("vPos");
    _textureShaderAttributeLocations.aTextCoords = _textureShaderProgram->getAttributeLocation("textureCoords");

    _textureShaderUniformLocations.spotLightPosition = _textureShaderProgram->getUniformLocation("spotLightPosition");
    _textureShaderUniformLocations.spotLightDirection = _textureShaderProgram->getUniformLocation("spotLightDirection");
    _textureShaderUniformLocations.spotLightWidth = _textureShaderProgram->getUniformLocation("spotLightWidth");
    _textureShaderUniformLocations.spotLightColor = _textureShaderProgram->getUniformLocation("spotLightColor");

}

void FPEngine::mSetupBuffers() {
    fprintf(stdout, "[DEBUG]: Setting up buffers...\n");

    //connect our 3D Object Library to our shader
    CSCI441::setVertexAttributeLocations( _lightingShaderAttributeLocations.vPos, _lightingShaderAttributeLocations.vNormal);

    _createGroundBuffers();
    _createArchBuffers();
    _generateEnvironment();

    glGenVertexArrays(1, &_marbleVAO);
    glBindVertexArray(_marbleVAO);

    glGenBuffers(1, &_marbleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, _marbleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(_marbleLocations), _marbleLocations, GL_STATIC_DRAW);

    // Set vertex attribute pointers for positions (no texture coordinates needed)
    glEnableVertexAttribArray(_lightingShaderAttributeLocations.vPos);
    glVertexAttribPointer(_lightingShaderAttributeLocations.vPos, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    glBindVertexArray(0);

    glGenVertexArrays(1, &_curveVAO);
    glGenBuffers(1, &_curveVBO);

    _createCurve(_curveVAO, _curveVBO, _numCurvePoints);

}

void FPEngine::_createGroundBuffers() {
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    float innerRadius = INNER_RADIUS;
    float outerRadius = OUTER_RADIUS;
    int numSegments = 100; // Increase for smoother ring

    // Generate vertices for the flat ring
    for (int i = 0; i <= numSegments; ++i) {
        float angle = (float)i / numSegments * 2.0f * M_PI;

        float xOuter = outerRadius * cos(angle);
        float zOuter = outerRadius * sin(angle);
        float uOuter = (cos(angle) + 1.0f) * 0.5f; // Map texture coordinates
        float vOuter = (sin(angle) + 1.0f) * 0.5f;

        float xInner = innerRadius * cos(angle);
        float zInner = innerRadius * sin(angle);
        float uInner = (cos(angle) * (innerRadius / outerRadius) + 1.0f) * 0.5f;
        float vInner = (sin(angle) * (innerRadius / outerRadius) + 1.0f) * 0.5f;

        // Outer vertex
        vertices.push_back(xOuter);
        vertices.push_back(0.0f); // Flat Y-coordinate
        vertices.push_back(zOuter);
        vertices.push_back(uOuter);
        vertices.push_back(vOuter);

        // Inner vertex
        vertices.push_back(xInner);
        vertices.push_back(0.0f); // Flat Y-coordinate
        vertices.push_back(zInner);
        vertices.push_back(uInner);
        vertices.push_back(vInner);
    }

    // Generate indices for the ring
    for (int i = 0; i < numSegments; ++i) {
        int outer1 = i * 2;
        int inner1 = outer1 + 1;
        int outer2 = (i + 1) * 2;
        int inner2 = outer2 + 1;

        // First triangle
        indices.push_back(outer1);
        indices.push_back(inner1);
        indices.push_back(outer2);

        // Second triangle
        indices.push_back(outer2);
        indices.push_back(inner1);
        indices.push_back(inner2);
    }

    // Create VAO and VBO
    glGenVertexArrays(1, &_groundVAO);
    glBindVertexArray(_groundVAO);

    GLuint groundVBO, groundEBO;
    glGenBuffers(1, &groundVBO);
    glGenBuffers(1, &groundEBO);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, groundEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // Bind vertex positions (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);

    // Bind texture coordinates (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

    glBindVertexArray(0);

    _numGroundPoints = static_cast<GLuint>(indices.size());
}


void FPEngine::_generateEnvironment() {
    fprintf(stdout, "[DEBUG]: Generating racetrack environment...\n");
    srand(static_cast<unsigned int>(time(0)));

    const float innerRadius = INNER_RADIUS;
    const float outerRadius = OUTER_RADIUS;
    const int numSegments = 20; // Number of segments for placing objects

    // Generate trees and spotlights along the inner and outer edges
    for (int i = 0; i < numSegments; ++i) {
        float angle = static_cast<float>(i) / numSegments * 2.0f * M_PI;
        float cosAngle = cos(angle);
        float sinAngle = sin(angle);

        glm::vec3 innerPosition(innerRadius * cosAngle, 0.0f, innerRadius * sinAngle);
        glm::vec3 outerPosition(outerRadius * cosAngle, 0.0f, outerRadius * sinAngle);

        if (i % 2 == 0) {
            // Place a tree
            glm::mat4 innerTreeMatrix = glm::translate(glm::mat4(1.0f), innerPosition);
            glm::mat4 innerLeavesMatrix = glm::translate(innerTreeMatrix, glm::vec3(0, 5, 0));
            _trees.emplace_back(TreeData{innerTreeMatrix, innerLeavesMatrix});

            glm::mat4 outerTreeMatrix = glm::translate(glm::mat4(1.0f), outerPosition);
            glm::mat4 outerLeavesMatrix = glm::translate(outerTreeMatrix, glm::vec3(0, 5, 0));
            _trees.emplace_back(TreeData{outerTreeMatrix, outerLeavesMatrix});
        } else {
            // Place a spotlight
            glm::mat4 innerLightMatrix = glm::translate(glm::mat4(1.0f), innerPosition);
            _lamps.emplace_back(LampData{

                .modelMatrixPost = innerLightMatrix,
                .modelMatrixLight = glm::translate(innerLightMatrix, glm::vec3(0, 7, 0)),
                .position = innerPosition
            });

            glm::mat4 outerLightMatrix = glm::translate(glm::mat4(1.0f), outerPosition);
            _lamps.emplace_back(LampData{
                .modelMatrixPost = outerLightMatrix,
                .modelMatrixLight = glm::translate(outerLightMatrix, glm::vec3(0, 7, 0)),
                .position = outerPosition,// Adjust height for light
            });
        }
    }
}


void FPEngine::mSetupScene() {
    // Create the Vehicle
    _pVehicle = new Vehicle(_lightingShaderProgram->getShaderProgramHandle(),
                            _lightingShaderUniformLocations.mvpMatrix,
                            _lightingShaderUniformLocations.normalMatrix,
                            _lightingShaderUniformLocations.materialAmbient,
                            _lightingShaderUniformLocations.materialDiffuse,
                            _lightingShaderUniformLocations.materialSpecular,
                            _lightingShaderUniformLocations.materialShininess);

    float initialAngle = 0.0f; // Start at the 0-degree mark of the track
    float initialHeading = glm::radians(180.0f);
    float startingRadius = (INNER_RADIUS + OUTER_RADIUS) / 2.0f; // Midpoint of the track
    float startX = startingRadius * cos(initialAngle);
    float startZ = startingRadius * sin(initialAngle);
    _pVehicle->setPosition(glm::vec3(startX, 0.0f, startZ));
    _pVehicle->setHeading(initialHeading);

    //initialize coins and marbles
    _initializeMarbleLocations();
    _initializeCoins();

    // Initialize cameras
    _pArcballCam = new ArcballCamera();
    _pArcballCam->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    _pArcballCam->rotate(0.0f, glm::radians(-30.0f)); // Initial angle

    _pFreeCam = new CSCI441::FreeCam();
    _pFreeCam->setPosition(glm::vec3(60.0f, 40.0f, 30.0f));
    _pFreeCam->setTheta(-M_PI / 3.0f);
    _pFreeCam->setPhi(M_PI / 2.8f);
    _pFreeCam->recomputeOrientation();
    _cameraSpeed = glm::vec2(0.25f, 0.02f);

    _pFPCam = new FPCamera(2.0f);
    _pFPCam->updatePositionAndOrientation(_pVehicle->getPosition(), _pVehicle->getHeading());

    _pTPCam = new TPCamera(); // Distance and height

    //spotlight
    _spotLight.pos = glm::vec3(startX, 10.0f, startZ);  // Position 10 units above the starting position
    _spotLight.dir = glm::vec3(0.0f, -1.0f, 0.0f);      // Pointing downwards
    _spotLight.width = glm::cos(glm::radians(15.0f));   // Spotlight cone width
    _spotLight.color = glm::vec3(1.0f, 0.0f, 0.0f);

    //jump
    _isJumping = false;
    _jumpProgress = 0.0f;

}


void FPEngine::_renderScene(glm::mat4 viewMtx, glm::mat4 projMtx) const {
    //for bezier
    int time = _bezierCurve.currentPos;
    float curveIndex= _bezierCurve.currentPos - time;

    //FOR THE GROUND
    // Use texture shader
    _textureShaderProgram->useProgram();

    glm::mat4 groundModelMtx = glm::mat4(1.0f);
    glm::mat4 mvpMtx = projMtx * viewMtx * groundModelMtx;

    // Set MVP matrix
    glUniformMatrix4fv(_textureShaderUniformLocations.mvpMatrix, 1, GL_FALSE, glm::value_ptr(mvpMtx));

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texHandles[TEXTURE_ID::RUG]);
    glUniform1i(_textureShaderUniformLocations.aTextMap, 0);

    // Bind and draw ground VAO
    glBindVertexArray(_groundVAO);
    glDrawElements(GL_TRIANGLES, _numGroundPoints, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // Send spotlight information
    glUniform3fv(_textureShaderUniformLocations.spotLightPosition, 1, glm::value_ptr(_spotLight.pos));
    glUniform3fv(_textureShaderUniformLocations.spotLightDirection, 1, glm::value_ptr(_spotLight.dir));
    glUniform1f(_textureShaderUniformLocations.spotLightWidth, _spotLight.width);
    glUniform3fv(_textureShaderUniformLocations.spotLightColor, 1, glm::value_ptr(_spotLight.color));

    const int MAX_POINT_LIGHTS = 10;

    // Arrays to hold point light data
    glm::vec3 pointLightPositions[MAX_POINT_LIGHTS];
    glm::vec3 pointLightColors[MAX_POINT_LIGHTS];
    float pointLightConstants[MAX_POINT_LIGHTS];
    float pointLightLinears[MAX_POINT_LIGHTS];
    float pointLightQuadratics[MAX_POINT_LIGHTS];

    // Determine the number of point lights
    int numPointLights = std::min(static_cast<int>(_lamps.size()), MAX_POINT_LIGHTS);

    // Populate the point light arrays
    for(int i = 0; i < numPointLights; ++i) {
        pointLightPositions[i] = _lamps[i].position;
        pointLightColors[i] = glm::vec3(0.0f, 0.0f, 1.0f); // Blue color
        pointLightConstants[i] = 1.0f;
        pointLightLinears[i] = 0.09f;
        pointLightQuadratics[i] = 0.032f;
    }

    _lightingShaderProgram->useProgram();
    glm::vec3 cameraPosition;
    if (currCamera == CameraType::ARCBALL) {
        cameraPosition = _pArcballCam->getPosition();
    } else if (currCamera == CameraType::FREECAM) {
        cameraPosition = _pFreeCam->getPosition();
    } else if (currCamera == CameraType::FIRSTPERSON) {
        cameraPosition = _pFPCam->getPosition();
    } else if (currCamera == CameraType::THIRDPERSON) {
        cameraPosition = _pTPCam->getPosition();
    }

    // Send the camera position to the shader
    glUniform3fv(_lightingShaderUniformLocations.viewPos, 1, glm::value_ptr(cameraPosition));

    // Set directional light uniforms using the correct uniform names and locations
    glm::vec3 lightDirection(-1.0f, -1.0f, -1.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    glUniform3fv(_lightingShaderUniformLocations.dirLightDirection, 1, glm::value_ptr(lightDirection));
    glUniform3fv(_lightingShaderUniformLocations.dirLightColor, 1, glm::value_ptr(lightColor));

    glUniform1i(_lightingShaderUniformLocations.numPointLights, numPointLights);
    glUniform3fv(_lightingShaderUniformLocations.pointLightPositions, numPointLights, glm::value_ptr(pointLightPositions[0]));
    glUniform3fv(_lightingShaderUniformLocations.pointLightColors, numPointLights, glm::value_ptr(pointLightColors[0]));
    glUniform1fv(_lightingShaderUniformLocations.pointLightConstants, numPointLights, pointLightConstants);
    glUniform1fv(_lightingShaderUniformLocations.pointLightLinears, numPointLights, pointLightLinears);
    glUniform1fv(_lightingShaderUniformLocations.pointLightQuadratics, numPointLights, pointLightQuadratics);

    // Set spot light uniforms
    glUniform3fv(_lightingShaderUniformLocations.spotLightPosition, 1, glm::value_ptr(_spotLight.pos));
    glUniform3fv(_lightingShaderUniformLocations.spotLightDirection, 1, glm::value_ptr(_spotLight.dir));
    glUniform3fv(_lightingShaderUniformLocations.spotLightColor, 1, glm::value_ptr(_spotLight.color));
    glUniform1f(_lightingShaderUniformLocations.spotLightWidth, _spotLight.width);

    //// BEGIN DRAWING THE TREES ////
    // Draw trunks
    for(const TreeData& tree : _trees){
        _computeAndSendMatrixUniforms(tree.modelMatrixTrunk, viewMtx, projMtx);

        // Set material properties for tree trunks
        glm::vec3 trunkAmbient(0.2f, 0.2f, 0.2f);
        glm::vec3 trunkDiffuse(99 / 255.f, 39 / 255.f, 9 / 255.f);
        glm::vec3 trunkSpecular(0.3f, 0.3f, 0.3f);
        float trunkShininess = 32.0f;

        glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(trunkAmbient));
        glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(trunkDiffuse));
        glUniform3fv(_lightingShaderUniformLocations.materialSpecular, 1, glm::value_ptr(trunkSpecular));
        glUniform1f(_lightingShaderUniformLocations.materialShininess, trunkShininess);
        _computeAndSendMatrixUniforms(tree.modelMatrixTrunk, viewMtx, projMtx);
        CSCI441::drawSolidCylinder(1, 1, 5, 16, 16);
    }

    // Draw leaves
    for(const TreeData& tree : _trees){
        _computeAndSendMatrixUniforms(tree.modelMatrixLeaves, viewMtx, projMtx);

        // Set material properties for tree leaves
        glm::vec3 leavesAmbient(0.2f, 0.2f, 0.2f);
        glm::vec3 leavesDiffuse(46 / 255.f, 143 / 255.f, 41 / 255.f);
        glm::vec3 leavesSpecular(0.3f, 0.3f, 0.3f);
        float leavesShininess = 32.0f;

        glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(leavesAmbient));
        glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(leavesDiffuse));
        glUniform3fv(_lightingShaderUniformLocations.materialSpecular, 1, glm::value_ptr(leavesSpecular));
        glUniform1f(_lightingShaderUniformLocations.materialShininess, leavesShininess);
        _computeAndSendMatrixUniforms(tree.modelMatrixLeaves, viewMtx, projMtx);
        CSCI441::drawSolidCone(3, 8, 16, 16);
    }
    //// END DRAWING THE TREES ////

    //// BEGIN DRAWING THE LAMPS ////
    // Draw posts
    for (const LampData &lamp: _lamps) {
        _computeAndSendMatrixUniforms(lamp.modelMatrixPost, viewMtx, projMtx);

        // Set material properties for lamp posts
        glm::vec3 postAmbient(0.2f, 0.2f, 0.2f);
        glm::vec3 postDiffuse(0.5f, 0.5f, 0.5f);
        glm::vec3 postSpecular(0.3f, 0.3f, 0.3f);
        float postShininess = 32.0f;

        glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(postAmbient));
        glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(postDiffuse));
        glUniform3fv(_lightingShaderUniformLocations.materialSpecular, 1, glm::value_ptr(postSpecular));
        glUniform1f(_lightingShaderUniformLocations.materialShininess, postShininess);

        CSCI441::drawSolidCylinder(0.2, 0.2, 7, 16, 16);
    }

    // Draw lights
    for (const LampData &lamp: _lamps) {
        _computeAndSendMatrixUniforms(lamp.modelMatrixLight, viewMtx, projMtx);

        // Set material properties for lamp lights
        glm::vec3 lightAmbient(0.2f, 0.2f, 0.5f);
        glm::vec3 lightDiffuse(0.0f, 0.0f, 1.0f); // Blue color
        glm::vec3 lightSpecular(0.5f, 0.5f, 0.5f);
        float lightShininess = 64.0f;

        glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(lightAmbient));
        glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(lightDiffuse));
        glUniform3fv(_lightingShaderUniformLocations.materialSpecular, 1, glm::value_ptr(lightSpecular));
        glUniform1f(_lightingShaderUniformLocations.materialShininess, lightShininess);

        CSCI441::drawSolidSphere(0.5, 16, 16);
    }
    //// END DRAWING THE LAMPS ////

    _pVehicle->drawVehicle(viewMtx, projMtx);

    //draw marbles
    _drawMarbles(viewMtx, projMtx);

    //draw coins
    _drawCoins(viewMtx, projMtx);

    //draw arch
    _drawArch(viewMtx, projMtx);

    //bezier curve
    // Render Bézier Curve
    _lightingShaderProgram->useProgram();

    glm::mat4 curveModelMtx = glm::mat4(1.0f);
    glm::mat4 curveMVP = projMtx * viewMtx * curveModelMtx;
    glUniformMatrix4fv(_lightingShaderUniformLocations.mvpMatrix, 1, GL_FALSE, glm::value_ptr(curveMVP));

}

void FPEngine::_updateScene() {
    const float FALL_BOUNDARY = OUTER_RADIUS; // Match ground size
    const glm::vec3 START_POSITION((INNER_RADIUS + OUTER_RADIUS) / 2.0f, 0.0f, 0.0f);
    const float BLINKING_DURATION = 3.0f; // Total blinking duration in seconds
    _animationTime += 0.016f;

    glm::vec3 currentPosition = _pVehicle->getPosition();
    glm::vec3 newPosition = currentPosition; // Start with the current position
    float vehicleRadius = _pVehicle->getBoundingRadius();

    // Spotlight hover over the vehicle
    _spotLight.pos = currentPosition + glm::vec3(0.0f, 10.0f, 0.0f);
    _spotLight.dir = glm::vec3(0.0f, -1.0f, 0.0f);


    _updateActiveCamera();
    //handle jump
    if (_isJumping) {
        _jumpProgress = glm::clamp(_jumpProgress + 0.02f, 0.0f, 1.0f); // Keep progress within bounds

        glm::vec3 jumpPosition = _evalBezierCurve(
            _jumpControlPoints[0],
            _jumpControlPoints[1],
            _jumpControlPoints[2],
            _jumpControlPoints[3],
            _jumpProgress
        );

        _pVehicle->setPosition(jumpPosition);

        if (_jumpProgress >= 1.0f) {
            _isJumping = false;
            _jumpProgress = 0.0f; // Reset progress for next jump
        }
        fprintf(stdout, "[DEBUG]: Jump Progress: %.2f, Position: (%.2f, %.2f, %.2f)\n",
        _jumpProgress, jumpPosition.x, jumpPosition.y, jumpPosition.z);

    }

    // Handle coin collisions
    for (auto it = _coins.begin(); it != _coins.end();) {
        const glm::vec3& coinPosition = it->getPosition();
        float coinRadius = it->getSize(); // Coin size
        float distance = glm::length(currentPosition - coinPosition);

        if (checkCollision(currentPosition, vehicleRadius, coinPosition, coinRadius)) {
            fprintf(stdout, "[INFO]: Coin collected! Removing coin at position (%.2f, %.2f, %.2f)\n",
                    coinPosition.x, coinPosition.y, coinPosition.z);
            _pVehicle->setCoinCount(_pVehicle->getCoinCount() + 1);
            it = _coins.erase(it);
        } else {
            ++it;
        }
    }

    if (_coins.empty()) {
        fprintf(stdout, "[INFO]: All coins collected! Deleting all enemies.\n");

        // Clear the enemies vector
        _enemies->clear();
        for (int i = 0; i < NUM_MARBLES; ++i) {
            _marbleLocations[i] = glm::vec3(0, 0, 0); // Set to default position
        }

        // If you want to ensure marble directions are reset as well
        for (int i = 0; i < NUM_MARBLES; ++i) {
            _marbleDirections[i] = glm::vec3(0, 0, 0); // Reset directions to prevent movement
        }
    }

    // Handle marble movement
    for (int i = 0; i < 4; ++i) { // Only update the first 4 marbles
        _marbleLocations[i] += glm::vec3(
            (getRand() - 0.5f) * 0.1f, // Random x direction
            0.0f,                     // No y movement
            (getRand() - 0.5f) * 0.1f  // Random z direction
        );
    }

    // Update VBO
    glBindBuffer(GL_ARRAY_BUFFER, _marbleVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(_marbleLocations), _marbleLocations);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    _moveMarbles();
    _collideMarblesWithWall();
    _collideMarblesWithMarbles();

    if (!_isFalling && !_isBlinking && !_isJumping) {
        // Handle player movement
        glm::vec3 movementVector(0.0f);
        if (_keys[GLFW_KEY_W]) movementVector += glm::vec3(sin(_pVehicle->getHeading()), 0.0f, cos(_pVehicle->getHeading())) * 0.2f;
        if (_keys[GLFW_KEY_S]) movementVector += glm::vec3(-sin(_pVehicle->getHeading()), 0.0f, -cos(_pVehicle->getHeading())) * 0.2f;
        if (_keys[GLFW_KEY_A]) _pVehicle->turnLeft();
        if (_keys[GLFW_KEY_D]) _pVehicle->turnRight();

        newPosition += movementVector;

        if (!isMovementValid(newPosition)) newPosition -= movementVector * 1.5f;

        // Check for collisions with marbles
        for (int i = 0; i < 4; ++i) {
            if (checkCollision(currentPosition, vehicleRadius, _marbleLocations[i], MARBLE_RADIUS)) {
                _isBlinking = true;
                _blinkTimer = 0.0f;
                _blinkingTime = 0.0f; // Track blinking duration
                _blinkCount = 0;
            }
        }

        bool isOffPlatform = glm::abs(newPosition.x) > (FALL_BOUNDARY) ||
                             glm::abs(newPosition.z) > (FALL_BOUNDARY);

        if (isOffPlatform) {
            _isFalling = true;
            _fallTime = 0.0f;
        }

        if (!_isBlinking) {
            _pVehicle->setPosition(newPosition);

        }
    } else if (_isBlinking) {
        // Handle blinking
        _blinkingTime += 0.016f;
        _blinkTimer += 0.016f;

        if (_blinkTimer >= 0.2f) { // Toggle visibility every 0.2 seconds
            _blinkTimer = 0.0f;
            _blinkCount++;
            _pVehicle->toggleVisibility();
        }

        if (_blinkingTime >= BLINKING_DURATION) { // After blinking duration
            _isBlinking = false;

            // Reset vehicle and marbles
            _pVehicle->setPosition(START_POSITION);
            for (int i = 0; i < 4; ++i) {
                _marbleLocations[i] = glm::vec3(
                    (i % 2 == 0 ? -WORLD_SIZE : WORLD_SIZE) / 2.0f,
                    Marble::RADIUS,
                    (i / 2 == 0 ? -WORLD_SIZE : WORLD_SIZE) / 2.0f
                );
                _marbleDirections[i] = glm::normalize(glm::vec3(0.0f) - _marbleLocations[i]); // Point to center
            }
        }
    } else if (_isFalling) {
        // Handle falling
        _fallTime += 0.016f;
        _pVehicle->animateFall(_fallTime);
        if (_fallTime > 3.0f) {
            _pVehicle->setPosition(START_POSITION);
            _isFalling = false;
            _initializeMarbleLocations(); // Reset marbles
            if (currCamera == CameraType::FIRSTPERSON) _pFPCam->updatePositionAndOrientation(START_POSITION, _pVehicle->getHeading());
            else if (currCamera == CameraType::ARCBALL) _pArcballCam->setTarget(START_POSITION);
            else if (currCamera == CameraType::THIRDPERSON) _pTPCam->update(START_POSITION, _pVehicle->getHeading());

        }
    }
}


void FPEngine::run() {
    while (!glfwWindowShouldClose(mpWindow)) {
        glDrawBuffer(GL_BACK);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Get framebuffer dimensions
        GLint framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize(mpWindow, &framebufferWidth, &framebufferHeight);

        // Set the main viewport
        glViewport(0, 0, framebufferWidth, framebufferHeight);

        // Render the main scene
        glm::mat4 projMtx;
        glm::mat4 viewMtx;

        if (currCamera == CameraType::ARCBALL) {
            projMtx = glm::perspective(glm::radians(45.0f), static_cast<float>(framebufferWidth) / framebufferHeight, 0.1f, 100.0f);
            viewMtx = _pArcballCam->getViewMatrix();
        } else if (currCamera == CameraType::FREECAM) {
            projMtx = _pFreeCam->getProjectionMatrix();
            viewMtx = _pFreeCam->getViewMatrix();
        } else if (currCamera == CameraType::FIRSTPERSON) {
            projMtx = glm::perspective(glm::radians(45.0f), static_cast<float>(framebufferWidth) / framebufferHeight, 0.1f, 100.0f);
            viewMtx = _pFPCam->getViewMatrix();
        } else if (currCamera == CameraType::THIRDPERSON) {
            projMtx = glm::perspective(glm::radians(45.0f), static_cast<float>(framebufferWidth) / framebufferHeight, 0.1f, 100.0f);
            _pTPCam->update(_pVehicle->getPosition(), _pVehicle->getHeading());
            viewMtx = _pTPCam->getViewMatrix();
        }

        _renderScene(viewMtx, projMtx);

        // Render the minimap
        _renderMinimap();

        // Update the scene
        _updateScene();

        glfwSwapBuffers(mpWindow);
        glfwPollEvents();
    }
}

void FPEngine::_renderMinimap() const {
    // Get framebuffer dimensions
    GLint framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(mpWindow, &framebufferWidth, &framebufferHeight);

    // Calculate minimap dimensions (1/5 of the screen size)
    GLint minimapWidth = framebufferWidth / 5;
    GLint minimapHeight = framebufferHeight / 5;

    // Set the minimap viewport to the top-right corner
    glViewport(framebufferWidth - minimapWidth, framebufferHeight - minimapHeight, minimapWidth, minimapHeight);

    // Disable depth testing for the minimap
    glDisable(GL_DEPTH_TEST);

    // Clear only the depth buffer to preserve the main scene's color
    glClear(GL_DEPTH_BUFFER_BIT);

    // Set up an orthographic projection for the minimap (top-down view)
    glm::mat4 projMtx = glm::ortho(
        -WORLD_SIZE / 2.0f, WORLD_SIZE / 2.0f,
        -WORLD_SIZE / 2.0f, WORLD_SIZE / 2.0f,
        -10.0f, 10.0f
    );

    // Flip the map by rotating the view matrix 180 degrees around the Y-axis
    glm::mat4 viewMtx = glm::lookAt(
        glm::vec3(0.0f, 10.0f, 0.0f),  // Camera positioned directly above
        glm::vec3(0.0f, 0.0f, 0.0f),   // Looking at the center of the platform
        glm::vec3(0.0f, 0.0f, -1.0f)   // "Up" direction for the minimap
    );
    viewMtx = glm::rotate(viewMtx, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Render the ground in the minimap
    _textureShaderProgram->useProgram();
    glm::mat4 groundModelMtx = glm::mat4(1.0f); // Identity matrix for ground
    glm::mat4 groundMVP = projMtx * viewMtx * groundModelMtx;
    glUniformMatrix4fv(_textureShaderUniformLocations.mvpMatrix, 1, GL_FALSE, glm::value_ptr(groundMVP));

    // Bind ground texture and VAO
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texHandles[TEXTURE_ID::RUG]);
    glUniform1i(_textureShaderUniformLocations.aTextMap, 0);
    glBindVertexArray(_groundVAO);

    // Draw the ground using the correct index count
    glDrawElements(GL_TRIANGLES, _numGroundPoints, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // Render the player as a green square in the minimap
    _lightingShaderProgram->useProgram();
    glm::mat4 playerModelMtx = glm::translate(glm::mat4(1.0f), _pVehicle->getPosition());
    playerModelMtx = glm::scale(playerModelMtx, glm::vec3(5.0f)); // Size for minimap
    glm::mat4 playerMVP = projMtx * viewMtx * playerModelMtx;
    glUniformMatrix4fv(_lightingShaderUniformLocations.mvpMatrix, 1, GL_FALSE, glm::value_ptr(playerMVP));
    glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f))); // Green color
    glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f)));
    CSCI441::drawSolidCube(1.0f);

    // Render enemies as red squares in the minimap
    for (const glm::vec3& enemyPosition : _marbleLocations) {
        if (enemyPosition != glm::vec3(0.0f, 0.0f, 0.0f)) { // Skip invalid positions
            glm::mat4 enemyModelMtx = glm::translate(glm::mat4(1.0f), enemyPosition);
            enemyModelMtx = glm::scale(enemyModelMtx, glm::vec3(5.0f)); // Size for minimap
            glm::mat4 enemyMVP = projMtx * viewMtx * enemyModelMtx;
            glUniformMatrix4fv(_lightingShaderUniformLocations.mvpMatrix, 1, GL_FALSE, glm::value_ptr(enemyMVP));
            glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(glm::vec3(1.0f, 0.0f, 0.0f))); // Red color
            glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(glm::vec3(1.0f, 0.0f, 0.0f)));
            CSCI441::drawSolidCube(1.0f);
        }
    }

    // Render coins as yellow squares in the minimap
    for (const Coin& coin : _coins) {
        glm::mat4 coinModelMtx = glm::translate(glm::mat4(1.0f), coin.getPosition());
        coinModelMtx = glm::scale(coinModelMtx, glm::vec3(5.0f)); // Size for minimap
        glm::mat4 coinMVP = projMtx * viewMtx * coinModelMtx;
        glUniformMatrix4fv(_lightingShaderUniformLocations.mvpMatrix, 1, GL_FALSE, glm::value_ptr(coinMVP));
        glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.0f))); // Yellow color
        glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.0f)));
        CSCI441::drawSolidCube(1.0f);
    }

    // Re-enable depth testing after rendering the minimap
    glEnable(GL_DEPTH_TEST);

    // Reset the viewport to the full screen
    glViewport(0, 0, framebufferWidth, framebufferHeight);
}

void FPEngine::_drawCoins(glm::mat4 viewMtx, glm::mat4 projMtx) const {
    _lightingShaderProgram->useProgram();
    for (const Coin& coin : _coins) {
        glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), coin.getPosition());
        modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate to align with z-axis
        modelMatrix = glm::scale(modelMatrix, glm::vec3(coin.getSize(), coin.getSize(), coin.getSize() * 0.4f)); // Thicker coins
        glm::mat4 mvpMatrix = projMtx * viewMtx * modelMatrix;

        // Send uniforms for MVP and material properties
        glUniformMatrix4fv(_lightingShaderUniformLocations.mvpMatrix, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
        glUniformMatrix4fv(_lightingShaderUniformLocations.modelMatrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix3fv(_lightingShaderUniformLocations.normalMatrix, 1, GL_FALSE,
                           glm::value_ptr(glm::transpose(glm::inverse(glm::mat3(modelMatrix)))));

        // Set consistent gold material properties
        glm::vec3 goldColor(1.0f, 0.84f, 0.0f); // Gold color
        glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(goldColor * 0.3f));
        glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(goldColor));
        glUniform3fv(_lightingShaderUniformLocations.materialSpecular, 1, glm::value_ptr(glm::vec3(0.8f)));
        glUniform1f(_lightingShaderUniformLocations.materialShininess, 64.0f);

        // Draw the main body of the coin (thicker cylinder)
        CSCI441::drawSolidCylinder(0.5f, 0.5f, 0.4f, 32, 32); // Increased height to 0.4

        // Top cap
        glm::mat4 topCapMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.2f)); // Adjust position for top cap
        glUniformMatrix4fv(_lightingShaderUniformLocations.modelMatrix, 1, GL_FALSE, glm::value_ptr(topCapMatrix));
        CSCI441::drawSolidDisk(0.0f, 0.5f, 32, 1);

        // Bottom cap
        glm::mat4 bottomCapMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, -0.2f)); // Adjust position for bottom cap
        glUniformMatrix4fv(_lightingShaderUniformLocations.modelMatrix, 1, GL_FALSE, glm::value_ptr(bottomCapMatrix));
        CSCI441::drawSolidDisk(0.0f, 0.5f, 32, 1);
    }
}

void FPEngine::_drawMarbles(glm::mat4 viewMtx, glm::mat4 projMtx) const {
    _lightingShaderProgram->useProgram();
    for (int i = 0; i < 4; ++i) {
        glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), _marbleLocations[i]);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(Marble::RADIUS)); // Scale marble
        glm::mat4 mvpMatrix = projMtx * viewMtx * modelMatrix;

        // Set material properties for marble
        glm::vec3 marbleColor(1.0f, 0.0f, 0.0f);
        glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(marbleColor * 0.3f));
        glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(marbleColor));
        glUniform3fv(_lightingShaderUniformLocations.materialSpecular, 1, glm::value_ptr(glm::vec3(0.5f)));
        glUniform1f(_lightingShaderUniformLocations.materialShininess, 32.0f);

        glUniformMatrix4fv(_lightingShaderUniformLocations.mvpMatrix, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
        CSCI441::drawSolidSphere(Marble::RADIUS, 16, 16);

        // Render the animated beak
        _animateBeak(i, viewMtx, projMtx);
    }
}

void FPEngine::_drawBeakTriangle(bool isTop) const {
    GLfloat beakVertices[] = {
        0.0f, 0.2f, 0.0f,  // Tip of the triangle
        -0.1f, 0.0f, 0.15f, // Left corner
        0.1f, 0.0f, 0.15f   // Right corner
    };

    if (!isTop) {
        for (int i = 1; i < 9; i += 3) {
            beakVertices[i] -= 0.15f; // Lower the triangle for the bottom beak
        }
    }

    GLuint beakVBO;
    glGenBuffers(1, &beakVBO);
    glBindBuffer(GL_ARRAY_BUFFER, beakVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(beakVertices), beakVertices, GL_STATIC_DRAW);

    // Bind and enable vertex attributes
    glEnableVertexAttribArray(_lightingShaderAttributeLocations.vPos);
    glVertexAttribPointer(_lightingShaderAttributeLocations.vPos, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Cleanup
    glDisableVertexAttribArray(_lightingShaderAttributeLocations.vPos);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &beakVBO);
}


void FPEngine::_animateBeak(int marbleIndex, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    // Oscillation
    float beakOffset = glm::sin(_animationTime * 2.0f) * 0.05f; // Faster oscillation

    // Rotation
    float rotationAngle = _animationTime * glm::radians(45.0f); // Rotate 45 degrees per second
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));

    // Scaling (pulsating)
    float scaleFactor = 1.0f + glm::sin(_animationTime * 3.0f) * 0.1f; // Pulsate between 1.0 and 1.1
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor));

    // Base transformation (marble position)
    glm::mat4 baseMatrix = glm::translate(glm::mat4(1.0f), _marbleLocations[marbleIndex]);

    // Apply transformations for the top beak
    glm::mat4 topBeakMatrix = baseMatrix * rotationMatrix * scaleMatrix;
    topBeakMatrix = glm::translate(topBeakMatrix, glm::vec3(0.0f, 0.1f + beakOffset, Marble::RADIUS));
    glm::mat4 topMVP = projMtx * viewMtx * topBeakMatrix;

    // Send matrix to the shader and draw the top triangle
    glUniformMatrix4fv(_lightingShaderUniformLocations.mvpMatrix, 1, GL_FALSE, glm::value_ptr(topMVP));
    _drawBeakTriangle(true);

    // Apply transformations for the bottom beak
    glm::mat4 bottomBeakMatrix = baseMatrix * rotationMatrix * scaleMatrix;
    bottomBeakMatrix = glm::translate(bottomBeakMatrix, glm::vec3(0.0f, -0.1f, Marble::RADIUS));
    glm::mat4 bottomMVP = projMtx * viewMtx * bottomBeakMatrix;

    // Send matrix to the shader and draw the bottom triangle
    glUniformMatrix4fv(_lightingShaderUniformLocations.mvpMatrix, 1, GL_FALSE, glm::value_ptr(bottomMVP));
    _drawBeakTriangle(false);
}

//*************************************************************************************
//
// Engine Cleanup

void FPEngine::mCleanupShaders() {
    fprintf( stdout, "[INFO]: ...deleting Shaders.\n" );
    delete _lightingShaderProgram;
    delete _textureShaderProgram;
}

void FPEngine::mCleanupBuffers() {
    fprintf( stdout, "[INFO]: ...deleting VAOs....\n" );
    CSCI441::deleteObjectVAOs();
    glDeleteVertexArrays( 1, &_groundVAO );

    fprintf( stdout, "[INFO]: ...deleting VBOs....\n" );
    CSCI441::deleteObjectVBOs();

    fprintf( stdout, "[INFO]: ...deleting models..\n" );
    delete _pVehicle;

    glDeleteVertexArrays(1, &_archVAO);
}

void FPEngine::mCleanupTextures() {
    fprintf( stdout, "[INFO]: ...deleting textures\n" );
    // TODO #23 - delete textures
    glDeleteTextures(1, &_texHandles[RUG]);

}

//*************************************************************************************
//
// Private Helper Functions

void FPEngine::_drawArch(glm::mat4 viewMtx, glm::mat4 projMtx) const {
    _lightingShaderProgram->useProgram();

glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(25.0f, 0.0f, 0.0f)); // Position on one side

    glm::mat4 mvpMatrix = projMtx * viewMtx * modelMatrix;

    glUniformMatrix4fv(_lightingShaderUniformLocations.mvpMatrix, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

    // Material properties for the arch
    glm::vec3 archColor(0.0f, 0.0f, 1.0f);
    glUniform3fv(_lightingShaderUniformLocations.materialAmbient, 1, glm::value_ptr(archColor * 0.3f));
    glUniform3fv(_lightingShaderUniformLocations.materialDiffuse, 1, glm::value_ptr(archColor));
    glUniform3fv(_lightingShaderUniformLocations.materialSpecular, 1, glm::value_ptr(glm::vec3(0.2f)));
    glUniform1f(_lightingShaderUniformLocations.materialShininess, 32.0f);

    glBindVertexArray(_archVAO);
    glDrawElements(GL_TRIANGLES, _numArchPoints, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}


void FPEngine::_createArchBuffers() {
    const int NUM_SEGMENTS = 100;   // Increase for smoother arch
    const float MAX_HEIGHT = 10.0f; // Maximum height of the arch
    const float START_ANGLE = M_PI; // Starting angle (180 degrees)
    const float END_ANGLE = 2.0f * M_PI; // Ending angle (360 degrees)
    const float INNER_RADIUS = 13.0f;          // Starting radius
    const float OUTER_RADIUS = 15.0f;
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    // Generate vertices for the arch
    for (int i = 0; i <= NUM_SEGMENTS; ++i) {
        float t = static_cast<float>(i) / NUM_SEGMENTS;
        float angle = START_ANGLE + t * (END_ANGLE - START_ANGLE);

        // Height follows a parabolic curve
        float height = MAX_HEIGHT * (1.0f - pow(2.0f * t - 1.0f, 2.0f));

        // Inner point of the arch
        float xInner = INNER_RADIUS * cos(angle);
        float zInner = INNER_RADIUS * sin(angle);
        vertices.push_back(xInner);
        vertices.push_back(height);
        vertices.push_back(zInner);

        // Outer point of the arch
        float xOuter = OUTER_RADIUS * cos(angle);
        float zOuter = OUTER_RADIUS * sin(angle);
        vertices.push_back(xOuter);
        vertices.push_back(height);
        vertices.push_back(zOuter);

        if (i < NUM_SEGMENTS) {
            int baseIndex = i * 2;
            // Create triangles
            indices.push_back(baseIndex);
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 2);

            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 3);
            indices.push_back(baseIndex + 2);
        }
    }

    // Create VAO and VBO for the arch
    GLuint archVBO, archEBO;
    glGenVertexArrays(1, &_archVAO);
    glBindVertexArray(_archVAO);

    glGenBuffers(1, &archVBO);
    glBindBuffer(GL_ARRAY_BUFFER, archVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &archEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, archEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); // Vertex positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);

    glBindVertexArray(0);

    _numArchPoints = static_cast<GLuint>(indices.size());
}



void FPEngine::_initializeCoins() {
    const int NUM_COINS = 10; // Number of coins to spawn
    const float innerRadius = 15.0f; // Midway between inner and outer edge
    const float outerRadius = 17.0f; // Slightly offset for variety

    for (int i = 0; i < NUM_COINS; ++i) {
        float angle = static_cast<float>(i) / NUM_COINS * 2.0f * M_PI; // Spread coins evenly
        float radius = (i % 2 == 0) ? innerRadius : outerRadius;       // Alternate between two radii
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        glm::vec3 position(x, 1.0f, z); // Coins slightly above the ground
        _coins.emplace_back(position, 1.0f); // Fixed size
    }
}


void FPEngine::_initializeMarbleLocations() {
    // Initialize marbles at the four corners of the platform
    _marbleLocations[0] = glm::vec3(-WORLD_SIZE / 2.0f, Marble::RADIUS, -WORLD_SIZE / 2.0f); // Bottom-left
    _marbleLocations[1] = glm::vec3(-WORLD_SIZE / 2.0f, Marble::RADIUS, WORLD_SIZE / 2.0f);  // Top-left
    _marbleLocations[2] = glm::vec3(WORLD_SIZE / 2.0f, Marble::RADIUS, -WORLD_SIZE / 2.0f);  // Bottom-right
    _marbleLocations[3] = glm::vec3(WORLD_SIZE / 2.0f, Marble::RADIUS, WORLD_SIZE / 2.0f);   // Top-right

    // Initialize directions to point toward the center (initial vehicle location)
    for (int i = 0; i < 4; ++i) {
        _marbleDirections[i] = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - _marbleLocations[i]); // Center is (0,0,0)
    }
}

void FPEngine::_collideMarblesWithWall() {
    for (int i = 0; i < NUM_MARBLES; ++i) {
        if (_marbleLocations[i].x > WORLD_SIZE / 2.0f - Marble::RADIUS || _marbleLocations[i].x < -WORLD_SIZE / 2.0f + MARBLE_RADIUS) {
            _marbleDirections[i].x *= -1.0f;
        }
        if (_marbleLocations[i].z > WORLD_SIZE / 2.0f - Marble::RADIUS || _marbleLocations[i].z < -WORLD_SIZE / 2.0f + MARBLE_RADIUS) {
            _marbleDirections[i].z *= -1.0f;
        }
    }
}

void FPEngine::_moveMarbles() {
    glm::vec3 heroPosition = _pVehicle->getPosition();

    for (int i = 0; i < NUM_MARBLES; ++i) {
        glm::vec3 toHero = glm::normalize(heroPosition - _marbleLocations[i]); // Vector pointing to Hero
        glm::vec3 currentDirection = _marbleDirections[i];                    // Current heading of the marble

        // Calculate the angular step toward the Hero
        const float angleStep = 0.05f; // Adjust this value to control turning speed
        float dotProduct = glm::dot(currentDirection, toHero); // Cosine of the angle between current heading and target direction
        dotProduct = glm::clamp(dotProduct, -1.0f, 1.0f); // Clamp to avoid numerical issues
        float angleToHero = acos(dotProduct);             // Calculate the angle to the Hero

        // Check if adjustment is needed
        if (angleToHero > 0.001f) { // Threshold to avoid jitter when nearly aligned
            glm::vec3 rotationAxis = glm::normalize(glm::cross(currentDirection, toHero)); // Axis of rotation
            if (glm::length(rotationAxis) > 0.0f) { // Ensure the vectors are not parallel
                glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::min(angleStep, angleToHero), rotationAxis);
                glm::vec4 newDirection = rotationMatrix * glm::vec4(currentDirection, 0.0f); // Apply the rotation
                _marbleDirections[i] = glm::normalize(glm::vec3(newDirection)); // Update marble heading
            }
        }

        // Move the marble forward along its new heading
        _marbleLocations[i] += _marbleDirections[i] * MARBLE_SPEED * 0.3f;

        // Keep the marbles at the correct height (on the ground)
        _marbleLocations[i].y = Marble::RADIUS;
    }
}


void FPEngine::_collideMarblesWithMarbles() {
    for (int i = 0; i < NUM_MARBLES; ++i) {
        for (int j = i + 1; j < NUM_MARBLES; ++j) {
            glm::vec3 diff = _marbleLocations[j] - _marbleLocations[i];
            float dist = glm::length(diff);
            if (dist < 2 * Marble::RADIUS) {
                glm::vec3 normal = glm::normalize(diff);
                glm::vec3 relativeVel = _marbleDirections[j] - _marbleDirections[i];
                float dot = glm::dot(relativeVel, normal);
                _marbleDirections[i] += dot * normal;
                _marbleDirections[j] -= dot * normal;
            }
        }
    }
}

void FPEngine::_updateActiveCamera() {
    glm::vec3 vehiclePosition = _pVehicle->getPosition();
    float vehicleHeading = _pVehicle->getHeading();

    if (currCamera == CameraType::FIRSTPERSON) {
        _pFPCam->updatePositionAndOrientation(vehiclePosition, vehicleHeading);
    } else if (currCamera == CameraType::THIRDPERSON) {
        _pTPCam->update(vehiclePosition, vehicleHeading);
    } else if (currCamera == CameraType::ARCBALL) {
        _pArcballCam->setTarget(vehiclePosition);
    }
}

void FPEngine::generateTorusMesh(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices,
                       float innerRadius, float outerRadius, int numSides, int numRings) {
    for (int ring = 0; ring <= numRings; ++ring) {
        float theta = (float)ring / numRings * 2.0f * M_PI;
        float cosTheta = cos(theta);
        float sinTheta = sin(theta);

        for (int side = 0; side <= numSides; ++side) {
            float phi = (float)side / numSides * 2.0f * M_PI;
            float cosPhi = cos(phi);
            float sinPhi = sin(phi);

            float x = (outerRadius + innerRadius * cosPhi) * cosTheta;
            float y = innerRadius * sinPhi;
            float z = (outerRadius + innerRadius * cosPhi) * sinTheta;

            f