//--------------------------------------------------------------------------------------
// File: Shadersasd.fx
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
#define NUM_LIGHTS (1)
#define NEAR_PLANE (0.01f)
#define FAR_PLANE (1000.0f)

Texture2D aTextures[2] : register(t0);
SamplerState aSamplers[2] : register(s0);

Texture2D shadowMapTexture : register(t2);
SamplerState shadowMapSampler : register(s2);

TextureCube envTexture : register(t3);
SamplerState envSampler : register(s3);

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
/*C+C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C
  Cbuffer:  cbChangeOnCameraMovement
  Summary:  Constant buffer used for view transformation
C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C-C*/
cbuffer cbChangeOnCameraMovement : register(b0)
{
    matrix View;
    float4 CameraPosition;
};

/*C+C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C
  Cbuffer:  cbChangeOnResize
  Summary:  Constant buffer used for projection transformation
C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C-C*/
cbuffer cbChangeOnResize : register(b1)
{
    matrix Projection;
};


/*C+C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C
  Cbuffer:  cbChangesEveryFrame
  Summary:  Constant buffer used for world transformation
C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C-C*/

cbuffer cbChangesEveryFrame : register(b2)
{
    matrix World;
    float4 OutputColor;
    bool HasNormalMap;
};

struct PointLight
{
    float4 Position;
    float4 Color;
    matrix View;
    matrix Projection;
    float4 AttenuationDistance;
};

cbuffer cbLights : register(b3)
{
    PointLight PointLights[NUM_LIGHTS];
};

//--------------------------------------------------------------------------------------
/*C+C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C
  Struct:   VS_INPUT

  Summary:  Used as the input to the vertex shader 
C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C-C*/

struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD;

};

struct VS_ENVIRONMENT_INPUT
{
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 Normal : NORMAL;
};

/*C+C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C
  Struct:   PS_INPUT

  Summary:  Used as the input to the pixel shader, output of the 
            vertex shader
C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C-C*/
struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
};

struct PS_ENVIRONMENT_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : WORLDPOS;
    float2 Tex : TEXCOORD0;
    float3 RefVec : REFLECTION;
    
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

PS_ENVIRONMENT_INPUT VSEnvironmentMap(VS_ENVIRONMENT_INPUT input)
{
    PS_ENVIRONMENT_INPUT output = (PS_ENVIRONMENT_INPUT) 0;
    output.Pos = mul(input.Position, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    
    output.Tex = input.TexCoord;
    output.WorldPos = mul(input.Position, World).xyz;
    
    float3 incident = normalize(output.WorldPos - CameraPosition.xyz);
    float3 normal = normalize(mul(float4(input.Normal, 0), World).xyz);
    
    output.RefVec = reflect(incident, normal);
    
    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 PSEnvironmentMap(PS_ENVIRONMENT_INPUT input) : SV_Target
{
    float4 output = (float4) 0;
    
    float4 color = aTextures[0].Sample(aSamplers[0], input.Tex);
    float3 ambient = float3(0.1f, 0.1f, 0.1f) * color.rgb;
    float3 environment = envTexture.Sample(envSampler, input.RefVec);
    
    float refAmonunt = 0.25f;
    output.rgb = saturate(lerp(ambient, environment, refAmonunt));
    output.a = color.a;
    
    return output;

}