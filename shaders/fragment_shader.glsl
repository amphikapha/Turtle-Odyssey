#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

// Ground blending: three textures (0..2)
uniform sampler2D groundTex[3];
uniform float textureZoneSize; // world-space size of each texture zone (Z axis)
uniform bool useGroundTextures; // when true, blend ground textures based on FragPos.z
uniform sampler2D ourTexture; // default object texture (unit 0)
uniform bool overrideColor; // when true, use objectColor for non-ground objects unconditionally

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

    // Sample the two textures using the provided TexCoords (dynamic indexing)
    vec4 c0 = texture(groundTex[idx], TexCoords);
    vec4 c1 = texture(groundTex[nextIdx], TexCoords);

        // If textures have alpha, respect it; otherwise assume alpha=1
        vec3 col0 = c0.rgb;
        vec3 col1 = c1.rgb;

    // Use hard-cutoff between zones (no blending). Always use the texture of the current zone
    // This gives a crisp border at zone boundaries instead of a smooth blend.
    finalColor = col0;
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
    FragColor = vec4(lit, 1.0);
}
