"""
Tecent Test: clamp vert influence count command plugin
import maya.cmds as cmds
# copy the file and paste it to MAYA_PLUG_IN_PATH
cmds.loadPlugin( 'clampInfluenceCount.py' )
cmds.clampInfluenceCount(geo=meshName, mi=1)
"""
import sys
import maya.OpenMayaMPx as OpenMayaMPx
import maya.OpenMaya as OpenMaya
import maya.OpenMayaAnim as OpenMayaAnim
import pymel.core as pm

pluginCmdName = 'clampInfluenceCount'

maxInfShortFlag = '-mi'
maxInfLongFlag = '-maxInfluences'

geoShortFlag = '-geo'
geoLongFlag = '-geometry'

##########################################################
# Plug-in
##########################################################


class ClampVertInfluenceCount(OpenMayaMPx.MPxCommand):

    def __init__(self):
        """ Constructor. """
        OpenMayaMPx.MPxCommand.__init__(self)
        # initiate args
        self.geo = None
        self.maxInfluences = None

    def doIt(self, args):
        """ Command execution. """

        # parsing arguments first.
        self.parseArguments(args)
        self.clampVertInfluenceCount()
        # Remove the following 'pass' keyword and replace it with the code you want to run.

    def clampVertInfluenceCount(self):
        """
        Clamps the influence count for all vertices on given geometry so that
        no vertex has more influences than the amount specified by maxInfluences.
        The lowest influence weights for each vertex are trimmed and the remaining
        influence weights are normalized.

        If maxInfluences is None, the maximum influence value on the skinCluster deforming
        the given geo will be used

        Return:
            return the # of verts that were modified
        """
        if self.geo is None:
            activeSelection = pm.ls(sl=True)
            if activeSelection:
                self.geo = activeSelection[0]
            else:
                raise Exception("Found no geometry, use 'geo' flag or select a mesh in the scene.")

        skinCluster = pm.listHistory(self.geo, type='skinCluster')
        if not skinCluster:
            raise Exception("'{0}' is not deformed by a skinCluster".format(self.geo))

        skinCluster = skinCluster[0]
        # get skin
        self.mfnSkinCluster = self.getMfnSkinCluster(skinCluster.name())

        if self.maxInfluences is None:
            self.maxInfluences = skinCluster.getMaximumInfluences()

        if self.maxInfluences < 1:
            raise ValueError("maxInfluences must be 1 or greater, not {0}".format(self.maxInfluences))

        self.dagPath, self.components = self.getGeomInfo()
        # get flat weight list (see MFnSkinCluster.setWeights for a description)
        weights = self.gatherInfluenceWeights()
        # init new weight MDoubleArray
        newWeights = OpenMaya.MDoubleArray()

        mod_vert_count = 0

        for vertexId in range(self.numComponents):

            weightsPerComponent = weights[self.numInfluenceObj * vertexId: self.numInfluenceObj * vertexId + self.numInfluenceObj]

            influenceWeights = [(index, weight) for index, weight in enumerate(weightsPerComponent) if weight]

            if len(influenceWeights) > self.maxInfluences:
                mod_vert_count += 1

                newWeightsPerComponent = [0] * self.numInfluenceObj

                # sort by weight
                influenceWeights.sort(key=lambda x: x[1])
                # drop the lowest weights
                influencesToKeep = influenceWeights[-self.maxInfluences:]

                weightSum = sum([v for i, v in influencesToKeep])

                for i, v in influencesToKeep:
                    newWeightsPerComponent[i] = v / weightSum

                for weight in newWeightsPerComponent:
                    newWeights.append(weight)
            else:
                for weight in weightsPerComponent:
                    newWeights.append(weight)

        self.setInfluenceWeights(newWeights)
        print '{0} verts were modified'.format(mod_vert_count)
        return mod_vert_count

    def getMfnSkinCluster(self, skinCluster):
        """pass skin cluster name(str) and return MFnSkincluster obj"""
        selectionList = OpenMaya.MSelectionList()
        selectionList.add(skinCluster)
        mObj_skinCluster = OpenMaya.MObject()
        selectionList.getDependNode(0, mObj_skinCluster)
        mfnSkinCluster = OpenMayaAnim.MFnSkinCluster(mObj_skinCluster)
        return mfnSkinCluster

    def getGeomInfo(self):
        """return geom info"""
        fnSet = OpenMaya.MFnSet(self.mfnSkinCluster.deformerSet())
        memebers = OpenMaya.MSelectionList()
        fnSet.getMembers(memebers, False)
        dagPath = OpenMaya.MDagPath()
        components = OpenMaya.MObject()
        memebers.getDagPath(0, dagPath, components)
        return dagPath, components

    def gatherInfluenceWeights(self):
        """gather skincluster weights data"""
        weights = OpenMaya.MDoubleArray()
        util = OpenMaya.MScriptUtil()
        util.createFromInt(0)
        pUInt = util.asUintPtr()
        self.mfnSkinCluster.getWeights(self.dagPath, self.components, weights, pUInt)
        # get influences and component count
        mDagPathArray = OpenMaya.MDagPathArray()
        self.numInfluenceObj = self.mfnSkinCluster.influenceObjects(mDagPathArray)
        numWeights = weights.length()
        self.numComponents = numWeights / self.numInfluenceObj

        return weights

    def setInfluenceWeights(self, weights):
        mDagPathArray = OpenMaya.MDagPathArray()
        numInfuluenceObj = self.mfnSkinCluster.influenceObjects(mDagPathArray)
        influenceIndices = OpenMaya.MIntArray()
        for i in xrange(numInfuluenceObj):
            influenceIndices.append(i)
        self.mfnSkinCluster.setWeights(self.dagPath, self.components, influenceIndices, weights, False)

    def parseArguments(self, args):
        """
        The presence of this function is not enforced,
        but helps separate argument parsing code from other
        command code.
        """

        # The following MArgParser object allows you to check if specific flags are set.
        argData = OpenMaya.MArgParser(self.syntax(), args)
        # Args:
        # geo: geometry name, if None, try to get current selected obj instead
        # maxInfluences: The maximum number of influences allowed per vertex. If None,
        # the maxInfluences value on the skinCluster deforming the given geo
        # will be used
        if argData.isFlagSet(geoShortFlag):
            self.geo = argData.flagArgumentString(geoShortFlag, 0)

        if argData.isFlagSet(maxInfShortFlag):
            self.maxInfluences = argData.flagArgumentInt(maxInfShortFlag, 0)

