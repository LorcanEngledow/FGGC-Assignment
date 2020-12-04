//--------------------------------------------------------------------------------------
// File: DX11 Framework.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;

    float4 DiffuseMtrl;
    float4 DiffuseLight;
    float3 LightVecW;

    float gTime;

    float4 AmbientMtrl;
    float4 AmbientLight;

    float4 SpecularMtrl;
    float4 SpecularLight;
    float  SpecularPower;
    float3 EyePosW;
}

//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float3 Norm : NORMAL;
	float3 PosW : POSITION;
};

//------------------------------------------------------------------------------------
// Vertex Shader - Implements Gouraud Shading using Diffuse lighting only
//------------------------------------------------------------------------------------
VS_OUTPUT VS(float4 Pos : POSITION, float3 NormalL : NORMAL)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    output.Pos = mul(Pos, World);

	

	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);

    // Convert from local space to world space 
    // W component of vector is 0 as vectors cannot be translated
	float3 normalW = mul(float4(NormalL, 0.0f), World).xyz;
	normalW = normalize(normalW);    
    
	output.Norm = normalW;
    
    return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( VS_OUTPUT input ) : SV_Target
{
	float3 toEye = normalize(EyePosW - input.Pos.xyz);
    
	// Compute Diffuse lighting 
	float diffuseAmount = max(dot(LightVecW, input.Norm), 0.0f);
	float3 diffuse = diffuseAmount * (DiffuseMtrl * DiffuseLight).rgb;
    
    //Compute Ambient Shading
	float3 ambient = AmbientMtrl * AmbientLight;
    
    //Compute Specular highlights
	float3 r = reflect(-LightVecW, input.Norm);
	float specularAmount = pow(max(dot(r, toEye), 0.0f), SpecularPower);
	float3 specular = specularAmount * (SpecularMtrl * SpecularLight).rgb;
    
	float4 Color;
    Color.rgb = ambient + diffuse + specular;
	Color.a = DiffuseMtrl.a;
    
	return Color;
}
