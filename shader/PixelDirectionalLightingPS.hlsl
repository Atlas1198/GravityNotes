#include "Common.hlsl" //必須インクルード

Texture2D g_Texture : register(t0);
Texture2D g_NormalMap : register(t2);
SamplerState g_SamplerState : register(s0);

float3 CalcNormalMapWorldNormal(PS_IN In)
{
    float3 baseNormal = normalize(In.Normal.xyz);
    float3 normalSample = g_NormalMap.Sample(g_SamplerState, In.TexCoord).xyz * 2.0f - 1.0f;

    float3 dp1 = ddx(In.WorldPosition.xyz);
    float3 dp2 = ddy(In.WorldPosition.xyz);
    float2 duv1 = ddx(In.TexCoord);
    float2 duv2 = ddy(In.TexCoord);

    float det = duv1.x * duv2.y - duv1.y * duv2.x;
    if (abs(det) < 0.00001f)
    {
        return baseNormal;
    }

    float invDet = 1.0f / det;
    float3 tangent = normalize((dp1 * duv2.y - dp2 * duv1.y) * invDet);
    float3 bitangent = normalize((dp2 * duv1.x - dp1 * duv2.x) * invDet);

    return normalize(
        normalSample.x * tangent +
        normalSample.y * bitangent +
        normalSample.z * baseNormal
    );
}

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    // 法線の正規化 (補間によって縮んでいるため)
    float3 worldNormal = CalcNormalMapWorldNormal(In);
    float3 toLight = Light.Position.xyz - In.WorldPosition.xyz;
    float distanceToLight = length(toLight);
    float3 lightDirection = (distanceToLight > 0.001f) ? toLight / distanceToLight : float3(0.0f, 1.0f, 0.0f);
    float lightRange = max(Light.PointLightParam.x, 0.001f);
    float lightIntensity = max(Light.PointLightParam.y, 0.0f);
    float attenuation = saturate(1.0f - distanceToLight / lightRange);
    attenuation = attenuation * attenuation * lightIntensity;

    // ランバート反射
    float light = 0.0f;
    if (Light.Enable)
    {
        light = saturate(dot(lightDirection, worldNormal)) * attenuation;
    }

    // スペキュラー反射（ブリン・フォン）
    float3 L = lightDirection;
    float3 V = normalize(CameraPosition.xyz - In.WorldPosition.xyz);
    float3 H = normalize(L + V);
    float specular = 0.0f;
    if (Light.Enable)
    {
        specular = pow(saturate(dot(worldNormal, H)), 50.0) * attenuation;
    }

    // テクスチャサンプリング
    float4 texColor = g_Texture.Sample(g_SamplerState, In.TexCoord);
	float3 baseColor = texColor.rgb * In.Diffuse.rgb;
	float3 ambient = saturate(Light.Ambient.rgb);
	float3 diffuse = baseColor * light * Light.Diffuse.rgb;

    // 最終出力 (ランバート + スペキュラー)
	outDiffuse.rgb = saturate(baseColor * ambient + diffuse + specular * Light.Diffuse.rgb);
    outDiffuse.a = texColor.a * In.Diffuse.a;
}
