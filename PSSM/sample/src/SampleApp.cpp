//-----------------------------------------------------------------------------------
// File : sampleApp.h
// Desc : Sample Application Module.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------------
#include <sampleApp.h>
#include <asdxShader.h>
#include <asdxLog.h>
#include <vector>

// テクスチャバイアス.
static const asdx::Matrix SHADOW_BIAS = asdx::Matrix(
    0.5f,  0.0f, 0.0f, 0.0f,
    0.0f, -0.5f, 0.0f, 0.0f,
    0.0f,  0.0f, 1.0f, 0.0f,
    0.5f,  0.5f, 0.0f, 1.0f );

/////////////////////////////////////////////////////////////////////////////////////
// QuadParam structure
/////////////////////////////////////////////////////////////////////////////////////
ASDX_ALIGN(16)
struct QuadParam
{
    asdx::Matrix matrix;
    f32          arrayIndex;
};

/////////////////////////////////////////////////////////////////////////////////////
// SampleApp class
/////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------------
SampleApp::SampleApp()
: Application( "Parallel-Split Shadow Map", 960, 540 )
, m_Quad    ()
, m_pQuadVS ( nullptr )
, m_pQuadPS ( nullptr )
, m_pQuadCB ( nullptr )
, m_pQuadSmp( nullptr )
, m_ShadowState()
, m_pVS     ( nullptr )
, m_pPS     ( nullptr )
, m_pCBMatrixForward( nullptr )
, m_Dosei   ()
, m_LightRotX( asdx::F_PIDIV4 )
, m_LightRotY( asdx::F_PIDIV2 )
, m_Lamda( 0.5f )
, m_ShowTexture( true )
{
    /* DO_NOTHING */
}

//-----------------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------------
SampleApp::~SampleApp()
{
    /* DO_NOTHING */
}

