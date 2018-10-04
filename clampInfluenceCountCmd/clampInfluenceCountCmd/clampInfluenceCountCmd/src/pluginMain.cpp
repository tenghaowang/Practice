#include "clampInfluenceCount.h"

#include <maya/MFnPlugin.h>

MStatus initializePlugin( MObject obj )
{
    MStatus status;

    MFnPlugin fnPlugin( obj, "Ryan Wang", "1.0", "Any" );

    status = fnPlugin.registerCommand( "clampInfluenceCount",
		clampInfluenceCountCommand::creator,
		clampInfluenceCountCommand::newSyntax );
    CHECK_MSTATUS_AND_RETURN_IT( status );

    return MS::kSuccess;
}


MStatus uninitializePlugin( MObject obj )
{
    MStatus status;

    MFnPlugin fnPlugin( obj );

    status = fnPlugin.deregisterCommand( "clampInfluenceCount" );
    CHECK_MSTATUS_AND_RETURN_IT( status );

    return MS::kSuccess;
}