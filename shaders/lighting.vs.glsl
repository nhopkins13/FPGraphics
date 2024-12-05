#version 410 core

layout(location = 0) in vec3 vPos;        // Vertex position
layout(location = 1) in vec3 vNormal;     // Vertex normal

// Uniforms
uniform mat4 mvpMatrix;
uniform mat3 normalMatrix;
uniform mat4 modelMatrix;
uniform vec3 viewPos; // Camera position

// Material properties
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};
uniform Material material;

// Directional Light properties
struct DirectionalLight {
    vec3 direction;
    vec3 color;
};
uniform DirectionalLight dirLight;

// Point Light properties
#define MAX_POINT_LIGHTS 10
uniform int numPointLights;
uniform vec3 pointLightPositions[MAX_POINT_LIGHTS];
uniform vec3 pointLightColors[MAX_POINT_LIGHTS];
uniform float pointLightConstants[MAX_POINT_LIGHTS];
uniform float pointLightLinears[MAX_POINT_LIGHTS];
uniform float pointLightQuadratics[MAX_POINT_LIGHTS];

// Spot Light properties
uniform vec3 spotLightPosition;
uniform vec3 spotLightDirection;
uniform vec3 spotLightColor;
uniform float spotLightWidth;

// Outputs to Fragment Shader
out vec3 vertexColor;

void main() {
    // Transformations
    gl_Position = mvpMatrix * vec4(vPos, 1.0);
    vec3 normal = normalize(normalMatrix * vNormal);
    vec3 worldPos = vec3(modelMatrix * vec4(vPos, 1.0));
    vec3 viewDir = normalize(viewPos - worldPos);

    // Initialize color
    vertexColor = vec3(0.0);

    // Directional Light
    {
        vec3 lightDir = normalize(-dirLight.direction);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

        vec3 ambient = material.ambient * dirLight.color;
        vec3 diffuse = material.diffuse * diff * dirLight.color;
        vec3 specular = material.specular * spec * dirLight.color;

        vertexColor += ambient + diffuse + specular;
    }

    // Point Lights
    for(int i = 0; i < numPointLights; i++) {
        vec3 lightPos = pointLightPositions[i];
        vec3 lightColor = pointLightColors[i];

        vec3 lightDir = normalize(lightPos - worldPos);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

        float distance = length(lightPos - worldPos);
        float attenuation = 1.0 / (pointLightConstants[i] + pointLightLinears[i] * distance + pointLightQuadratics[i] * (distance * distance));

        vec3 ambient = material.ambient * lightColor;
        vec3 diffuse = material.diffuse * diff * lightColor;
        vec3 specular = material.specular * spec * lightColor;

        ambient *= attenuation;
        diffuse *= attenuation;
        specular *= attenuation;

        vertexColor += ambient + diffuse + specular;
    }

    // Spot Light
    {
        float linear = 0.09f;
        float quadratic = 0.032f;

        vec3 lightDir = normalize(spotLightPosition - worldPos);
        float diff = max(dot(normal, lightDir), 0.0);

        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 5);

        if( dot(lightDir, normalize(-spotLightDirection)) > spotLightWidth ){
            float dist = length(spotLightPosition - worldPos);
            float attenuation = 1.0 / (1.0 + (linear * dist) + (quadratic * (dist * dist)));

            vec3 ambient = material.ambient * spotLightColor * attenuation;
            vec3 diffuse = material.diffuse * diff * spotLightColor * attenuation;
            vec3 specular = material.specular * spec * spotLightColor * attenuation;
            vertexColor += ambient + diffuse + specular;
        }
    }
    //marbles
    gl_PointSize = 60.0; // Set point size
}