//-----------------------------------------------------------------------------------
//      矩形の初期化処理です.
//-----------------------------------------------------------------------------------
bool SampleApp::InitQuad()
{
    HRESULT hr = S_OK;
    {
        ID3DBlob* pBlob;

        // 頂点シェーダをコンパイル.
        hr = asdx::ShaderHelper::CompileShaderFromFile( 
            L"../res/shader/QuadRenderer.hlsl",
            "VSFunc",
            asdx::ShaderHelper::VS_4_0,
            &pBlob );

        // エラーチェック.
        if ( FAILED( hr ) )
        {
            ELOG( "Error : CompileShaderFromFile() Failed." );
            return false;
        }

        // 頂点シェーダを生成.
        hr = m_pDevice->CreateVertexShader(
            pBlob->GetBufferPointer(),
            pBlob->GetBufferSize(),
            nullptr,
            &m_pQuadVS );

        // エラーチェック.
        if ( FAILED( hr ) )
        {
            ASDX_RELEASE( pBlob );
            ELOG( "Error : ID3D11Device::CreateVertexShader() Failed." );
            return false;
        }

        // 矩形ヘルパーの初期化.
        if ( !m_Quad.Init( m_pDevice, pBlob->GetBufferPointer(), pBlob->GetBufferSize() ) )
        {
            ASDX_RELEASE( pBlob );
            ELOG( "Error : QuadRenderer::Init() Failed." );
            return false;
        }

        // 解放.
        ASDX_RELEASE( pBlob );

        // ピクセルシェーダをコンパイル.
        hr = asdx::ShaderHelper::CompileShaderFromFile(
            L"../res/shader/QuadRenderer.hlsl",
            "PSFunc",
            asdx::ShaderHelper::PS_4_0,
            &pBlob );

        // ピクセルシェーダを生成.
        hr = m_pDevice->CreatePixelShader(
            pBlob->GetBufferPointer(),
            pBlob->GetBufferSize(),
            nullptr,
            &m_pQuadPS );

        // 解放.
        ASDX_RELEASE( pBlob );

        // エラーチェック.
        if ( FAILED( hr ) )
        {
            ELOG( "Error : ID3D11Device::CreatePixelShader() Failed." );
            return false;
        }
    }

    // 定数バッファの生成.
    {
        D3D11_BUFFER_DESC desc;
        ZeroMemory( &desc, sizeof(desc) );
        desc.Usage          = D3D11_USAGE_DEFAULT;
        desc.ByteWidth      = sizeof( QuadParam );
        desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;

        // 定数バッファを生成.
        hr = m_pDevice->CreateBuffer( &desc, nullptr, &m_pQuadCB );
        if ( FAILED( hr ) )
        {
            ELOG( "Error : ID3D11Device::CreateBuffer()" );
            return false;
        }
    }

    // サンプラーステートを生成.
    {
        CD3D11_SAMPLER_DESC desc = CD3D11_SAMPLER_DESC( CD3D11_DEFAULT() );
        
        // サンプラーステートを生成.
        hr = m_pDevice->CreateSamplerState( &desc, &m_pQuadSmp );
        if ( FAILED( hr ) )
        {
            ELOG( "Error : ID3D11Device::CreateSamplerState() Failed." );
            return false;
        }
    }

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------------
//      矩形の終了処理を行います.
//-----------------------------------------------------------------------------------
void SampleApp::TermQuad()
{
    m_Quad.Term();
    ASDX_RELEASE( m_pQuadVS );
    ASDX_RELEASE( m_pQuadPS );
    ASDX_RELEASE( m_pQuadCB );
    ASDX_RELEASE( m_pQuadSmp );
}

//-----------------------------------------------------------------------------------
//      シャドウステートの初期化を行います.
//-----------------------------------------------------------------------------------
bool SampleApp::InitShadowState()
{
    HRESULT hr = S_OK;

    // シャドウマップの解像度.
    u32 mapSize = 1024;

    for( int i=0; i<MAX_CASCADE; ++i )
    {

        // 深度テクスチャの生成.
        {
            D3D11_TEXTURE2D_DESC desc;
            ZeroMemory( &desc, sizeof(desc) );

            desc.Width              = mapSize;
            desc.Height             = mapSize;
            desc.Format             = DXGI_FORMAT_R16_TYPELESS;
            desc.MipLevels          = 1;
            desc.ArraySize          = 1;
            desc.SampleDesc.Count   = 1;
            desc.SampleDesc.Quality = 0;
            desc.Usage              = D3D11_USAGE_DEFAULT;
            desc.BindFlags          = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags     = 0;
            desc.MiscFlags          = 0;

            hr = m_pDevice->CreateTexture2D( &desc, nullptr, &m_ShadowState.pDepthTex[i] );
            if ( FAILED( hr ) )
            {
                ELOG( "Error : CreateTexture2D() Failed." );
                return false;
            }
        }

   
        // 深度ステンシルビューの生成.
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC desc;
            ZeroMemory( &desc, sizeof(desc) );
            desc.Format             = DXGI_FORMAT_D16_UNORM;
            desc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice = 0;

            hr = m_pDevice->CreateDepthStencilView( m_ShadowState.pDepthTex[i], &desc, &m_ShadowState.pDSV[i] );
            if ( FAILED( hr ) )
            {
                ELOG( "Error : CreateDepthStencilView() Failed." );
                return false;
            }
        }

 
        // 深度用シェーダリソースビューの生成.
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            ZeroMemory( &desc, sizeof(desc) );
            desc.Format                    = DXGI_FORMAT_R16_UNORM;
            desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipLevels       = 1;
            desc.Texture2D.MostDetailedMip = 0;

            hr = m_pDevice->CreateShaderResourceView( m_ShadowState.pDepthTex[i], &desc, &m_ShadowState.pDepthSRV[i] );
            if ( FAILED( hr ) )
            {
                ELOG( "Error : CreateShaderResourceView() Failed. ");
                return false;
            }
        }
    }

    // ビューポートの設定.
    m_ShadowState.Viewport.Width    = f32(mapSize);
    m_ShadowState.Viewport.Height   = f32(mapSize);
    m_ShadowState.Viewport.TopLeftX = 0.0f;
    m_ShadowState.Viewport.TopLeftY = 0.0f;
    m_ShadowState.Viewport.MinDepth = 0.0f;
    m_ShadowState.Viewport.MaxDepth = 1.0f;

    // 頂点シェーダの生成.
    {
        ID3DBlob* pBlob;

        if ( asdx::ShaderHelper::CompileShaderFromFile(
            L"../res/shader/ShadowVS.hlsl",
            "VSFunc",
            asdx::ShaderHelper::VS_4_0,
            &pBlob ) )
        {
            ELOG( "Error : CompileShaderFromFile() Vertex Shader Failed." );
            return false;
        }

        hr = m_pDevice->CreateVertexShader( 
            pBlob->GetBufferPointer(),
            pBlob->GetBufferSize(),
            nullptr,
            &m_ShadowState.pVS );

        pBlob->Release();

        if ( FAILED( hr ) )
        {
            ELOG( "Error : CreateVertexShader() Failed." );
            return false;
        }
    }

    // 定数バッファの生成.
    {
        D3D11_BUFFER_DESC desc;
        ZeroMemory( &desc, sizeof( desc ) );
        desc.Usage          = D3D11_USAGE_DEFAULT;
        desc.ByteWidth      = sizeof( CBGenShadow );
        desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;

        hr = m_pDevice->CreateBuffer( &desc, nullptr, &m_ShadowState.pCB );
        if ( FAILED( hr ) )
        {
            ELOG( "Error : ID3D11Device::CreateBuffer() Failed. ");
            return false;
        }
    }

    // サンプラーステートの生成.
    {
        D3D11_SAMPLER_DESC desc;
        ZeroMemory( &desc, sizeof( desc ) );
        desc.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
        desc.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
        desc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
        desc.BorderColor[0] = 1.0f;
        desc.BorderColor[1] = 1.0f;
        desc.BorderColor[2] = 1.0f;
        desc.BorderColor[3] = 1.0f;
        desc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
        desc.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        desc.MaxAnisotropy  = 1;
        desc.MipLODBias     = 0;
        desc.MinLOD         = -FLT_MAX;
        desc.MaxLOD         = +FLT_MAX;

        // サンプラーステートを生成.
        hr = m_pDevice->CreateSamplerState( &desc, &m_ShadowState.pSmp );
        if ( FAILED( hr ) )
        {
            ELOG( "Error : ID3D11Device::CreateSamplerState() Failed." );
            return false;
        }
    }

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------------
//      シャドウステートの終了処理です.
//-----------------------------------------------------------------------------------
void SampleApp::TermShadowState()
{
    m_ShadowState.Release();
}

