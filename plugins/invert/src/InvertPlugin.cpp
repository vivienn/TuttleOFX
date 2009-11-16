/**
 * @file InvertPlugin.cpp
 * @brief
 * @author
 * @date    01/10/09 12:01
 *
 */

#include "InvertPlugin.hpp"
#include "InvertProcess.hpp"

#include <ofxsImageEffect.h>
#include <ofxsMultiThread.h>
#include <boost/gil/gil_all.hpp>

namespace tuttle {

using namespace boost::gil;
const static std::string kInvertHelpString = "<b>Image inverter</b> is used to invert components of an image.  <br />";

InvertPlugin::InvertPlugin( OfxImageEffectHandle handle ) :
ImageEffect( handle )
{
    _srcClip = fetchClip( kOfxImageEffectSimpleSourceClipName );
    _dstClip = fetchClip( kOfxImageEffectOutputClipName );
}

OFX::Clip * InvertPlugin::getSrcClip( ) const
{
    return _srcClip;
}

OFX::Clip * InvertPlugin::getDstClip( ) const
{
    return _dstClip;
}

/**
 * @brief The overridden render function
 * @param[in]   args     Rendering parameters
 */
void InvertPlugin::render( const OFX::RenderArguments &args )
{
    // instantiate the render code based on the pixel depth of the dst clip
    OFX::BitDepthEnum dstBitDepth = _dstClip->getPixelDepth( );
    OFX::PixelComponentEnum dstComponents = _dstClip->getPixelComponents( );

    // do the rendering
    if( dstComponents == OFX::ePixelComponentRGBA )
    {
        switch( dstBitDepth )
        {
            case OFX::eBitDepthUByte :
            {
                InvertProcess<rgba8_view_t> fred( *this );
                fred.setupAndProcess( args );
                break;
            }
            case OFX::eBitDepthUShort :
            {
                InvertProcess<rgba16_view_t> fred( *this );
                fred.setupAndProcess( args );
                break;
            }
            case OFX::eBitDepthFloat :
            {
                InvertProcess<rgba32f_view_t> fred( *this );
                fred.setupAndProcess( args );
                break;
            }
            case OFX::eBitDepthNone :
                COUT_FATALERROR( "BitDepthNone not recognize." );
                return;
            case OFX::eBitDepthCustom :
                COUT_FATALERROR( "BitDepthCustom not recognize." );
                return;
        }
    }
    else if( dstComponents == OFX::ePixelComponentAlpha )
    {
        switch( dstBitDepth )
        {
            case OFX::eBitDepthUByte :
            {
                InvertProcess<gray8_view_t> fred( *this );
                fred.setupAndProcess( args );
                break;
            }
            case OFX::eBitDepthUShort :
            {
                InvertProcess<gray16_view_t> fred( *this );
                fred.setupAndProcess( args );
                break;
            }
            case OFX::eBitDepthFloat :
            {
                InvertProcess<gray32f_view_t> fred( *this );
                fred.setupAndProcess( args );
                break;
            }
            case OFX::eBitDepthNone :
                COUT_FATALERROR( "BitDepthNone not recognize." );
                return;
            case OFX::eBitDepthCustom :
                COUT_FATALERROR( "BitDepthCustom not recognize." );
                return;
        }
    }
}

void InvertPlugin::changedParam( const OFX::InstanceChangedArgs &args, const std::string &paramName )
{
    if( paramName == "Help" )
    {
        sendMessage( OFX::Message::eMessageMessage,
                     "", // No XML resources
                     kInvertHelpString );
    }
}

}