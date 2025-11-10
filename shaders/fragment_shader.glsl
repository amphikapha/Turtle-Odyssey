#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform sampler2D ourTexture;

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
    
    // Get texture color
    vec4 texColor = texture(ourTexture, TexCoords);
    
    // If texture is loaded (alpha > 0), use it; otherwise use objectColor
    vec3 finalColor;
    if (texColor.a > 0.0) {
        finalColor = (ambient + diffuse + specular) * texColor.rgb;
    } else {
        finalColor = (ambient + diffuse + specular) * objectColor;
    }
    
    FragColor = vec4(finalColor, 1.0);
}