//-----------------------------------------------------------------------------------
//      フォワードレンダリングの初期化処理です.
//-----------------------------------------------------------------------------------
bool SampleApp::InitForward()
{
    HRESULT hr = S_OK;

    // メッシュの読み込み.
    {
        ID3DBlob* pVSBlob = nullptr;
        hr = asdx::ShaderHelper::CompileShaderFromFile(
            L"../res/shader/ForwardVS.hlsl",
            "VSFunc",
            asdx::ShaderHelper::VS_4_0,
            &pVSBlob );
        if ( FAILED( hr ) )
        {
            ELOG( "Error : CompileShaderFromFile() Failed." );
            return false;
        }

        asdx::ResMesh resMesh;
        if ( !resMesh.LoadFromFile( "../res/scene/scene.msh" ) )
        {
            ASDX_RELEASE( pVSBlob );
            ELOG( "Error : Mesh Load Failed." );
            return false;
        }

        // AABBを求めておく.
        if ( resMesh.GetVertexCount() >= 1 )
        {
            asdx::Vector3 mini = resMesh.GetVertex(0).Position;
            asdx::Vector3 maxi = resMesh.GetVertex(0).Position;

            for( u32 i=1; i<resMesh.GetVertexCount(); ++i )
            {
                mini = asdx::Vector3::Min( mini, resMesh.GetVertex(i).Position );
                maxi = asdx::Vector3::Max( maxi, resMesh.GetVertex(i).Position );
            }

            m_Box_Dosei = asdx::BoundingBox( mini, maxi );
        }

        if ( !m_Dosei.Init( 
            m_pDevice,
            resMesh,
            pVSBlob->GetBufferPointer(),
            pVSBlob->GetBufferSize(),
            "../res/scene/",
            "../res/dummy/" ) )
        {
            ELOG( "Error : Mesh Init Falied." );
            ASDX_RELEASE( pVSBlob );
            return false;
        }

        resMesh.Release();

        hr = m_pDevice->CreateVertexShader( pVSBlob->GetBufferPointer(),
            pVSBlob->GetBufferSize(),
            nullptr,
            &m_pVS );

        ASDX_RELEASE( pVSBlob );
        if ( FAILED( hr ) )
        {
            ELOG( "Error : ID3D11Device::CreateVertexShader() Failed." );
            return false;
        }
    }

    // ピクセルシェーダの生成.
    {
        ID3DBlob* pBlob;

        hr = asdx::ShaderHelper::CompileShaderFromFile(
            L"../res/shader/ForwardPS.hlsl",
            "PSFunc",
            asdx::ShaderHelper::PS_5_0,
            &pBlob );

        if ( FAILED( hr ) )
        {
            ELOG( "Error : Shader Compile Failed." );
            return false;
        }

        hr = m_pDevice->CreatePixelShader(
            pBlob->GetBufferPointer(),
            pBlob->GetBufferSize(),
            nullptr,
            &m_pPS );

        ASDX_RELEASE( pBlob );

        if ( FAILED( hr ) )
        {
            ELOG( "Error : ID3D11Device::CreatePixelShader() Failed." );
            return false;
        }
    }

    // 定数バッファの生成.
    {
        D3D11_BUFFER_DESC desc;
        ZeroMemory( &desc, sizeof( desc ) );
        desc.Usage     = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.ByteWidth = sizeof( CBForward );
        desc.CPUAccessFlags = 0;

        hr = m_pDevice->CreateBuffer( &desc, nullptr, &m_pCBMatrixForward );
        if ( FAILED( hr ) )
        {
            ELOG( "Error : ID3D11Device::CreateBuffer() Failed." );
            return false;
        }

    }

    return true;
}

//-----------------------------------------------------------------------------------
//      フォワードレンダリングの終了処理です.
//-----------------------------------------------------------------------------------
void SampleApp::TermForward()
{
    ASDX_RELEASE( m_pVS );
    ASDX_RELEASE( m_pPS );
    ASDX_RELEASE( m_pCBMatrixForward );
    m_Dosei.Term();
}

//-----------------------------------------------------------------------------------
//      初期化処理です.
//-----------------------------------------------------------------------------------
bool SampleApp::OnInit()
{
    if ( !m_Font.Init( m_pDevice, "../res/font/SetoMini-P.fnt", f32(m_Width), f32(m_Height) ) )
    { return false; }

    if ( !InitQuad() )
    { return false; }

    if ( !InitShadowState() )
    { return false; }

    if ( !InitForward() )
    { return false; }


    // カメラの設定.
    m_Camera.GetCamera().SetPosition( asdx::Vector3( 100.0f, 0.0f, 0.0f ) );
    m_Camera.GetCamera().SetTarget( asdx::Vector3( 0.0f, 0.0f, 0.0f ) );
    m_Camera.GetCamera().SetUpward( asdx::Vector3( 0.0f, 1.0f, 0.0f ) );
    m_Camera.GetCamera().Preset();
    m_Camera.GetCamera().Update();

    // クリップ平面までの距離を設定.
    m_CameraNear = 0.1f;
    m_CameraFar  = 1000.0f;
    m_CameraFov  = asdx::F_PIDIV4;

    // ビュー・射影行列を求めておく.
    m_View = m_Camera.GetView();
    m_Proj = asdx::Matrix::CreatePerspectiveFieldOfView(
        m_CameraFov,
        m_AspectRatio,
        m_CameraNear,
        m_CameraFar );

    // ライトの設定.
    m_LightDir = asdx::Vector3( 0.0f, -1.0f, 0.0f );
    m_LightDir.Normalize();
    m_LightBasis.InitFromW( m_LightDir );

    return true;
}

//-----------------------------------------------------------------------------------
//      終了処理です.
//-----------------------------------------------------------------------------------
void SampleApp::OnTerm()
{
    TermForward();
    TermQuad();
    TermShadowState();
    m_Font.Term();
}

