#ifndef PNG_WRITER_PLUGIN_H
#define PNG_WRITER_PLUGIN_H

#include <ofxsImageEffect.h>
#include <boost/gil/gil_all.hpp>

namespace tuttle {
using namespace boost::gil;

static const std::string kInputFilename = "Output filename";
static const std::string kInput = "Input";
static const std::string kRender = "Render";
static const std::string kRenderSecret = "RenderSecret";

/**
 * @brief Class used to denoise with partial derivated equations
 *
 */
class PNGWriterPlugin : public OFX::ImageEffect
{
public:
    PNGWriterPlugin( OfxImageEffectHandle handle );
    OFX::Clip *getSrcClip( ) const;
    OFX::Clip *getDstClip( ) const;

public:
    virtual void render( const OFX::RenderArguments &args );
    void changedParam( const OFX::InstanceChangedArgs &args, const std::string &paramName );
protected:
    OFX::PushButtonParam    *_renderButton;   ///< Render push button
    OFX::StringParam        *_filepath;       ///< Target file path
    // do not need to delete these, the ImageEffect is managing them for us
    OFX::Clip               *_srcClip;        ///< Input image clip
    OFX::Clip               *_dstClip;        ///< Ouput image clip
    bool                     _bRenderOnce;    ///< Avoid multiple useless writing
};

}

#endif