#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "../common_defines.glsl"

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragTangent;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D texSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler;

layout(set = 1, binding = 0) uniform UniformBufferObjectPerFrame {
    mat4 camModel;
    mat4 camView;
    mat4 camProj;
    light_t light1; 
} UBOPerFrame;


void main() {

    //parameters
    vec3 lightPos = UBOPerFrame.light1.position[3].xyz;
    vec3 lightDir = normalize( UBOPerFrame.light1.position[2].xyz );     //for directional light, looks down local y axis
    vec3 viewDir  = normalize( UBOPerFrame.camModel[3].xyz - fragPos );

    vec3 N        = normalize( fragNormal );
    vec3 T        = normalize( fragTangent );
    T             = normalize( T - dot(T, N)*N );
    vec3 B        = cross( T, N );
    mat3 TBN      = mat3(T,B,N);
    vec3 mapnorm  = normalize( texture(normalSampler, fragTexCoord).xyz*2.0 - 1.0 );
    vec3 normal   = normalize( TBN * mapnorm );

    vec4 param    = UBOPerFrame.light1.param;
    vec3 fragColor = texture(texSampler, fragTexCoord).xyz;

    lightDir = normalize(lightPos - fragPos);  

    //start light calculations

    //ambient
    vec3 ambcol  = UBOPerFrame.light1.col_ambient.xyz;
    vec3 diffcol = UBOPerFrame.light1.col_diffuse.xyz;
    vec3 speccol = UBOPerFrame.light1.col_specular.xyz;
    
    float pf = dot( N, lightDir )<0 ? 0:1;

    //diffuse
    float diff = max(dot(normal, lightDir), 0.0) * pf;
    vec3 diffuse = diff * diffcol;

    //specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 40) * pf;
    vec3 specular = spec * speccol; 

    //add up to get the result
    vec3 result = (ambcol + diffuse + specular) * fragColor;

    outColor = vec4( result, 1.0 );
}

