#version 400 core

layout (location = 0) out vec4 FragColor;

in vec2 UV;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDiffuse;

struct LightSourceParameters
{
    vec3 color;
    vec3 direction;
};

uniform float                       ambient_intensity;
uniform LightSourceParameters       Light;

const int CascadeMaxCount = 5;
const vec3 CamPos = vec3(0,0,0);
const vec3 debug_colors[5] = vec3[] (vec3(1,0,0), vec3(0,1,0), vec3(0,0,1), vec3(1,1,0), vec3(0,1,1));

uniform sampler2D   shadowMaps[CascadeMaxCount];
uniform float       cascade_planes[CascadeMaxCount];
uniform mat4        lightMtx[CascadeMaxCount];
uniform mat4        ViewToWorld;

uniform int   CascadeCount;
uniform float blend_dist;
uniform float bias;
uniform int   pcf_samples;
uniform bool  draw_cascade_levels;

float ComputePCF(int layer, vec3 worldPos)
{
    //compute the coordinates of the pixel in the shadow map that it belongs to
    vec4 fragPosLightSpace = lightMtx[layer] * vec4(worldPos.xyz, 1.0f);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    //get the depth since it is alreadye in [0,1]
    float depth = projCoords.z;

    //get the coordinates in [0,1]
    projCoords = projCoords * 0.5 + 0.5;

    //check if the pixel is outside the final frustrum
    if (projCoords.z > 1.0) 
        return 1.0;

    float center = (depth + bias) < texture(shadowMaps[layer], projCoords.xy).r ? 1.0 : 0.0f;
    //check if 0 samples
    if(pcf_samples == 0)
        return center;

    // compute pcf for the samples given
    float shadow = 0.0;
    int samples_count = 0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMaps[layer], 0));
    for(int x = -pcf_samples; x < pcf_samples; ++x)
    {
        for(int y = -pcf_samples; y < pcf_samples; ++y)
        {
            float pcfDepth = texture(shadowMaps[layer], projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += (depth + bias) < pcfDepth ? 1.0 : 0.0;  
            samples_count++;
        }    
    }
    //total shadow value
    shadow /= samples_count;
    return shadow;
}

float ComputeShadow(vec3 frag_pos)
{
    // select cascade layer
       vec4 worldfragPos = ViewToWorld * vec4(frag_pos, 1.0);
       float depthValue = abs(frag_pos.z);

       //iterate through the cascade levels
       int layer = -1;
       bool blend_shadow = false;
       for (int i = 0; i < CascadeCount; ++i)
       {
           //check in which layer is the point contained 
           float dist_to_plane = abs(cascade_planes[i] - depthValue);
           if(dist_to_plane < blend_dist)
                blend_shadow = true;
           if (depthValue < cascade_planes[i] || blend_shadow)
           {
               layer = i;
               break;
           }
       }
       //check if point outside the frustrum
       if (layer == -1)
           return 1.0f;

       // get the value of the shadow
       float shadow = ComputePCF(layer, worldfragPos.xyz);

       //check if we need to blend so take the shadow value of the other shadow map
       if(blend_shadow && (layer + 1) < CascadeCount)
       {
            float shadow_to_blend = ComputePCF(layer + 1, worldfragPos.xyz);
            float t = (depthValue - (cascade_planes[layer] - blend_dist)) / (blend_dist + blend_dist);
            shadow = (1 - t) * shadow + t * shadow_to_blend;
       }
       return shadow;
}


vec3 ComputeDebugShadowColor(vec3 frag_pos)
{
       // select cascade layer
       vec4 worldfragPos = ViewToWorld * vec4(frag_pos, 1.0);
       float depthValue = abs(frag_pos.z);

       //iterate through the cascade levels
       int layer = -1;
       bool blend_shadow = false;
       for (int i = 0; i < CascadeCount; ++i)
       {
           //check in which layer is the point contained 
           float dist_to_plane = abs(cascade_planes[i] - depthValue);
           if(dist_to_plane < blend_dist)
                blend_shadow = true;
           if (depthValue < cascade_planes[i] || blend_shadow)
           {
               layer = i;
               break;
           }
       }
       //check if point outside the frustrum
       if (layer == -1)
           return vec3(1,1,1);

       // get the value of the color
       vec3 color = debug_colors[layer];

       //check if we need to blend so take the color value of the other shadow map
       if(blend_shadow && (layer + 1) < CascadeCount)
       {
            vec3 color_to_blend = debug_colors[layer + 1];
            float t = (depthValue - (cascade_planes[layer] - blend_dist)) / (blend_dist + blend_dist);
            color = (1 - t) * color + t * color_to_blend;
       }
       return color;
}

void main()
{          
    // retrieve data from gbuffer
    vec3 FragPos = texture(gPosition, UV).rgb;
    vec3 Normal = texture(gNormal, UV).rgb;   
    vec3 Diffuse = texture(gDiffuse, UV).rgb; 
    float Shininess = texture(gNormal, UV).a; 
    float Specular = texture(gDiffuse, UV).a; 

    ////----------COMPUTE LIGHTING VECTORS---------------//
    vec3 normal     = normalize(Normal);          
    vec3 view       = normalize(CamPos - FragPos);
    vec3 light		= normalize(-Light.direction);
    vec3 refl		= reflect(-light, normal);    
    
    //---------COMPUTE LIGTH COMPONENTS----------------//
    vec3 diffuse	= Light.color * (max(dot(normal, light), 0.0f) * Diffuse);      
    vec3 specular	= Light.color * (pow(max(dot(view, refl), 0.0f), 64.0f) * 0.5f); 
	
    //add the resultant color
    vec3 result = (diffuse + specular);

    //check if we are in debug draw of the cascade levels
    if(draw_cascade_levels)
        result *= ComputeDebugShadowColor(FragPos);
    else
        result *= (1.0f - ComputeShadow(FragPos));

    //set color
    FragColor = vec4(result, 1.0);
}