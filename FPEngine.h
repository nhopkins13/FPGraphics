
#ifndef MP_ENGINE_H
#define MP_ENGINE_H

#include <CSCI441/OpenGLEngine.hpp>
#include <CSCI441/ShaderProgram.hpp>
#include <CSCI441/objects.hpp>
#include <CSCI441/FreeCam.hpp>
#include <stb_image.h>
#include <ctime>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string.h>
#include <vector>

//#include "FPSCamera.hpp"
#include "ArcballCamera.h"
#include "Vehicle.h"
#include "FPCamera.h"
#include "Marble.h"
#include "Coin.h"
#include "TPCamera.h"

// Forward Declarations of Callback Functions
void mp_engine_keyboard_callback(GLFWwindow *window, int key, int scancode, int action, int mods );
void mp_engine_cursor_callback(GLFWwindow *window, double x, double y );
void mp_engine_mouse_button_callback(GLFWwindow *window, int button, int action, int mods );

enum class CameraType {
    ARCBALL,
    FREECAM,
    FIRSTPERSON,
    THIRDPERSON
};

struct BezierCurve {
    glm::vec3* controlPoints = nullptr;
    GLuint numControlPoints = 0;
    GLuint numCurves = 0;
    float currentPos = 0;
};

extern BezierCurve _bezierCurve;

class FPEngine final : public CSCI441::OpenGLEngine {
public:
    BezierCurve _bezierCurve;
    static constexpr GLfloat WORLD_SIZE = 300.0f;
    FPEngine();
    ~FPEngine() final;
    CameraType currCamera = CameraType::THIRDPERSON;
    void run() final;

    // Event Handlers
    void handleKeyEvent(GLint key, GLint action, GLint mods);
    void handleMouseButtonEvent(GLint button, GLint action, GLint mods); // Updated to include mods
    void handleCursorPositionEvent(glm::vec2 currMousePosition);
    static bool checkCollision(const glm::vec3& pos1, float radius1,
                               const glm::vec3& pos2, float radius2);
    bool isMovementValid(const glm::vec3& newPosition) const;
    void _updateActiveCamera();

    static constexpr GLfloat MOUSE_UNINITIALIZED = -9999.0f;

    bool _isFalling = false;
    float _fallTime = 0.0f;
    bool _isJumping = false;       // Track if the vehicle is currently jumping
    float _jumpProgress = 0.0f;// Progress along the Bezier curve (0 to 1)
    GLuint _curveVAO, _curveVBO;
    GLsizei _numCurvePoints;
    glm::vec3 _jumpStartPosition;  // Starting position of the jump
    glm::vec3 _jumpControlPoints[4]; // Control points for the Bezier curve


    std::vector<Marble> *_enemies; // List of all enemies
    float _enemySpawnInterval = 10.0f; // Time between enemy spawns
    float _enemySpawnTimer = 0.0f;
    GLuint _marbleVBO;
    GLuint _marbleVAO;
    static constexpr int NUM_MARBLES = 100;
    glm::vec3 _marbleLocations[NUM_MARBLES];
    void _initializeMarbleLocations();
    const float MARBLE_RADIUS = 0.5f;
    const float MARBLE_SPEED = 0.1f;
    glm::vec3 _marbleDirections[NUM_MARBLES];
    void _renderMinimap() const;
    void _createCurve(GLuint vao, GLuint vbo, GLsizei &numVAOPoints) const;
    glm::vec3 _evalBezierCurve(const glm::vec3 P0, const glm::vec3 P1, const glm::vec3 P2, const glm::vec3 P3, const GLfloat T) const;

