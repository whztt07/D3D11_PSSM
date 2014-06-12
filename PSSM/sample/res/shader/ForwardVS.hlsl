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
    float3  Position    : POSITION;         //!< �ʒu���W�ł�(���[�J�����W�n).
    float3  Normal      : NORMAL;           //!< �@���x�N�g���ł�(���[�J�����W�n).
    float3  Tangent     : TANGENT;          //!< �ڃx�N�g���ł�(���[�J�����W�n).
    float2  TexCoord    : TEXCOORD;         //!< �e�N�X�`�����W�ł�.
};

///////////////////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4  Position    : SV_POSITION;          //!<�@�r���[�ˉe��Ԃ̈ʒu���W�ł�.
    float4  WorldPos    : WORLD_POSITION;       //!< ���[���h��Ԃ̈ʒu���W�ł�.
    float3  Normal      : NORMAL;               //!< �@���x�N�g���ł�.
    float2  TexCoord    : TEXCOORD0;            //!< �e�N�X�`�����W�ł�.
    float3  LightDir    : LIGHT_DIRECTION;      //!< ���C�g�̕����x�N�g���ł�.
    float3  CameraPos   : CAMERA_POSITION;      //!< �J�����ʒu�ł�.
    float4  SplitPos    : SPLIT_POSITION;       //!< ���������ł�.
    float4  SdwCoord[4] : SHADOW_COORD;         //!< �V���h�E���W�ł�.
};

///////////////////////////////////////////////////////////////////////////////////////////
// CBMatrix buffer
///////////////////////////////////////////////////////////////////////////////////////////
cbuffer CBMatrix : register( b1 )
{
    float4x4 World                  : packoffset( c0 );     //!< ���[���h�s��ł�.
    float4x4 View                   : packoffset( c4 );     //!< �r���[�s��ł�.
    float4x4 Proj                   : packoffset( c8 );     //!< �ˉe�s��ł�.
    float4   CameraPos              : packoffset( c12 );    //!< �J�����ʒu�ł�.
    float4   LightDir               : packoffset( c13 );    //!< ���C�g�ʒu�ł�.
    float4   SplitPos               : packoffset( c14 );    //!< ���������ł�.
    float4x4 Shadow0                : packoffset( c15 );    //!< �V���h�E�}�b�v�s��0.
    float4x4 Shadow1                : packoffset( c19 );    //!< �V���h�E�}�b�v�s��1.
    float4x4 Shadow2                : packoffset( c23 );    //!< �V���h�E�}�b�v�s��2.
    float4x4 Shadow3                : packoffset( c27 );    //!< �V���h�E�}�b�v�s��3.
};


//-----------------------------------------------------------------------------------------
//! @brief      ���_�V�F�[�_���C���G���g���[�|�C���g.
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

    // �J�X�P�[�h�V���h�E�}�b�v�p.
    output.SplitPos    = SplitPos;
    output.SdwCoord[0] = mul( Shadow0, worldPos );
    output.SdwCoord[1] = mul( Shadow1, worldPos );
    output.SdwCoord[2] = mul( Shadow2, worldPos );
    output.SdwCoord[3] = mul( Shadow3, worldPos );

    return output;
}