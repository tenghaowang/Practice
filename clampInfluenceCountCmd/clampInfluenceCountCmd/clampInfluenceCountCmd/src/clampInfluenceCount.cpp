#include "clampInfluenceCount.h"
#include <vector>
#include <algorithm>
#include <string>

#define CheckError(stat,msg)            \
        if ( MS::kSuccess != stat ) {   \
                displayError(msg);                      \
        }


clampInfluenceCountCommand::clampInfluenceCountCommand()
{
}


void* clampInfluenceCountCommand::creator()
{
    return new clampInfluenceCountCommand;
}


bool clampInfluenceCountCommand::isUndoable() const
{
    return false;
}


MSyntax clampInfluenceCountCommand::newSyntax()
{
    MSyntax syntax;
    
    syntax.addFlag( "-geo", "-geometry", MSyntax::kString );
	syntax.addFlag("-mi", "-maxInfluences", MSyntax::kDouble);

    return syntax;
}



MStatus clampInfluenceCountCommand::doIt( const MArgList& argList )
{
	MStatus status;
	status = parseArguments(argList);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	MSelectionList selection;
	selection.add(m_geoName);
	MObject m_geo;
	MDagPath m_path;
	status = selection.getDagPath(0, m_path);
	CheckError(status, " Cannot find specified geometry in the scene." );
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = getShapeNode(m_path);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	m_geo = m_path.node(&status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	// test to find skin cluster if object name is passed in 
	MObject m_sclObj;
	MItDependencyGraph itDG(m_geo, MFn::kSkinClusterFilter, MItDependencyGraph::kUpstream);
	for (; !itDG.isDone(); itDG.next())
	{
		m_sclObj = itDG.currentItem();
		break;
	}
	MFnSkinCluster skinCluster(m_sclObj, &status);
	CheckError(status, "Cannot get skincluster connected to the geometry.");
	CHECK_MSTATUS_AND_RETURN_IT(status);

	MFnSet fnSet(skinCluster.deformerSet());
	MSelectionList members;
	fnSet.getMembers(members, false);
	MDagPath geoDagPath;
	MObject components;
	members.getDagPath(0, geoDagPath, components);
	MDoubleArray weights;
	unsigned int numInfluenceObj;
	status = skinCluster.getWeights(geoDagPath, components, weights, numInfluenceObj);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	int modVertCount = 0;
	int numWeights;
	numWeights = weights.length();
	int numComponents;
	numComponents = numWeights / numInfluenceObj;
	MDoubleArray newWeights;
	std::vector<std::pair<double, int>> weightsPerComponent;

	// convert flag weights array to component based weights array
	for (unsigned int ii = 0; ii < numComponents; ii++)
	{
		// std::vector<std::pair<double, int>> weightsPerComponent;
		weightsPerComponent.clear();
		for (unsigned int kk = 0; kk < numInfluenceObj; kk++)
		{
			if (weights[numInfluenceObj * ii + kk] > 0.0)
			{
				std::pair<double, int> weightIndexPair = std::make_pair(weights[numInfluenceObj * ii + kk], kk);
				weightsPerComponent.push_back(weightIndexPair);
			}
		}

		//sort by descending
		std::sort(weightsPerComponent.begin(), weightsPerComponent.end());

		// if current influences is larger than max influences, remove influences with smallest weights
		if (weightsPerComponent.size() > m_maxInfluences)
		{
			MDoubleArray newWeightsPerComponent(numInfluenceObj, 0.0);
			// pop the smallest weights
			weightsPerComponent.erase(weightsPerComponent.begin(), weightsPerComponent.begin() + weightsPerComponent.size() - m_maxInfluences);
			// get weight sum
			double weightSum = 0.0;

			for (unsigned int ii = 0; ii < weightsPerComponent.size(); ii++)
			{
				weightSum = weightSum + weightsPerComponent[ii].first;
			}

#			// normalize current weights for the active component
			for (unsigned int ii = 0; ii < weightsPerComponent.size(); ii++)
			{
				newWeightsPerComponent[weightsPerComponent[ii].second] = weightsPerComponent[ii].first / weightSum;
			}
	
			// new weights array
			for (unsigned int ii = 0; ii < newWeightsPerComponent.length(); ii++)
			{
				newWeights.append(newWeightsPerComponent[ii]);
			}
	
			modVertCount += 1;
		}
		else
		{
			for (unsigned int kk = 0; kk< numInfluenceObj; kk++)
			{
				newWeights.append(weights[numInfluenceObj * ii + kk]);
			}
		}

	}
	// set skinCluter weights
	MIntArray influenceIndices;
	for (unsigned ii = 0; ii < numInfluenceObj; ii++)
	{
		influenceIndices.append(ii);
	}
	status = skinCluster.setWeights(geoDagPath, components, influenceIndices, newWeights, false);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	
	MString outputMessage("modified verts count: ");
	MGlobal::displayInfo(outputMessage + modVertCount);
	
	return MS::kSuccess;
}


MStatus clampInfluenceCountCommand::parseArguments(const MArgList& argList)
{
	MStatus status;
	// Read all the flag arguments
	MArgDatabase argData(syntax(), argList, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	
	if (argData.isFlagSet("-geo"))
	{
		m_geoName = argData.flagArgumentString("-geo", 0, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}
	else
	{
		m_geoName = "";
	}

	if (argData.isFlagSet("-mi"))
	{
		m_maxInfluences = argData.flagArgumentInt("-mi", 0, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}
	else
	{
		m_maxInfluences = 1;
	}

	return MS::kSuccess;
}


MStatus clampInfluenceCountCommand::getShapeNode(MDagPath& path)
{
	MStatus status;

	if (path.apiType() == MFn::kMesh)
	{
		return MS::kSuccess;
	}

	unsigned int numShapes;
	status = path.numberOfShapesDirectlyBelow(numShapes);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	for (unsigned int i = 0; i < numShapes; ++i)
	{
		status = path.extendToShapeDirectlyBelow(i);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		if (!path.hasFn(MFn::kMesh))
		{
			path.pop();
			continue;
		}

		MFnDagNode fnNode(path, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		if (!fnNode.isIntermediateObject())
		{
			return MS::kSuccess;
		}
		path.pop();
	}

	return MS::kFailure;
}


MStatus clampInfluenceCountCommand::undoIt()
{
    MStatus status;
	// undo not implemented
    return status;
}

