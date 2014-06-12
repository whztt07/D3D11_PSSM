//-----------------------------------------------------------------------------------
// File : main.cpp
// Desc : Application Main Entry Point.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------------

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#endif//defined(DEBUG) || defined(_DEBUG)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

//-----------------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------------
#include <crtdbg.h>
#include <sampleApp.h>


//-----------------------------------------------------------------------------------
//      アプリケーションメインエントリーポイントです.
//-----------------------------------------------------------------------------------
int main( int argc, char** argv )
{
#if defined(DEBUG) || defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif//defined(DEBUG) || defined(_DEBUG)
    {
        SampleApp app;

        // アプリケーション実行.
        app.Run();
    }

    return 0;
}