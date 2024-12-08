#version 330 core

in vec2 TexCoords;            // Interpolated texture coordinates
in vec3 FragPos;              // World-space position of the fragment

out vec4 FragColor;

uniform sampler2D textureMap; // Base texture

// Spotlight parameters
uniform vec3 spotLightPosition;
uniform vec3 spotLightDirection;
uniform float spotLightWidth;
uniform vec3 spotLightColor;

void main() {
    // Fetch texture color
    vec4 texColor = texture(textureMap, TexCoords);

    // Spotlight calculations
    vec3 lightDir = normalize(spotLightPosition - FragPos); // Direction from fragment to light
    float theta = dot(lightDir, normalize(-spotLightDirection)); // Angle between light and spotlight direction

    // Spotlight effect
    vec3 spotlightEffect = vec3(0.0); // Initialize with no contribution
    if (theta > spotLightWidth) {
        // Spotlight is active on this fragment
        float distance = length(spotLightPosition - FragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance)); // Quadratic attenuation

        // Calculate spotlight contribution
        spotlightEffect = spotLightColor * attenuation * (theta - spotLightWidth);
    }

    // Combine texture and spotlight
    vec3 finalColor = texColor.rgb + spotlightEffect;
    FragColor = vec4(finalColor, texColor.a); // Add spotlight to texture
}
