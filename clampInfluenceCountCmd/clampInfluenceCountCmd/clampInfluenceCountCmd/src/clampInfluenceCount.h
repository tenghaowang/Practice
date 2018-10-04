#include <maya/MArgDataBase.h>
#include <maya/MDagPath.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MMeshIntersector.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>
#include <maya/MPxCommand.h>
#include <maya/MSelectionList.h>
#include <maya/MSyntax.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MFnSet.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MDagPathArray.h>

class clampInfluenceCountCommand : public MPxCommand
{
public:
	clampInfluenceCountCommand();
    virtual MStatus doIt( const MArgList& argList );
    virtual MStatus undoIt();
    virtual bool isUndoable() const;
    static void* creator();
    static MSyntax newSyntax();

private:
	MStatus parseArguments(const MArgList& argList);
	MStatus getShapeNode(MDagPath& path);
	// args
	MString m_geoName;
	// MObject m_sclObj;
	int m_maxInfluences;

};