//-----------------------------------------------------------------------------------
//      投影行列をを生成する.
//-----------------------------------------------------------------------------------
asdx::Matrix SampleApp::CreateScreenMatrix
(
    const f32 x,
    const f32 y,
    const f32 sx,
    const f32 sy
)
{
    f32 dx = 2.0f / m_Width;
    f32 dy = 2.0f / m_Height;
    f32 tx = -1.0f + dx * ( x + sx );
    f32 ty = -1.0f + dy * ( y + sy );
    return asdx::Matrix(
        dx * sx,     0.0f,        0.0f,        0.0f,
        0.0f,        dy * sy,     0.0f,        0.0f,
        0.0f,        0.0f,        1.0f,        0.0f,
        tx,          ty,          0.0f,        1.0f ); 
}

//-----------------------------------------------------------------------------------
//      矩形を描画します.
//-----------------------------------------------------------------------------------
void SampleApp::DrawQuad( ID3D11ShaderResourceView** ppSRV )
{
    m_pDeviceContext->VSSetShader( m_pQuadVS,   nullptr, 0 );
    m_pDeviceContext->GSSetShader( nullptr,     nullptr, 0 );
    m_pDeviceContext->PSSetShader( m_pQuadPS,   nullptr, 0 );
    m_pDeviceContext->DSSetShader( nullptr,     nullptr, 0 );
    m_pDeviceContext->HSSetShader( nullptr,     nullptr, 0 );

    QuadParam param;

    m_pDeviceContext->PSSetSamplers( 0, 1, &m_pQuadSmp );

    // index : 0
    {
        m_pDeviceContext->PSSetShaderResources( 0, 1, &ppSRV[0] );

        param.matrix = CreateScreenMatrix( 0.0f, 0.0f, 50.0f, 50.0f );
        param.arrayIndex = 0.0f;
        m_pDeviceContext->UpdateSubresource( m_pQuadCB, 0, nullptr, &param, 0, 0 );
        m_pDeviceContext->VSSetConstantBuffers( 0, 1, &m_pQuadCB );
        m_pDeviceContext->PSSetConstantBuffers( 0, 1, &m_pQuadCB );

        m_Quad.Draw( m_pDeviceContext );
    }

    // index : 1
    {
        m_pDeviceContext->PSSetShaderResources( 0, 1, &ppSRV[1] );

        param.matrix = CreateScreenMatrix( 100.0f, 0.0f, 50.0f, 50.0f );
        param.arrayIndex = 1.0f;
        m_pDeviceContext->UpdateSubresource( m_pQuadCB, 0, nullptr, &param, 0, 0 );
        m_pDeviceContext->VSSetConstantBuffers( 0, 1, &m_pQuadCB );
        m_pDeviceContext->PSSetConstantBuffers( 0, 1, &m_pQuadCB );

        m_Quad.Draw( m_pDeviceContext );
    }

    // index : 2
    {
        m_pDeviceContext->PSSetShaderResources( 0, 1, &ppSRV[2] );

        param.matrix = CreateScreenMatrix( 200.0f, 0.0f, 50.0f, 50.0f );
        param.arrayIndex = 2.0f;
        m_pDeviceContext->UpdateSubresource( m_pQuadCB, 0, nullptr, &param, 0, 0 );
        m_pDeviceContext->VSSetConstantBuffers( 0, 1, &m_pQuadCB );
        m_pDeviceContext->PSSetConstantBuffers( 0, 1, &m_pQuadCB );

        m_Quad.Draw( m_pDeviceContext );
    }

    // index : 3
    {
        m_pDeviceContext->PSSetShaderResources( 0, 1, &ppSRV[3] );

        param.matrix = CreateScreenMatrix( 300.0f, 0.0f, 50.0f, 50.0f );
        param.arrayIndex = 3.0f;
        m_pDeviceContext->UpdateSubresource( m_pQuadCB, 0, nullptr, &param, 0, 0 );
        m_pDeviceContext->VSSetConstantBuffers( 0, 1, &m_pQuadCB );
        m_pDeviceContext->PSSetConstantBuffers( 0, 1, &m_pQuadCB );

        m_Quad.Draw( m_pDeviceContext );
    }

    ID3D11SamplerState*         pNullSmp = nullptr;
    ID3D11ShaderResourceView*   pNullSRV = nullptr;
    m_pDeviceContext->PSSetSamplers( 0, 1, &pNullSmp );
    m_pDeviceContext->PSSetShaderResources( 0, 1, &pNullSRV );

    m_pDeviceContext->VSSetShader( nullptr, nullptr, 0 );
    m_pDeviceContext->GSSetShader( nullptr, nullptr, 0 );
    m_pDeviceContext->PSSetShader( nullptr, nullptr, 0 );
    m_pDeviceContext->DSSetShader( nullptr, nullptr, 0 );
    m_pDeviceContext->HSSetShader( nullptr, nullptr, 0 );
}

