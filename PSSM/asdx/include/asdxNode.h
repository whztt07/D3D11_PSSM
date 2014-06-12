//-----------------------------------------------------------------------------------
// File : asdxNode.h
// Desc : Node Module.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------------

#ifndef __ASDX_NODE_H__
#define __ASDX_NODE_H__

//-----------------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------------
#include <asdxTypedef.h>
#include <vector>
#include <memory>


namespace asdx {

/////////////////////////////////////////////////////////////////////////////////////
// Node class
/////////////////////////////////////////////////////////////////////////////////////
class Node
{
    //===============================================================================
    // list of friend classes and methods.
    //===============================================================================
    /* NOTHING */

public:
    //===============================================================================
    // public variables.
    //===============================================================================
    /* NOTHING */

    //===============================================================================
    // public methods.
    //===============================================================================
    Node( const char*, Node* );
    virtual ~Node();

    void        Update       ( void* );
    void        Release      ( void );
    bool        HasChild     ( void ) const;
    bool        HasParent    ( void ) const;
    Node*       GetParent    ( void ) const;
    Node*       GetChild     ( u32 )  const;
    const char* GetName      ( void ) const;
    u32         GetChildCount( bool = false ) const;
    void        SetParent    ( Node* );
    bool        AddChild     ( Node* );
    bool        DelChild     ( Node*, bool = false );
    Node*       FindChild    ( const char*, bool = false );
    Node*       FindParent   ( const char* );

protected:
    //===============================================================================
    // protected variables
    //===============================================================================
    const char*             m_pName;
    Node*                   m_pParent;
    std::vector<Node*>      m_Children;

    //===============================================================================
    // protected methods
    //===============================================================================
    virtual void OnUpdate   ( void* );
    virtual void OnRelease  ( void );

private:
    //===============================================================================
    // private variables.
    //===============================================================================
    /* NOTHING */

    //===============================================================================
    // private methods.
    //===============================================================================
    Node            ( const Node& );
    void operator = ( const Node& );
};


} // namespace asdx


#endif//__ASDX_NODE_H__