##########################################################
# Plug-in initialization.
##########################################################


def cmdCreator():
    """ Create an instance of our command. """
    return OpenMayaMPx.asMPxPtr(ClampVertInfluenceCount())


def syntaxCreator():
    """ Defines the argument and flag syntax for this command. """
    syntax = OpenMaya.MSyntax()

    # geometry will be expecting a string value, denoted by OpenMaya.MSyntax.
    syntax.addFlag(geoShortFlag, geoLongFlag, OpenMaya.MSyntax.kString)
    # max influences will be expecting a numeric value, denoted by OpenMaya.MSyntax.kDouble.
    syntax.addFlag(maxInfShortFlag, maxInfLongFlag, OpenMaya.MSyntax.kDouble)

    return syntax


def initializePlugin(mobject):
    """ Initialize the plug-in when Maya loads it. """
    mplugin = OpenMayaMPx.MFnPlugin(mobject)
    try:
        mplugin.registerCommand(pluginCmdName, cmdCreator, syntaxCreator)
    except:
        sys.stderr.write('Failed to register command: ' + pluginCmdName)


def uninitializePlugin(mobject):
    """ Uninitialize the plug-in when Maya un-loads it. """
    mplugin = OpenMayaMPx.MFnPlugin(mobject)
    try:
        mplugin.deregisterCommand(pluginCmdName)
    except:
        sys.stderr.write('Failed to unregister command: ' + pluginCmdName)
