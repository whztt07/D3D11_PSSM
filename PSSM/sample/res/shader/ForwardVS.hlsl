//-----------------------------------------------------------------------------------------
// File : ForwardVS.hlsl
// Desc : Forward Shading [Vertex Shader]
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////////////////
// VSInput structure
///////////////////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3  Position    : POSITION;         //!< 位置座標です(ローカル座標系).
    float3  Normal      : NORMAL;           //!< 法線ベクトルです(ローカル座標系).
    float3  Tangent     : TANGENT;          //!< 接ベクトルです(ローカル座標系).
    float2  TexCoord    : TEXCOORD;         //!< テクスチャ座標です.
};

///////////////////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4  Position    : SV_POSITION;          //!<　ビュー射影空間の位置座標です.
    float4  WorldPos    : WORLD_POSITION;       //!< ワールド空間の位置座標です.
    float3  Normal      : NORMAL;               //!< 法線ベクトルです.
    float2  TexCoord    : TEXCOORD0;            //!< テクスチャ座標です.
    float3  LightDir    : LIGHT_DIRECTION;      //!< ライトの方向ベクトルです.
    float3  CameraPos   : CAMERA_POSITION;      //!< カメラ位置です.
    float4  SplitPos    : SPLIT_POSITION;       //!< 分割距離です.
    float4  SdwCoord[4] : SHADOW_COORD;         //!< シャドウ座標です.
};

///////////////////////////////////////////////////////////////////////////////////////////
// CBMatrix buffer
///////////////////////////////////////////////////////////////////////////////////////////
cbuffer CBMatrix : register( b1 )
{
    float4x4 World                  : packoffset( c0 );     //!< ワールド行列です.
    float4x4 View                   : packoffset( c4 );     //!< ビュー行列です.
    float4x4 Proj                   : packoffset( c8 );     //!< 射影行列です.
    float4   CameraPos              : packoffset( c12 );    //!< カメラ位置です.
    float4   LightDir               : packoffset( c13 );    //!< ライト位置です.
    float4   SplitPos               : packoffset( c14 );    //!< 分割距離です.
    float4x4 Shadow0                : packoffset( c15 );    //!< シャドウマップ行列0.
    float4x4 Shadow1                : packoffset( c19 );    //!< シャドウマップ行列1.
    float4x4 Shadow2                : packoffset( c23 );    //!< シャドウマップ行列2.
    float4x4 Shadow3                : packoffset( c27 );    //!< シャドウマップ行列3.
};


//-----------------------------------------------------------------------------------------
//! @brief      頂点シェーダメインエントリーポイント.
//-----------------------------------------------------------------------------------------
VSOutput VSFunc( VSInput input )
{
    VSOutput output = (VSOutput)0;

    float4 localPos = float4( input.Position, 1.0f );
    float4 worldPos = mul( World, localPos );
    float4 viewPos  = mul( View,  worldPos );
    float4 projPos  = mul( Proj,  viewPos );

    output.Position = projPos;
    output.WorldPos = worldPos;
    output.LightDir = -LightDir.xyz;

    output.Normal   = input.Normal;
    output.TexCoord = input.TexCoord;

    output.CameraPos = CameraPos.xyz;

    // カスケードシャドウマップ用.
    output.SplitPos    = SplitPos;
    output.SdwCoord[0] = mul( Shadow0, worldPos );
    output.SdwCoord[1] = mul( Shadow1, worldPos );
    output.SdwCoord[2] = mul( Shadow2, worldPos );
    output.SdwCoord[3] = mul( Shadow3, worldPos );

    return output;
}