    void _collideMarblesWithWall();
    void _collideMarblesWithMarbles();
    void _moveMarbles();
    void _drawMarbles(glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void _animateBeak(int marbleIndex, glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void _drawBeakTriangle(bool isTop) const;

    std::vector<Coin> _coins;
    void _initializeCoins();
    void _drawCoins(glm::mat4 viewMtx, glm::mat4 projMtx) const;

    bool _isBlinking = false;        // Track if the vehicle is in blinking state
    float _blinkTimer = 0.0f;        // Timer for controlling blink intervals
    int _blinkCount = 0;             // Count the number of blinks
    float _blinkingTime = 0.0f;
    const int MAX_BLINKS = 3;

    //ground data
    const int INNER_RADIUS = 10.0f;
    const int OUTER_RADIUS = 40.0f;



private:
    // Engine Setup and Cleanup
    void mSetupGLFW() final;
    void mSetupOpenGL() final;
    void mSetupShaders() final;
    void mSetupBuffers() final;
    void mSetupScene() final;

    void mCleanupBuffers() final;
    void mCleanupShaders() final;
    void mCleanupTextures() final;

    // Rendering
    void _renderScene(glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void _updateScene();

    // Input Tracking
    static constexpr GLuint NUM_KEYS = GLFW_KEY_LAST;
    GLboolean _keys[NUM_KEYS];
    glm::vec2 _mousePosition;
    GLint _leftMouseButtonState;

    // Camera and Vehicle
    ArcballCamera* _pArcballCam;
    FPCamera* _pFPCam;
    CSCI441::FreeCam* _pFreeCam;
    glm::vec2 _cameraSpeed;
    TPCamera* _pTPCam;

    //FPSCamera* _pFPSCam;

    Vehicle* _pVehicle;

    // Animation State
    float _animationTime;

    // Ground
    GLuint _groundVAO;
    GLsizei _numGroundPoints;

    //arch
    GLuint _archVAO;
    GLsizei _numArchPoints;

    //***************************************************************************
    // Shader Program Information

    GLuint _groundTexture;
    GLuint _loadAndRegisterTexture(const char* FILENAME);
    GLuint _loadAndRegisterCubemap(const std::vector<const char*>& faces);

    void mSetupTextures();
    /// \desc total number of textures in our scene
    static constexpr GLuint NUM_TEXTURES = 2;
    /// \desc used to index through our texture array to give named access
    enum TEXTURE_ID {
        /// \desc metal texture
        RUG = 0,
        MARBLE = 1
    };
    /// \desc texture handles for our textures
    GLuint _texHandles[NUM_TEXTURES];
    /// \desc shader program that performs texturing
    CSCI441::ShaderProgram* _textureShaderProgram;
    /// \desc stores the locations of all of our shader uniforms
    struct TextureShaderUniformLocations {
        /// \desc precomputed MVP matrix location
        GLint mvpMatrix;
        GLint aTextMap;
        GLint colorTint;
        GLint spotLightPosition;
        GLint spotLightDirection;
        GLint spotLightWidth;
        GLint spotLightColor;
    } _textureShaderUniformLocations;
    /// \desc stores the locations of all of our shader attributes
    struct TextureShaderAttributeLocations {
        /// \desc vertex position location
        GLint vPos;
        /// \desc vertex normal location
        /// \note not used in this lab
        GLint vNormal;
        GLint aTextCoords;
    } _textureShaderAttributeLocations;

    // Lamps
    struct LampData {
        glm::mat4 modelMatrixPost;
        glm::mat4 modelMatrixLight;
        glm::vec3 position;
    };
    std::vector<LampData> _lamps;

    // Trees
    struct TreeData {
        glm::mat4 modelMatrixTrunk;
        glm::mat4 modelMatrixLeaves;
    };
    std::vector<TreeData> _trees;
    const std::vector<TreeData>& getTrees() const { return _trees; }

    // Spot Light data
    struct SpotLight{
        glm::vec3 pos;
        glm::vec3 dir;
        glm::vec3 color = glm::vec3(1);
        float width = glm::radians(5.f);
    } _spotLight;

    // Shaders
    CSCI441::ShaderProgram* _lightingShaderProgram = nullptr;
    struct LightingShaderUniformLocations {
        GLint mvpMatrix;
        GLint normalMatrix;
        GLint modelMatrix;
        GLint viewPos;

        // Material properties
        GLint materialAmbient;
        GLint materialDiffuse;
        GLint materialSpecular;
        GLint materialShininess;

        // Directional Light properties
        GLint dirLightDirection;
        GLint dirLightColor;

        // Point Light properties
        GLint numPointLights;
        GLint pointLightPositions;
        GLint pointLightColors;
        GLint pointLightConstants;
        GLint pointLightLinears;
        GLint pointLightQuadratics;

        // Spot Light properties
        GLint spotLightPosition;
        GLint spotLightDirection;
        GLint spotLightWidth;
        GLint spotLightColor;
    }_lightingShaderUniformLocations;

    struct LightingShaderAttributeLocations {
        GLint vPos;
        GLint vNormal;
        GLint vTexCoord;
    } _lightingShaderAttributeLocations;


    // Helper Functions
    void _drawArch(glm::mat4 viewMtx, glm::mat4 projMtx) const;
    void _createArchBuffers();
    void _createGroundBuffers();
    void _generateEnvironment();
    void _computeAndSendMatrixUniforms(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::m