#version 410 core

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;
in vec3 fPosition;

out vec4 fColor;

//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;

//texture
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

vec3 ambient;
float ambientStrength = 2.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;
float shadow;

vec3 ambient_point;
vec3 specular_point;
vec3 diffuse_point;

uniform float fogDensity;

// Point light parameters
uniform vec3 pointLightPosition;
uniform float pointConstant;
uniform float pointLinear;
uniform float pointQuadratic;
uniform vec3 pointLightColor;

uniform bool choose_light;

void computeLightComponents()
{
    vec3 cameraPosEye = vec3(0.0f); // in eye coordinates, the viewer is situated at the origin
    
    // transform normal
    vec3 normalEye = normalize(fNormal);
    
    // compute light direction
    vec3 lightDirN = normalize(lightDir);
    
    // compute view direction 
    vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
        
    // compute ambient light
    ambient = ambientStrength * lightColor;
    
    // compute diffuse light
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
    
    // compute specular light
    vec3 reflection = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
    specular = specularStrength * specCoeff * lightColor;
    
    // For point light
    vec3 pointLightDirN = normalize(pointLightPosition - fPosition.xyz);
	
	
    float dist = length(pointLightPosition - fPosition.xyz);
    float att = 1.0 / (pointConstant + pointLinear * dist + pointQuadratic * (dist * dist));
    ambient_point = att * ambientStrength * pointLightColor;
    diffuse_point = att * max(dot(normalEye, pointLightDirN), 0.0) * pointLightColor;
    specular_point = att * specularStrength * specCoeff * pointLightColor;
}

float computeShadow()
{
    // perform perspective divide
    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    normalizedCoords = normalizedCoords * 0.5 + 0.5;
    
    if (normalizedCoords.z > 1.0f)
        return 0.0f;
    
    // Get closest depth value from light's perspective
    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
    
    // Get depth of current fragment from light's perspective
    float currentDepth = normalizedCoords.z;
    
    // Check whether current frag pos is in shadow
    float bias = 0.005f;
    float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;
    
    return shadow;
}

float computeFog()
{
    float fragmentDistance = length(fPosEye);
    float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));
    return clamp(fogFactor, 0.0f, 1.0f);
}

void main() 
{
    computeLightComponents();
    
    vec3 baseColor = vec3(0.9f, 0.35f, 0.0f); // orange
    
    ambient *= texture(diffuseTexture, fTexCoords).rgb;
    diffuse *= texture(diffuseTexture, fTexCoords).rgb;
    specular *= texture(specularTexture, fTexCoords).rgb;

    ambient_point *= texture(diffuseTexture, fTexCoords).rgb;
    diffuse_point *= texture(diffuseTexture, fTexCoords).rgb;
    specular_point *= texture(specularTexture, fTexCoords).rgb;

    // modulate with shadow
    shadow = computeShadow();
    vec3 color = min((ambient + (1.0f - shadow) * diffuse) + (1.0f - shadow) * specular, 1.0f);
    vec3 color1 = min((ambient_point + diffuse_point) + specular_point, 1.0f);
    vec3 final_color;
    if(choose_light) {
	final_color = color1;
    } else {
	final_color = color;
    }
	
    float fogFactor = computeFog();
    vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
    fColor = mix(fogColor, vec4(final_color , 1.0f), fogFactor);
}
