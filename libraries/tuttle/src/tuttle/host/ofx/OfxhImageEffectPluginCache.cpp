#include "OfxhImageEffectPluginCache.hpp"

///@todo tuttle: remove this!
#include <tuttle/host/core/Core.hpp>

namespace tuttle {
namespace host {
namespace ofx {
namespace imageEffect {


OfxhImageEffectPluginCache::OfxhImageEffectPluginCache( tuttle::host::ofx::imageEffect::OfxhImageEffectHost& host )
	: OfxhPluginAPICacheI( kOfxImageEffectPluginApi, 1, 1 ),
	_currentPlugin( 0 ),
	_currentProp( 0 ),
	_currentContext( 0 ),
	_currentParam( 0 ),
	_currentClip( 0 ),
	_host( &host ) {}

OfxhImageEffectPluginCache::~OfxhImageEffectPluginCache() {}

/**
 * get the plugin by id.  vermaj and vermin can be specified.  if they are not it will
 * pick the highest found version.
 */
OfxhImageEffectPlugin* OfxhImageEffectPluginCache::getPluginById( const std::string& id, int vermaj, int vermin )
{
	// return the highest version one, which fits the pattern provided
	OfxhImageEffectPlugin* sofar = 0;

	for( std::vector<OfxhImageEffectPlugin*>::iterator i = _plugins.begin(); i != _plugins.end(); ++i )
	{
		OfxhImageEffectPlugin* p = *i;

		if( p->getIdentifier() != id )
		{
			continue;
		}

		if( vermaj != -1 && p->getVersionMajor() != vermaj )
		{
			continue;
		}

		if( vermin != -1 && p->getVersionMinor() != vermin )
		{
			continue;
		}

		if( !sofar || p->trumps( sofar ) )
		{
			sofar = p;
		}
	}
	return sofar;
}

/// whether we support this plugin.
bool OfxhImageEffectPluginCache::pluginSupported( tuttle::host::ofx::OfxhPlugin* p, std::string& reason ) const
{
	return core::Core::instance().getHost().pluginSupported( dynamic_cast<tuttle::host::ofx::imageEffect::OfxhImageEffectPlugin*>( p ), reason );
}

/**
 * get the plugin by label. vermaj and vermin can be specified.  if they are not it will
 * pick the highest found version.
 */
OfxhImageEffectPlugin* OfxhImageEffectPluginCache::getPluginByLabel( const std::string& label, int vermaj, int vermin )
{
	// return the highest version one, which fits the pattern provided
	OfxhImageEffectPlugin* sofar = 0;

	for( std::vector<OfxhImageEffectPlugin*>::iterator i = _plugins.begin(); i != _plugins.end(); ++i )
	{
		OfxhImageEffectPlugin* p = *i;

		if( p->getDescriptor().getProperties().getStringProperty( kOfxPropLabel ) != label )
		{
			continue;
		}

		if( vermaj != -1 && p->getVersionMajor() != vermaj )
		{
			continue;
		}

		if( vermin != -1 && p->getVersionMinor() != vermin )
		{
			continue;
		}

		if( !sofar || p->trumps( sofar ) )
		{
			sofar = p;
		}
	}

	return sofar;
}

const std::vector<OfxhImageEffectPlugin*>& OfxhImageEffectPluginCache::getPlugins() const
{
	return _plugins;
}

const std::map<std::string, OfxhImageEffectPlugin*>& OfxhImageEffectPluginCache::getPluginsByID() const
{
	return _pluginsByID;
}

/**
 * handle the case where the info needs filling in from the file.
 * Runs the "describe" action on the plugin.
 */
void OfxhImageEffectPluginCache::loadFromPlugin( OfxhPlugin* op ) const
{
	std::string msg = "loading ";

	msg += op->getRawIdentifier();

	_host->loadingStatus( msg );

	OfxhImageEffectPlugin* p = dynamic_cast<OfxhImageEffectPlugin*>( op );
	assert( p );

	OfxhPluginHandle plug( p, _host );

	int rval = plug->mainEntry( kOfxActionLoad, 0, 0, 0 );

	if( rval != kOfxStatOK && rval != kOfxStatReplyDefault )
	{
		throw core::exception::LogicError( "Load failed on plugin " + op->getIdentifier() );
	}

	rval = plug->mainEntry( kOfxActionDescribe, p->getDescriptor().getHandle(), 0, 0 );

	if( rval != kOfxStatOK && rval != kOfxStatReplyDefault )
	{
		throw core::exception::LogicError( "Describe failed on plugin " + op->getIdentifier() );
	}

	const imageEffect::OfxhImageEffectNodeDescriptor& e = p->getDescriptor();
	const property::OfxhSet& eProps                     = e.getProperties();

	int size = eProps.getDimension( kOfxImageEffectPropSupportedContexts );

	for( int j = 0; j < size; ++j )
	{
		std::string context = eProps.getStringProperty( kOfxImageEffectPropSupportedContexts, j );
		p->addContext( context );
	}

	rval = plug->mainEntry( kOfxActionUnload, 0, 0, 0 );

	if( rval != kOfxStatOK && rval != kOfxStatReplyDefault )
	{
		throw core::exception::LogicError( "Unload failed on plugin " + op->getIdentifier() + " at initialization." );
	}
}

/**
 * handler for preparing to read in a chunk of XML from the cache, set up context to do this
 */
void OfxhImageEffectPluginCache::beginXmlParsing( OfxhPlugin* p )
{
	_currentPlugin = dynamic_cast<OfxhImageEffectPlugin*>( p );
}

/**
 * XML handler : element begins (everything is stored in elements and attributes)
 */
void OfxhImageEffectPluginCache::xmlElementBegin( const std::string& el, std::map<std::string, std::string> map )
{
	if( el == "apiproperties" )
	{
		return;
	}

	if( el == "context" )
	{
		_currentContext = core::Core::instance().getHost().makeDescriptor( _currentPlugin->getBinary()->getBundlePath(), _currentPlugin );
		_currentPlugin->addContext( map["name"], _currentContext );
		return;
	}

	if( el == "param" && _currentContext )
	{
		std::string pname = map["name"];
		std::string ptype = map["type"];

		_currentParam = _currentContext->paramDefine( ptype.c_str(), pname.c_str() );
		return;
	}

	if( el == "clip" && _currentContext )
	{
		std::string cname = map["name"];

		_currentClip = new attribute::OfxhClipImageDescriptor( cname );
		_currentContext->addClip( cname, _currentClip );
		return;
	}

	if( _currentContext && _currentParam )
	{
		APICache::propertySetXMLRead( el, map, _currentParam->getEditableProperties(), _currentProp );
		return;
	}

	if( _currentContext && _currentClip )
	{
		APICache::propertySetXMLRead( el, map, _currentClip->getEditableProperties(), _currentProp );
		return;
	}

	if( !_currentContext && !_currentParam )
	{
		APICache::propertySetXMLRead( el, map, _currentPlugin->getDescriptor().getEditableProperties(), _currentProp );
		return;
	}

	std::cout << "element " << el << "\n";
	assert( false );
}

void OfxhImageEffectPluginCache::xmlCharacterHandler( const std::string& ) {}

void OfxhImageEffectPluginCache::xmlElementEnd( const std::string& el )
{
	if( el == "param" )
	{
		_currentParam = 0;
	}

	if( el == "context" )
	{
		_currentContext = 0;
	}
}

void OfxhImageEffectPluginCache::endXmlParsing()
{
	_currentPlugin = 0;
}

void OfxhImageEffectPluginCache::saveXML( const OfxhPlugin* const ip, std::ostream& os ) const
{
	const OfxhImageEffectPlugin* const p = dynamic_cast<const OfxhImageEffectPlugin* const>( ip );

	p->saveXML( os );
}

void OfxhImageEffectPluginCache::confirmPlugin( OfxhPlugin* p )
{
	OfxhImageEffectPlugin* plugin = dynamic_cast<OfxhImageEffectPlugin*>( p );

	_plugins.push_back( plugin );

	if( _pluginsByID.find( plugin->getIdentifier() ) != _pluginsByID.end() )
	{
		OfxhImageEffectPlugin* otherPlugin = _pluginsByID[plugin->getIdentifier()];
		if( plugin->trumps( otherPlugin ) )
		{
			_pluginsByID[plugin->getIdentifier()] = plugin;
		}
	}
	else
	{
		_pluginsByID[plugin->getIdentifier()] = plugin;
	}

	OfxhMajorPlugin maj( plugin );

	if( _pluginsByIDMajor.find( maj ) != _pluginsByIDMajor.end() )
	{
		OfxhImageEffectPlugin* otherPlugin = _pluginsByIDMajor[maj];
		if( plugin->getVersionMajor() != otherPlugin->getVersionMajor() || plugin->trumps( otherPlugin ) )
		{
			_pluginsByIDMajor[maj] = plugin;
		}
	}
	else
	{
		_pluginsByIDMajor[maj] = plugin;
	}
}

OfxhPlugin* OfxhImageEffectPluginCache::newPlugin( OfxhPluginBinary* pb,
                                                   int               pi,
                                                   OfxPlugin*        pl )
{
	OfxhImageEffectPlugin* plugin = new OfxhImageEffectPlugin( *this, pb, pi, pl );

	return plugin;
}

OfxhPlugin* OfxhImageEffectPluginCache::newPlugin( OfxhPluginBinary*  pb,
                                                   int                pi,
                                                   const std::string& api,
                                                   int                apiVersion,
                                                   const std::string& pluginId,
                                                   const std::string& rawId,
                                                   int                pluginMajorVersion,
                                                   int                pluginMinorVersion )
{
	OfxhImageEffectPlugin* plugin = new OfxhImageEffectPlugin( *this, pb, pi, api, apiVersion, pluginId, rawId, pluginMajorVersion, pluginMinorVersion );

	return plugin;
}

void OfxhImageEffectPluginCache::dumpToStdOut() const
{
	if( _pluginsByID.empty() )
		std::cout << "No Plug-ins Found." << std::endl;

	std::cout << "________________________________________________________________________________" << std::endl;
	for( std::map<std::string, OfxhImageEffectPlugin*>::const_iterator it = _pluginsByID.begin(); it != _pluginsByID.end(); ++it )
	{
		std::cout << "Plug-in:" << it->first << std::endl;
		std::cout << "\t" << "Filepath: " << it->second->getBinary()->getFilePath();
		std::cout << "(" << it->second->getIndex() << ")" << std::endl;

		std::cout << "Contexts:" << std::endl;
		const std::set<std::string>& contexts = it->second->getContexts();
		for( std::set<std::string>::const_iterator it2 = contexts.begin(); it2 != contexts.end(); ++it2 )
			std::cout << "\t* " << *it2 << std::endl;
		const OfxhImageEffectNodeDescriptor& d = it->second->getDescriptor();
		std::cout << "Inputs:" << std::endl;
		const std::map<std::string, attribute::OfxhClipImageDescriptor*>& inputs = d.getClips();
		for( std::map<std::string, attribute::OfxhClipImageDescriptor*>::const_iterator it2 = inputs.begin(); it2 != inputs.end(); ++it2 )
			std::cout << "\t\t* " << it2->first << std::endl;
		std::cout << "________________________________________________________________________________" << std::endl;
	}
}

}
}
}
}