//---------------------------------------------------------------------------------------
//      描画時の処理です.
//---------------------------------------------------------------------------------------
void SampleApp::OnFrameRender( asdx::FrameEventParam& param )
{
    // PSSM行列を求める.
    ComputeShadowMatrixPSSM();

    // シャドウマップに描画する.
    DrawToShadowMap();

    // レンダーターゲットビュー・深度ステンシルビューを取得.
    {
        ID3D11RenderTargetView* pRTV = nullptr;
        ID3D11DepthStencilView* pDSV = nullptr;
        pRTV = m_RT.GetRTV();
        pDSV = m_DST.GetDSV();

        // NULLチェック.
        if ( pRTV == nullptr )
        { return; }
        if ( pDSV == nullptr )
        { return; }
    
        // 出力マネージャに設定.
        m_pDeviceContext->OMSetRenderTargets( 1, &pRTV, pDSV );

        D3D11_VIEWPORT viewport;
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width    = f32(m_Width);
        viewport.Height   = f32(m_Height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        m_pDeviceContext->RSSetViewports( 1, &viewport );

        // クリア処理.
        m_pDeviceContext->ClearRenderTargetView( pRTV, m_ClearColor );
        m_pDeviceContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );

        m_pDeviceContext->VSSetShader( m_pVS,   nullptr, 0 );
        m_pDeviceContext->GSSetShader( nullptr, nullptr, 0 );   // ジオメトリシェーダつかったら遅かったので，使わない.
        m_pDeviceContext->PSSetShader( m_pPS,   nullptr, 0 );
        m_pDeviceContext->HSSetShader( nullptr, nullptr, 0 );
        m_pDeviceContext->DSSetShader( nullptr, nullptr, 0 );

        //　定数バッファ更新.
        asdx::Matrix lightRot = asdx::Matrix::CreateRotationX( m_LightRotX )
                              * asdx::Matrix::CreateRotationY( m_LightRotY );
        CBForward cbParam;
        cbParam.World     = asdx::Matrix::CreateScale( 0.25f );
        cbParam.View      = m_View;
        cbParam.Proj      = m_Proj;
        cbParam.CameraPos = m_Camera.GetCamera().GetPosition();
        cbParam.LightDir  = asdx::Vector3::Transform( m_LightDir, lightRot );
        for( u32 i=0; i<4; ++i )
        {
            cbParam.Shadow[i]   = m_ShadowMatrix[i] * SHADOW_BIAS;
            cbParam.SplitPos[i] = m_SplitPos[i];
        }
        m_pDeviceContext->UpdateSubresource( m_pCBMatrixForward, 0, nullptr, &cbParam, 0, 0 );

        // 定数バッファ設定.
        m_pDeviceContext->VSSetConstantBuffers( 1, 1, &m_pCBMatrixForward ); 
        m_pDeviceContext->PSSetShaderResources( 3, 4, m_ShadowState.pDepthSRV );

        // サンプラーステートを設定.
        m_pDeviceContext->PSSetSamplers( 3, 1, &m_ShadowState.pSmp );

        // 描画キック.
        m_Dosei.Draw ( m_pDeviceContext );

        ID3D11ShaderResourceView* pNullSRV[4] = { nullptr, nullptr, nullptr, nullptr };
        m_pDeviceContext->PSSetShaderResources( 0, 4, pNullSRV );

        m_pDeviceContext->VSSetShader( nullptr, nullptr, 0 );
        m_pDeviceContext->PSSetShader( nullptr, nullptr, 0 );

        // FPS描画.
        {
            m_Font.Begin( m_pDeviceContext);
            m_Font.DrawStringArg( 10, 10, "FPS : %.2f", param.FPS );
            m_Font.DrawStringArg( 10, 30, "Light Rotation X : %f", m_LightRotX );
            m_Font.DrawStringArg( 10, 50, "Light Rotation Y : %f", m_LightRotY );
            m_Font.DrawStringArg( 10, 70, "Lamda : %f", m_Lamda );
            m_Font.End( m_pDeviceContext );
        }

        // 左下にシャドウマップをデバッグ描画.
        if ( m_ShowTexture )
        {
            DrawQuad( m_ShadowState.pDepthSRV );
        }
    }

    // コマンドを実行して，画面に表示.
    Present( 0 );
}

//---------------------------------------------------------------------------------------
//      シャドウマップに描画する.
//---------------------------------------------------------------------------------------
void SampleApp::DrawToShadowMap()
{
    ID3D11RenderTargetView* pRTV = nullptr;
    ID3D11DepthStencilView* pDSV = nullptr;

    m_pDeviceContext->VSSetShader( m_ShadowState.pVS, nullptr, 0 );
    m_pDeviceContext->GSSetShader( nullptr, nullptr, 0 );
    m_pDeviceContext->PSSetShader( nullptr, nullptr, 0 );   // 倍速Z-Onlyレンダリング使用.
    m_pDeviceContext->DSSetShader( nullptr, nullptr, 0 );
    m_pDeviceContext->HSSetShader( nullptr, nullptr, 0 );

    m_pDeviceContext->RSSetViewports( 1, &m_ShadowState.Viewport );
    m_pDeviceContext->RSSetState( m_pRS );
    m_pDeviceContext->OMSetDepthStencilState( m_pDSS, 0 );

    CBGenShadow param;
    param.World = asdx::Matrix::CreateScale( 0.25f );

    for( int i=0; i<MAX_CASCADE; ++i )
    {
        pDSV = m_ShadowState.pDSV[i];

        // ターゲットをバインド.
        m_pDeviceContext->OMSetRenderTargets( 0, nullptr, pDSV );

        // クリア処理.
        m_pDeviceContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0 );

        // ビュー射影行列を設定.
        param.ViewProj = m_ShadowMatrix[i];
    
        // 定数バッファを送る.
        m_pDeviceContext->UpdateSubresource( m_ShadowState.pCB, 0, nullptr, &param, 0, 0 );
        m_pDeviceContext->VSSetConstantBuffers( 1, 1, &m_ShadowState.pCB );

        // 描画キック.
        m_Dosei.Draw ( m_pDeviceContext );
    }

    m_pDeviceContext->VSSetShader( nullptr, nullptr, 0 );
    m_pDeviceContext->GSSetShader( nullptr, nullptr, 0 );
    m_pDeviceContext->PSSetShader( nullptr, nullptr, 0 );
    m_pDeviceContext->DSSetShader( nullptr, nullptr, 0 );
    m_pDeviceContext->HSSetShader( nullptr, nullptr, 0 );
}


