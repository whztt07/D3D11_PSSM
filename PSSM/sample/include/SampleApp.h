//-----------------------------------------------------------------------------------
// File : sampleApp.h
// Desc : Sample Application Module.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------------

#ifndef __SAMPLE_APP_H__
#define __SAMPLE_APP_H__

//------------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------------
#include <asdxTypedef.h>
#include <asdxApp.h>
#include <asdxQuadRenderer.h>
#include <asdxFont.h>
#include <asdxMesh.h>
#include <asdxCameraUpdater.h>
#include <asdxGeometry.h>
#include <asdxOnb.h>


// カスケードの段数です.
static const int MAX_CASCADE = 4;


//////////////////////////////////////////////////////////////////////////////////////
// ShadowState structure
//////////////////////////////////////////////////////////////////////////////////////
struct ShadowState
{
    ID3D11DepthStencilView*     pDSV[ MAX_CASCADE ];            //!< 深度ステンシルビュー.
    ID3D11Texture2D*            pDepthTex[ MAX_CASCADE ];       //!< 深度テクスチャ.
    ID3D11ShaderResourceView*   pDepthSRV[ MAX_CASCADE ];       //!< 深度シェーダリソースビュー.
    ID3D11VertexShader*         pVS;                            //!< シャドウマップ描画用頂点シェーダ.
    ID3D11Buffer*               pCB;                            //!< 定数バッファ.
    ID3D11DepthStencilState*    pDSS;                           //!< 深度ステンシルステート.
    ID3D11SamplerState*         pSmp;                           //!< シャドウマップフェッチ用サンプラーステート.
    D3D11_VIEWPORT              Viewport;                       //!< ビューポート.

    //-------------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //-------------------------------------------------------------------------------
    ShadowState()
    : pVS       ( nullptr )
    , pCB       ( nullptr )
    , pDSS      ( nullptr )
    , pSmp      ( nullptr )
    {
        for( int i=0; i<MAX_CASCADE; ++i )
        {
            pDSV[i]      = nullptr;
            pDepthTex[i] = nullptr;
            pDepthSRV[i] = nullptr;
        }
    }

    //-------------------------------------------------------------------------------
    //! @brief      解放処理です.
    //-------------------------------------------------------------------------------
    void Release()
    {
        for( int i=0; i<MAX_CASCADE; ++i )
        {
            ASDX_RELEASE( pDSV[i] );
            ASDX_RELEASE( pDepthTex[i] );
            ASDX_RELEASE( pDepthSRV[i] );
        }

        ASDX_RELEASE( pVS );
        ASDX_RELEASE( pCB );
        ASDX_RELEASE( pDSS );
        ASDX_RELEASE( pSmp );
    }
};


//////////////////////////////////////////////////////////////////////////////////////
// CBForward structure
//////////////////////////////////////////////////////////////////////////////////////
struct CBForward
{
    asdx::Matrix        World;                      //!< ワールド行列です.
    asdx::Matrix        View;                       //!< ビュー行列です.
    asdx::Matrix        Proj;                       //!< 射影行列です.
    asdx::Vector3       CameraPos;                  //!< カメラ位置です.
    f32                 dummy0;                     //!< ダミー.
    asdx::Vector3       LightDir;                   //!< ライトの方向ベクトルです.
    f32                 dummy1;                     //!< ダミー.
    f32                 SplitPos[ MAX_CASCADE ];    //!< 分割位置です(ビュー空間での距離).
    asdx::Matrix        Shadow  [ MAX_CASCADE ];    //!< シャドウマップ行列です.
};


//////////////////////////////////////////////////////////////////////////////////////
// CBGenShadow structure
//////////////////////////////////////////////////////////////////////////////////////
struct CBGenShadow
{
    asdx::Matrix        World;          //!< ワールド行列です.
    asdx::Matrix        ViewProj;       //!< ビュー射影行列です.
};


//////////////////////////////////////////////////////////////////////////////////////
// SampleApp class
//////////////////////////////////////////////////////////////////////////////////////
class SampleApp : public asdx::Application
{
    //================================================================================
    // list of friend classes and methods.
    //================================================================================
    /* NOTHING */

public:
    //================================================================================
    // public variables.
    //================================================================================

    //================================================================================
    // public methods.
    //================================================================================
    SampleApp();
    virtual ~SampleApp();

protected:
    //================================================================================
    // protected variables.
    //================================================================================

    //================================================================================
    // protected methods.
    //================================================================================
    virtual bool OnInit();
    virtual void OnTerm();
    virtual void OnFrameRender( asdx::FrameEventParam& );

    bool InitQuad();
    void TermQuad();
    void DrawQuad( ID3D11ShaderResourceView** ppSRV );
    bool InitShadowState();
    void TermShadowState();
    void DrawToShadowMap();
    bool InitMesh();
    void TermMesh();
    bool InitForward();
    void TermForward();

    void ComputeShadowMatrixPSSM();

    asdx::Matrix CreateUnitCubeClipMatrix(
        const asdx::Vector3& mini,
        const asdx::Vector3& maxi );

    asdx::Matrix CreateScreenMatrix(
        const f32 x,
        const f32 y,
        const f32 sx,
        const f32 sy );

    asdx::Matrix CreateCropMatrix( asdx::BoundingBox& box );

    void AdjustClipPlanes(
        const asdx::BoundingBox& casterBox,
        const asdx::Vector3& cameraPos,
        const asdx::Vector3& viewDir, 
        f32& nearClip,
        f32& farClip );

    void ComputeSplitPositions(
        s32 splitCount,
        f32 lamda,
        f32 nearClip,
        f32 farClip,
        f32* pPositions );

    asdx::BoundingBox CalculateFrustum(
        f32 nearClip,
        f32 farClip,
        asdx::Matrix& viewProj );

    void OnFrameMove  ( asdx::FrameEventParam& param );
    void OnMouse      ( const asdx::MouseEventParam&  param );
    void OnKey        ( const asdx::KeyEventParam& param );

private:
    //================================================================================
    // private variables.
    //================================================================================
    asdx::QuadRenderer          m_Quad;
    ID3D11VertexShader*         m_pQuadVS;
    ID3D11PixelShader*          m_pQuadPS;
    ID3D11Buffer*               m_pQuadCB;
    ID3D11SamplerState*         m_pQuadSmp;
    asdx::Font                  m_Font;
    asdx::CameraUpdater         m_Camera;

    ShadowState                 m_ShadowState;

    ID3D11VertexShader*         m_pVS;
    ID3D11PixelShader*          m_pPS;
    ID3D11Buffer*               m_pCBMatrixForward;

    asdx::Mesh                  m_Dosei;
    asdx::BoundingBox           m_Box_Dosei;

    asdx::Matrix                m_View;
    asdx::Matrix                m_Proj;

    asdx::Vector3               m_LightDir;
    asdx::Matrix                m_LightView;
    asdx::Matrix                m_LightProj;
    asdx::OrthonormalBasis      m_LightBasis;
    asdx::Matrix                m_ShadowMatrix[ MAX_CASCADE ];
    f32                         m_SplitPos[ MAX_CASCADE ];

    f32                         m_LightRotX;
    f32                         m_LightRotY;
    f32                         m_Lamda;
    f32                         m_CameraFar;
    f32                         m_CameraNear;
    f32                         m_CameraFov;
    bool                        m_ShowTexture;


    //================================================================================
    // private methods.
    //================================================================================
    SampleApp       ( const SampleApp& );
    void operator = ( const SampleApp& );
};

#endif//__SAMPLE_APP_H__
