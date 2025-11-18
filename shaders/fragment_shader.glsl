#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

// Ground blending: three textures (0..2) plus bridge texture
uniform sampler2D groundTex[3];
uniform sampler2D bridgeTexture; // bridge texture for lake zones
uniform float textureZoneSize; // world-space size of each texture zone (Z axis)
uniform bool useGroundTextures; // when true, blend ground textures based on FragPos.z
uniform sampler2D ourTexture; // default object texture (unit 0)
uniform bool overrideColor; // when true, use objectColor for non-ground objects unconditionally
uniform bool showBridgeInLake; // when true, show bridge texture in lake zones instead of water

// Fog uniforms
uniform float fogNear;
uniform float fogFar;
uniform vec3 fogColor;

void main()
{
    // Ambient - เพิ่มให้สว่างขึ้น
    float ambientStrength = 0.6;
    vec3 ambient = ambientStrength * lightColor;
  	
    // Diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    float specularStrength = 0.3;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * lightColor;  
    
    vec3 finalColor = objectColor;

    if (useGroundTextures) {
        // Determine which zone this fragment lies in along Z (world space)
        // We use -FragPos.z because the game uses decreasing Z to move forward
        float zoneFloat = -FragPos.z / textureZoneSize;
        float zoneFloor = floor(zoneFloat);
        float frac = zoneFloat - zoneFloor; // blend factor between zoneFloor and zoneFloor+1

        int idx = int(mod(zoneFloor, 3.0));
        if (idx < 0) idx += 3;
        int nextIdx = (idx + 1) % 3;

        // If this is a lake zone (idx == 1) and we want to show bridge
        if (idx == 1 && showBridgeInLake) {
            // Sample the water texture as base
            vec4 waterCol = texture(groundTex[idx], TexCoords);
            finalColor = waterCol.rgb;
            
            // Overlay bridge texture in the center (where player walks)
            // Bridge is about ±15 units wide in a 40-unit zone, so normalize X to [-20, 20] range
            // Texture repeats based on world X position divided by zone size
            float bridgeWidth = 16.0; // Width of the bridge stripe
            float normalizedX = mod(FragPos.x, 40.0); // Repeat every 40 units (zone size)
            if (normalizedX > 20.0) normalizedX -= 40.0; // Shift to [-20, 20] range
            
            // Draw bridge in center: from -bridgeWidth/2 to +bridgeWidth/2
            if (abs(normalizedX) < bridgeWidth * 0.5) {
                // Sample bridge texture - use Y texture coord for bridge detail
                vec4 bridgeCol = texture(bridgeTexture, TexCoords);
                // Use bridge texture alpha or just overlay the color
                finalColor = mix(waterCol.rgb, bridgeCol.rgb, 0.8); // 80% bridge color, 20% water shows through
            }
        } else {
            // Sample the normal ground textures
            vec4 c0 = texture(groundTex[idx], TexCoords);
            vec4 c1 = texture(groundTex[nextIdx], TexCoords);

            // If textures have alpha, respect it; otherwise assume alpha=1
            vec3 col0 = c0.rgb;
            vec3 col1 = c1.rgb;

            // Use hard-cutoff between zones (no blending). Always use the texture of the current zone
            // This gives a crisp border at zone boundaries instead of a smooth blend.
            finalColor = col0;
        }
    } else {
        // If requested, force a solid object color (useful for pickups)
        if (overrideColor) {
            finalColor = objectColor;
        } else {
            // Sample object texture when not rendering blended ground
            vec4 texColor = texture(ourTexture, TexCoords);
            if (texColor.a > 0.0)
                finalColor = texColor.rgb;
            else
                finalColor = objectColor;
        }
    }

    // Apply lighting to the chosen color
    vec3 lit = (ambient + diffuse + specular) * finalColor;
    
    // Apply fog effect
    float distance = length(FragPos - viewPos);
    float fogFactor = (fogFar - distance) / (fogFar - fogNear);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    
    vec3 finalLit = mix(fogColor, lit, fogFactor);
    
    FragColor = vec4(finalLit, 1.0);
}