//---------------------------------------------------------------------------------------
//      フレーム遷移時の処理です.
//---------------------------------------------------------------------------------------
void SampleApp::OnFrameMove( asdx::FrameEventParam& param )
{
    // ビュー行列を更新.
    m_View = m_Camera.GetView();
}

//---------------------------------------------------------------------------------------
//      マウスインベント時の処理です.
//---------------------------------------------------------------------------------------
void SampleApp::OnMouse( const asdx::MouseEventParam& param )
{
    // カメラ処理.
    m_Camera.OnMouse( 
        param.X,
        param.Y,
        param.WheelDelta,
        param.IsLeftButtonDown,
        param.IsRightButtonDown,
        param.IsMiddleButtonDown,
        param.IsSideButton1Down,
        param.IsSideButton2Down
    );
}

//---------------------------------------------------------------------------------------
//      Parallel-Split Shadow Map 行列を求めます.
//---------------------------------------------------------------------------------------
void SampleApp::ComputeShadowMatrixPSSM()
{
    // ライトの回転処理.
    asdx::Matrix  lightRot = asdx::Matrix::CreateRotationX( m_LightRotX ) 
                           * asdx::Matrix::CreateRotationY( m_LightRotY );

    // ライトの方向ベクトルを求める.
    asdx::Vector3 lightDir = asdx::Vector3::Transform( m_LightDir, lightRot );
    lightDir.Normalize();

    // ライトの基底ベクトルを求める.
    m_LightBasis.InitFromW( lightDir );

    // キャスターのAABB.
    asdx::BoundingBox casterBox = m_Box_Dosei;

    // 凸包.
    asdx::Vector3x8 convexHull;
    { casterBox.GetCorners( convexHull ); }

    // キャスターのワールド行列.
    asdx::Matrix world = asdx::Matrix::CreateScale( 0.25f );
    for( u32 i=0; i<convexHull.GetSize(); ++i )
    {
        asdx::Vector3 val = asdx::Vector3::Transform( convexHull[i], world );
        convexHull.SetAt(i, val);
    }

    //---------------------------------------
    // ライトのビュー行列と射影行列を求める.
    //---------------------------------------
    {
        // ライトのビュー行列を生成.
        asdx::Matrix lightView = asdx::Matrix::CreateLookTo(
            asdx::Vector3( 0.0f, 0.0f, 0.0f ),
            m_LightBasis.w,
            m_LightBasis.v );

        // ライトビュー空間でのAABBを求める.
        asdx::Vector3 point = asdx::Vector3::TransformCoord( convexHull[0], lightView );
        asdx::Vector3 mini = point;
        asdx::Vector3 maxi = point;
        {
            for( u32 i=1; i<convexHull.GetSize(); ++i )
            {
                point = asdx::Vector3::TransformCoord( convexHull[i], lightView );
                mini  = asdx::Vector3::Min( mini, point );
                maxi  = asdx::Vector3::Max( maxi, point );
            }
        }

        // ライトビュー空間での中心を求める.
        asdx::Vector3 center = ( mini + maxi ) * 0.5f;

        // ニアクリップ平面とファークリップ平面の距離を求める.
        f32 nearClip  = 1.0f;
        f32 farClip   = fabs( maxi.z - mini.z ) * 1.001f + nearClip;

        // 後退量を求める.
        f32 slideBack = fabs( center.z - mini.z ) + nearClip;

        // 正しいライト位置を求める.
        asdx::Vector3 lightPos = center - ( m_LightBasis.w * slideBack );

        // ライトのビュー行列を算出し直す.
        m_LightView = asdx::Matrix::CreateLookTo(
            lightPos,
            m_LightBasis.w,
            m_LightBasis.v);

        // 求め直したライトのビュー行列を使ってAABBを求める.
        point = asdx::Vector3::TransformCoord( convexHull[0], m_LightView );
        mini  = point;
        maxi  = point;
        {
            for( u32 i=1; i<convexHull.GetSize(); ++i )
            {
                point = asdx::Vector3::TransformCoord( convexHull[i], m_LightView );
                mini  = asdx::Vector3::Min( mini, point );
                maxi  = asdx::Vector3::Max( maxi, point );
            }
        }

        // サイズを求める.
        f32 size = ( maxi - mini ).Length();

        // ライトの射影行列.
        m_LightProj = asdx::Matrix::CreateOrthographic(
            size,
            size,
            nearClip,
            farClip );

        // ライトのビュー射影行列を求める.
        asdx::Matrix lightViewProj = m_LightView * m_LightProj;

        //----------------------------------
        //　単位キューブクリッピング.
        //----------------------------------
        {
            point = asdx::Vector3::TransformCoord( convexHull[0], lightViewProj );
            mini  = point;
            maxi  = point;
            {
                for( u32 i=1; i<convexHull.GetSize(); ++i )
                {
                    point = asdx::Vector3::TransformCoord( convexHull[i], lightViewProj );
                    mini  = asdx::Vector3::Min( mini, point );
                    maxi  = asdx::Vector3::Max( maxi, point );
                }
            }

            // クリップ行列を求める.
            asdx::Matrix clip = CreateUnitCubeClipMatrix( mini, maxi );

            // シャドウマップめいっぱいに映るようにフィッティング.
            m_LightProj = m_LightProj * clip;
        }
    }

    f32 nearClip = m_CameraNear;
    f32 farClip  = m_CameraFar;

    // カメラ位置を算出.
    asdx::Vector3 pos = m_Camera.GetCamera().GetPosition();

    // カメラの方向ベクトルを算出.
    asdx::Vector3 dir = m_Camera.GetCamera().GetTarget() - pos;
    dir.Normalize();

    // クリップ平面の距離を調整.
    AdjustClipPlanes( casterBox, pos, dir, nearClip, farClip );

    // 平行分割処理.
    f32 splitPositions[ MAX_CASCADE + 1 ];
    ComputeSplitPositions( 4, m_Lamda, nearClip, farClip, splitPositions );

    // カスケード処理.
    for( u32 i=0; i<MAX_CASCADE; ++i )
    {
        // ライトのビュー射影行列.
        m_ShadowMatrix[i] = m_LightView * m_LightProj;

        // 分割した視錘台の8角をもとめて，ライトのビュー射影空間でAABBを求める.
        asdx::BoundingBox box = CalculateFrustum(
            splitPositions[ i + 0 ],
            splitPositions[ i + 1 ],
            m_ShadowMatrix[ i ] );

        // クロップ行列を求める.
        asdx::Matrix crop = CreateCropMatrix( box );

        // シャドウマップ行列と分割位置を設定.
        m_ShadowMatrix[i] = m_ShadowMatrix[i] * crop;
        m_SplitPos[i]     = splitPositions[ i + 1 ];
    }
}

//---------------------------------------------------------------------------------------
//      単位キューブクリッピング行列を作成します.
//---------------------------------------------------------------------------------------
asdx::Matrix SampleApp::CreateUnitCubeClipMatrix
(
    const asdx::Vector3& mini,
    const asdx::Vector3& maxi
)
{
    // 単位キューブクリップ行列を求める.
    asdx::Matrix clip;
    clip._11 = 2.0f / ( maxi.x - mini.x );
    clip._12 = 0.0f;
    clip._13 = 0.0f;
    clip._14 = 0.0f;

    clip._21 = 0.0f;
    clip._22 = 2.0f / ( maxi.y - mini.y );
    clip._23 = 0.0f;
    clip._24 = 0.0f;

    clip._31 = 0.0f;
    clip._32 = 0.0f;
    clip._33 = 1.0f / ( maxi.z - mini.z );
    clip._34 = 0.0f;

    clip._41 = -( maxi.x + mini.x ) / ( maxi.x - mini.x );
    clip._42 = -( maxi.y + mini.y ) / ( maxi.y - mini.y );
    clip._43 = - mini.z / ( maxi.z - mini.z );
    clip._44 = 1.0f;

    return clip;
}

//---------------------------------------------------------------------------------------
//      クロップ行列を作成します.
//---------------------------------------------------------------------------------------
asdx::Matrix SampleApp::CreateCropMatrix( asdx::BoundingBox& box )
{
    /* ほぼ単位キューブクリッピングと同じ処理 */
    f32 scaleX  = 1.0f;
    f32 scaleY  = 1.0f;
    f32 scaleZ  = 1.0f;
    f32 offsetX = 0.0f;
    f32 offsetY = 0.0f;
    f32 offsetZ = 0.0f;

    asdx::Vector3 mini = box.mini;
    asdx::Vector3 maxi = box.maxi;

    scaleX = 2.0f / ( maxi.x - mini.x );
    scaleY = 2.0f / ( maxi.y - mini.y );

    offsetX = -0.5f * ( maxi.x + mini.x ) * scaleX;
    offsetY = -0.5f * ( maxi.y + mini.y ) * scaleY;

    // 1.0を下回るとシャドウマップに移る部分が小さくなってしまうので，
    // 制限をかける.
    scaleX = asdx::Max( 1.0f, scaleX );
    scaleY = asdx::Max( 1.0f, scaleY );

#if 0
    // GPU Gems 3の実装では下記処理を行っている.
    // maxi.zの値が0近づくことがあり, シャドウマップが真っ黒になるなどおかしくなることが発生するので,
    // この実装はZ成分については何も処理を行わない.
    mini.z = 0.0f;

    scaleZ  = 1.0f / ( maxi.z - mini.z );
    offsetZ = -mini.z * scaleZ;
#endif

    return asdx::Matrix(
        scaleX,  0.0f,    0.0f,    0.0f,
        0.0f,    scaleY,  0.0f,    0.0f,
        0.0f,    0.0f,    scaleZ,  0.0f,
        offsetX, offsetY, offsetZ, 1.0f );
}

//---------------------------------------------------------------------------------------
//      クリップ平面の距離を調整します.
//---------------------------------------------------------------------------------------
void SampleApp::AdjustClipPlanes
(
    const asdx::BoundingBox&    casterBox,
    const asdx::Vector3&        cameraPos,
    const asdx::Vector3&        viewDir,
    f32& nearClip,
    f32& farClip
)
{
    f32 maxZ = nearClip;
    f32 minZ = farClip;

    asdx::Vector3x8 corners;
    casterBox.GetCorners( corners );

    for( u32 i=0; i<8; ++i )
    {
        asdx::Vector3 pos = corners[i] - cameraPos;

        f32 fZ = asdx::Vector3::Dot( pos, viewDir );

        maxZ = asdx::Max( fZ, maxZ );
        minZ = asdx::Min( fZ, minZ );
    }

    nearClip = asdx::Max( minZ, nearClip );
    farClip  = asdx::Max( maxZ, nearClip + 1.0f );
}

//---------------------------------------------------------------------------------------
//      平行分割位置を求めます.
//---------------------------------------------------------------------------------------
void SampleApp::ComputeSplitPositions
(
    s32 splitCount,
    f32 lamda,
    f32 nearClip,
    f32 farClip,
    f32* positions
)
{
    // 分割数が１の場合は，普通のシャドウマップと同じ.
    if ( splitCount == 1 )
    {
        positions[0] = nearClip;
        positions[1] = farClip;
        return;
    }

    f32 inv_m = 1.0f / f32( splitCount );    // splitCountがゼロでないことは保証済み.

    // ゼロ除算対策.
    assert( nearClip != 0.0f );

    // (f/n)を計算.
    f32 f_div_n = farClip / nearClip;

    // (f-n)を計算.
    f32 f_sub_n = farClip - nearClip;

    // 実用分割スキームを適用.
    // ※ GPU Gems 3, Chapter 10. Parallel-Split Shadow Maps on Programmable GPUs.
    //    http://http.developer.nvidia.com/GPUGems3/gpugems3_ch10.html を参照.
    for( s32 i=1; i<splitCount + 1; ++i )
    {
        // 対数分割スキームで計算.
        f32 Ci_log = nearClip * powf( f_div_n, inv_m * i );

        // 一様分割スキームで計算.
        f32 Ci_uni = nearClip + f_sub_n * i * inv_m;

        // 上記の２つの演算結果を線形補間する.
        positions[i] = lamda * Ci_log + Ci_uni * ( 1.0f - lamda );
    }

    // 最初は, ニア平面までの距離を設定.
    positions[ 0 ] = nearClip;

    // 最後は, ファー平面までの距離を設定.
    positions[ splitCount ] = farClip;
}

//---------------------------------------------------------------------------------------
//      視錘台の8角を求めて，ビュー射影行列をかけてAABBを求める.
//---------------------------------------------------------------------------------------
asdx::BoundingBox SampleApp::CalculateFrustum
(
    f32 nearClip,
    f32 farClip,
    asdx::Matrix& viewProj
)
{
    asdx::Camera camera = m_Camera.GetCamera();
    asdx::Vector3 vZ = ( camera.GetTarget() - camera.GetPosition() ).Normalize();
    asdx::Vector3 vX = ( asdx::Vector3::Cross( camera.GetUpward(), vZ ) ).Normalize();
    asdx::Vector3 vY = ( asdx::Vector3::Cross( vZ, vX ) ).Normalize();

    f32 aspect = m_AspectRatio;
    f32 fov    = m_CameraFov;

    f32 nearPlaneHalfHeight = tanf( fov * 0.5f ) * nearClip;
    f32 nearPlaneHalfWidth  = nearPlaneHalfHeight * aspect;

    f32 farPlaneHalfHeight = tanf( fov * 0.5f ) * farClip;
    f32 farPlaneHalfWidth  = farPlaneHalfHeight * aspect;

    asdx::Vector3 nearPlaneCenter = camera.GetPosition() + vZ * nearClip;
    asdx::Vector3 farPlaneCenter  = camera.GetPosition() + vZ * farClip;;

    asdx::Vector3 corners[8];

    corners[0] = asdx::Vector3( nearPlaneCenter - vX * nearPlaneHalfWidth - vY * nearPlaneHalfHeight );
    corners[1] = asdx::Vector3( nearPlaneCenter - vX * nearPlaneHalfWidth + vY * nearPlaneHalfHeight );
    corners[2] = asdx::Vector3( nearPlaneCenter + vX * nearPlaneHalfWidth + vY * nearPlaneHalfHeight );
    corners[3] = asdx::Vector3( nearPlaneCenter + vX * nearPlaneHalfWidth - vY * nearPlaneHalfHeight );

    corners[4] = asdx::Vector3( farPlaneCenter - vX * farPlaneHalfWidth - vY * farPlaneHalfHeight );
    corners[5] = asdx::Vector3( farPlaneCenter - vX * farPlaneHalfWidth + vY * farPlaneHalfHeight );
    corners[6] = asdx::Vector3( farPlaneCenter + vX * farPlaneHalfWidth + vY * farPlaneHalfHeight );
    corners[7] = asdx::Vector3( farPlaneCenter + vX * farPlaneHalfWidth - vY * farPlaneHalfHeight );

    asdx::Vector3 point = asdx::Vector3::TransformCoord( corners[0], viewProj );
    asdx::Vector3 mini = point;
    asdx::Vector3 maxi = point;
    for( int i=1; i<8; ++i )
    {
        point = asdx::Vector3::TransformCoord( corners[i], viewProj );
        mini  = asdx::Vector3::Min( point, mini );
        maxi  = asdx::Vector3::Max( point, maxi );
    }

    return asdx::BoundingBox( mini, maxi );
}

//---------------------------------------------------------------------------------------
//      キー入力時の処理.
//---------------------------------------------------------------------------------------
void SampleApp::OnKey( const asdx::KeyEventParam& param )
{
    f32 speed = 0.05f;
    if ( param.IsKeyDown )
    {
        switch( param.KeyCode )
        {
        case 'W':
            { m_LightRotX += speed; }
            break;

        case 'X':
            { m_LightRotX -= speed; }
            break;

        case 'A':
            { m_LightRotY -= speed; }
            break;

        case 'D':
            { m_LightRotY += speed; }
            break;

        case 'R':
            {
                m_LightRotX = asdx::F_PIDIV4;
                m_LightRotY = asdx::F_PIDIV2;
                m_Lamda = 0.5f;
            }
            break;

        case 'I':
            {
                m_Lamda += 0.01f;
                if ( m_Lamda > 1.0f )
                { m_Lamda = 1.0f; }
            }
            break;

        case 'U':
            {
                m_Lamda -= 0.01f;
                if ( m_Lamda < 0.0f )
                { m_Lamda = 0.0f; }
            }
            break;

        case 'H':
            { m_ShowTexture = (!m_ShowTexture); }
            break;
        }
    }
}