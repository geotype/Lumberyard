/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

// include the required headers
#include "SelectionCommands.h"
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>


namespace CommandSystem
{
    void SelectActorInstancesUsingCommands(const MCore::Array<EMotionFX::ActorInstance*>& selectedActorInstances)
    {
        SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        const uint32 numSelectedActorInstances = selectedActorInstances.GetLength();

        // check if the current selection is equal to the desired actor instances selection list
        bool nothingChanged = true;
        for (uint32 i = 0; i < numSelectedActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = selectedActorInstances[i];
            if (selection.CheckIfHasActorInstance(actorInstance) == false)
            {
                nothingChanged = false;
                break;
            }
        }
        for (uint32 i = 0; i < selection.GetNumSelectedActorInstances(); ++i)
        {
            EMotionFX::ActorInstance* actorInstance = selection.GetActorInstance(i);
            if (selectedActorInstances.Find(actorInstance) == MCORE_INVALIDINDEX32)
            {
                nothingChanged = false;
                break;
            }
        }

        if (nothingChanged == false)
        {
            // create our command group
            MCore::String outResult;
            MCore::CommandGroup commandGroup("Select actor instances");

            // clear the old selection
            commandGroup.AddCommandString("Unselect -actorInstanceID SELECT_ALL -actorID SELECT_ALL");

            // add the newly selected actor instances
            MCore::String commandString;
            for (uint32 a = 0; a < numSelectedActorInstances; ++a)
            {
                EMotionFX::ActorInstance* actorInstance = selectedActorInstances[a];
                commandString.Format("Select -actorInstanceID %i -actorID %i", actorInstance->GetID(), actorInstance->GetActor()->GetID());
                commandGroup.AddCommandString(commandString.AsChar());
            }

            // execute the group command
            if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
            {
                MCore::LogError(outResult.AsChar());
            }
        }
    }


    bool CheckIfHasMotionSelectionParameter(const MCore::CommandLine& parameters)
    {
        if (parameters.CheckIfHasParameter("motionName")   )
        {
            return true;
        }
        if (parameters.CheckIfHasParameter("motionIndex")  )
        {
            return true;
        }

        return false;
    }


    bool CheckIfHasAnimGraphSelectionParameter(const MCore::CommandLine& parameters)
    {
        if (parameters.CheckIfHasParameter("animGraphIndex")  )
        {
            return true;
        }
        if (parameters.CheckIfHasParameter("animGraphName")   )
        {
            return true;
        }
        if (parameters.CheckIfHasParameter("animGraphID")     )
        {
            return true;
        }

        return false;
    }


    bool CheckIfHasActorSelectionParameter(const MCore::CommandLine& parameters, bool ignoreInstanceParameters)
    {
        if (parameters.CheckIfHasParameter("actorID")   )
        {
            return true;
        }
        if (parameters.CheckIfHasParameter("actorName"))
        {
            return true;
        }
        if (ignoreInstanceParameters == false)
        {
            if (parameters.CheckIfHasParameter("actorInstance"))
            {
                return true;
            }
            if (parameters.CheckIfHasParameter("actorInstanceID")  )
            {
                return true;
            }
        }

        return false;
    }

    //--------------------------------------------------------------------------------
    // CommandSelect
    //--------------------------------------------------------------------------------

    // constructor
    CommandSelect::CommandSelect(MCore::Command* orgCommand)
        : MCore::Command("Select", orgCommand)
    {
    }


    // destructor
    CommandSelect::~CommandSelect()
    {
    }


    // the actual selection routine used by the select and the unselect command
    bool CommandSelect::Select(Command* command, const MCore::CommandLine& parameters, MCore::String& outResult, bool unselect)
    {
        // if we are in selection lock mode return directly
        //if (GetCommandManager()->GetLockSelection())
        //  return false;

        SelectionList& selection        = GetCommandManager()->GetCurrentSelection();
        const uint32 numActors          = EMotionFX::GetActorManager().GetNumActors();
        const uint32 numActorInstances  = EMotionFX::GetActorManager().GetNumActorInstances();
        const uint32 numMotions         = EMotionFX::GetMotionManager().GetNumMotions();
        const uint32 numAnimGraphs     = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();

        MCore::String valueString;

        // add the actor with the given index to the selection list
        if (parameters.CheckIfHasParameter("actorID"))
        {
            parameters.GetValue("actorID", command, &valueString);
            if (valueString.CheckIfIsEqualNoCase("SELECT_ALL"))
            {
                // iterate through all available actors and add them to the selection
                for (uint32 i = 0; i < numActors; ++i)
                {
                    EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);

                    if (actor->GetIsOwnedByRuntime())
                    {
                        continue;
                    }
                    
                    if (unselect == false)
                    {
                        selection.AddActor(actor);
                    }
                    else
                    {
                        selection.RemoveActor(actor);
                    }
                }
            }
            else
            {
                const uint32 actorID = parameters.GetValueAsInt("actorID", command);

                // find the actor based on the given id
                EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
                if (actor == nullptr)
                {
                    outResult.Format("Cannot select actor. Actor ID %i is not valid.", actorID);
                    return false;
                }

                if (actor->GetIsOwnedByRuntime())
                {
                    return false;
                }

                if (unselect == false)
                {
                    selection.AddActor(actor);
                }
                else
                {
                    selection.RemoveActor(actor);
                }
            }
        }


        // add the actor with the given name to the selection list
        if (parameters.CheckIfHasParameter("actorName"))
        {
            // get the actor name and check if it is valid
            parameters.GetValue("actorName", command, &valueString);
            if (valueString.GetIsEmpty())
            {
                outResult = "Actor name is empty. Cannot select actors with empty name.";
                return false;
            }

            // iterate through all available actors and add them to the selection
            for (uint32 i = 0; i < numActors; ++i)
            {
                EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);

                if (actor->GetIsOwnedByRuntime())
                {
                    continue;
                }

                if (valueString.CheckIfIsEqualNoCase(actor->GetName()))
                {
                    if (unselect == false)
                    {
                        selection.AddActor(actor);
                    }
                    else
                    {
                        selection.RemoveActor(actor);
                    }
                }
            }
        }


        // add the actor instance with the given index to the selection list
        if (parameters.CheckIfHasParameter("actorInstanceID"))
        {
            parameters.GetValue("actorInstanceID", command, &valueString);
            if (valueString.CheckIfIsEqualNoCase("SELECT_ALL"))
            {
                // iterate through all available actor instances and add them to the selection
                for (uint32 i = 0; i < numActorInstances; ++i)
                {
                    EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

                    if (actorInstance->GetIsOwnedByRuntime())
                    {
                        continue;
                    }

                    if (unselect == false)
                    {
                        selection.AddActorInstance(actorInstance);
                    }
                    else
                    {
                        selection.RemoveActorInstance(actorInstance);
                    }
                }
            }
            else
            {
                // get the actor instance from the string and check if it is valid
                const uint32 actorInstanceID = parameters.GetValueAsInt("actorInstanceID", command);
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
                if (actorInstance == nullptr)
                {
                    outResult.Format("Actor instance ID %i is not valid. There are no actor instances registered in the actor manager with the given ID.", actorInstanceID);
                    return false;
                }

                if (actorInstance->GetIsOwnedByRuntime())
                {
                    return false;
                }

                if (unselect == false)
                {
                    selection.AddActorInstance(actorInstance);
                }
                else
                {
                    selection.RemoveActorInstance(actorInstance);
                }
            }
        }


        // add the motion with the given name to the selection list
        if (parameters.CheckIfHasParameter("motionName"))
        {
            // get the motion name and check if it is valid
            parameters.GetValue("motionName", command, &valueString);
            if (valueString.GetIsEmpty())
            {
                outResult = "Motion name is empty. Cannot select motions with empty name.";
                return false;
            }

            // iterate through all available motions and add them to the selection
            for (uint32 i = 0; i < numMotions; ++i)
            {
                // get the current motion
                EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(i);

                if (motion->GetIsOwnedByRuntime())
                {
                    continue;
                }

                // compare the motion name to the parameter
                if (valueString.CheckIfIsEqualNoCase(motion->GetName()))
                {
                    if (unselect == false)
                    {
                        selection.AddMotion(motion);
                    }
                    else
                    {
                        selection.RemoveMotion(motion);
                    }
                }
            }
        }

        // add the motion with the given name to the selection list
        if (parameters.CheckIfHasParameter("motionIndex"))
        {
            parameters.GetValue("motionIndex", command, &valueString);
            if (valueString.CheckIfIsEqualNoCase("SELECT_ALL"))
            {
                // iterate through all available motions and add them to the selection
                for (uint32 i = 0; i < numMotions; ++i)
                {
                    // get the current motion
                    EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(i);

                    if (motion->GetIsOwnedByRuntime())
                    {
                        continue;
                    }

                    if (unselect == false)
                    {
                        selection.AddMotion(motion);
                    }
                    else
                    {
                        selection.RemoveMotion(motion);
                    }
                }
            }
            else
            {
                // get the motion index from the string and check if it is valid
                const uint32 motionIndex = parameters.GetValueAsInt("motionIndex", command);
                if (motionIndex >= numMotions)
                {
                    if (numMotions == 0)
                    {
                        outResult = "Motion index is not valid. There is no motion registered in the motion library.";
                    }
                    else
                    {
                        outResult.Format("Motion index '%i' is not valid. Valid range is [0, %i].", motionIndex, numMotions - 1);
                    }

                    return false;
                }

                // get the given motion from the motion library and add/remove it to the selection
                EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(motionIndex);

                if (!motion->GetIsOwnedByRuntime())
                {
                    return false;
                }

                if (unselect == false)
                {
                    selection.AddMotion(motion);
                }
                else
                {
                    selection.RemoveMotion(motion);
                }
            }
        }


        // add the anim graph with the given name to the selection list
        if (parameters.CheckIfHasParameter("animGraphIndex"))
        {
            parameters.GetValue("animGraphIndex", command, &valueString);
            if (valueString.CheckIfIsEqualNoCase("SELECT_ALL"))
            {
                // iterate through all available motions and add them to the selection
                for (uint32 i = 0; i < numAnimGraphs; ++i)
                {
                    // get the current anim graph
                    EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

                    if (animGraph->GetIsOwnedByRuntime())
                    {
                        continue;
                    }

                    if (unselect == false)
                    {
                        selection.AddAnimGraph(animGraph);
                    }
                    else
                    {
                        selection.RemoveAnimGraph(animGraph);
                    }
                }
            }
            else
            {
                // get the anim graph index from the string and check if it is valid
                const uint32 animGraphIndex = parameters.GetValueAsInt("animGraphIndex", command);
                if (animGraphIndex >= numAnimGraphs)
                {
                    if (numAnimGraphs == 0)
                    {
                        outResult = "Anim graph index is not valid. There is no anim graph registered in the anim graph manager.";
                    }
                    else
                    {
                        outResult.Format("Anim graph index '%i' is not valid. Valid range is [0, %i].", animGraphIndex, numAnimGraphs - 1);
                    }

                    return false;
                }

                // get the given anim graph from the motion library and add/remove it to the selection
                EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(animGraphIndex);

                if (animGraph->GetIsOwnedByRuntime())
                {
                    return false;
                }

                if (unselect == false)
                {
                    selection.AddAnimGraph(animGraph);
                }
                else
                {
                    selection.RemoveAnimGraph(animGraph);
                }
            }
        }


        // add the anim graph with the given id to the selection list
        if (parameters.CheckIfHasParameter("animGraphID"))
        {
            parameters.GetValue("animGraphID", command, &valueString);
            if (valueString.CheckIfIsEqualNoCase("SELECT_ALL"))
            {
                // iterate through all available motions and add them to the selection
                for (uint32 i = 0; i < numAnimGraphs; ++i)
                {
                    // get the current anim graph
                    EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

                    if (animGraph->GetIsOwnedByRuntime())
                    {
                        continue;
                    }

                    if (unselect == false)
                    {
                        selection.AddAnimGraph(animGraph);
                    }
                    else
                    {
                        selection.RemoveAnimGraph(animGraph);
                    }
                }
            }
            else
            {
                // get the anim graph id from the string and check if it is valid
                const uint32            animGraphID    = parameters.GetValueAsInt("animGraphID", command);
                EMotionFX::AnimGraph*  animGraph      = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
                if (animGraph == nullptr)
                {
                    outResult.Format("Anim graph id '%i' is not valid. Cannot find anim graph with the given id.", animGraphID);
                    return false;
                }

                if (animGraph->GetIsOwnedByRuntime())
                {
                    return false;
                }

                if (unselect == false)
                {
                    selection.AddAnimGraph(animGraph);
                }
                else
                {
                    selection.RemoveAnimGraph(animGraph);
                }
            }
        }


        // add the anim graph with the given name to the selection list
        if (parameters.CheckIfHasParameter("animGraphName"))
        {
            // get the anim graph name and check if it is valid
            parameters.GetValue("animGraphName", command, &valueString);
            if (valueString.GetIsEmpty())
            {
                outResult = "Anim graph name is empty. Cannot select anim graphs without a name.";
                return false;
            }

            // iterate through all available anim graphs and add them to the selection
            for (uint32 i = 0; i < numAnimGraphs; ++i)
            {
                // get the current motion
                EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

                if (animGraph->GetIsOwnedByRuntime())
                {
                    continue;
                }

                // compare the motion name to the parameter
                if (valueString.CheckIfIsEqualNoCase(animGraph->GetName()))
                {
                    if (unselect == false)
                    {
                        selection.AddAnimGraph(animGraph);
                    }
                    else
                    {
                        selection.RemoveAnimGraph(animGraph);
                    }
                }
            }
        }

        return true;
    }


    // execute the command
    bool CommandSelect::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // store the old selection list for undo
        mData = GetCommandManager()->GetCurrentSelection();

        // selection add mode
        return Select(this, parameters, outResult, false);
    }


    // undo the command
    bool CommandSelect::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        // restore the old selection and return success
        GetCommandManager()->SetCurrentSelection(mData);
        return true;
    }


    // init the syntax of the command
    void CommandSelect::InitSyntax()
    {
        GetSyntax().ReserveParameters(11);
        GetSyntax().AddParameter("actorName",              ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "unnamed");
        GetSyntax().AddParameter("actorID",                ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("actorInstanceID",        ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("motionName",             ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "unnamed");
        GetSyntax().AddParameter("motionIndex",            ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("motionInstanceIndex",    ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("nodeName",               ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "unnamed");
        GetSyntax().AddParameter("nodeIndex",              ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("animGraphName",         ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "unnamed");
        GetSyntax().AddParameter("animGraphIndex",        ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("animGraphID",           ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
    }


    // get the description
    const char* CommandSelect::GetDescription() const
    {
        return "Select a given item.";
    }


    //--------------------------------------------------------------------------------
    // CommandUnselect
    //--------------------------------------------------------------------------------

    // destructor
    CommandUnselect::~CommandUnselect()
    {
    }


    // execute the command
    bool CommandUnselect::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // store the old selection list for undo
        mData = GetCommandManager()->GetCurrentSelection();

        // unselect mode
        return CommandSelect::Select(this, parameters, outResult, true);
    }


    // undo the command
    bool CommandUnselect::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        // restore the old selection and return success
        GetCommandManager()->SetCurrentSelection(mData);
        return true;
    }


    // init the syntax of the command
    void CommandUnselect::InitSyntax()
    {
        GetSyntax().ReserveParameters(11);
        GetSyntax().AddParameter("actorName",              ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "unnamed");
        GetSyntax().AddParameter("actorID",                ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("actorInstanceID",        ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("motionName",             ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "unnamed");
        GetSyntax().AddParameter("motionIndex",            ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("motionInstanceIndex",    ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("nodeName",               ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "unnamed");
        GetSyntax().AddParameter("nodeIndex",              ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("animGraphName",         ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "unnamed");
        GetSyntax().AddParameter("animGraphIndex",        ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("animGraphID",           ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
    }


    // get the description
    const char* CommandUnselect::GetDescription() const
    {
        return "Unselect a given item.";
    }


    //--------------------------------------------------------------------------------
    // CommandClearSelection
    //--------------------------------------------------------------------------------

    // destructor
    CommandClearSelection::~CommandClearSelection()
    {
    }


    // execute the command
    bool CommandClearSelection::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        // get the current selection, store it for the undo function and unselect everything
        SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        mData = selection;

        // if we are in selection lock mode return directly
        //if (GetCommandManager()->GetLockSelection())
        //  return false;

        // clear the selection
        selection.Clear();
        MCORE_ASSERT(selection.GetIsEmpty());

        return true;
    }


    // undo the command
    bool CommandClearSelection::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        // restore the old selection and return success
        GetCommandManager()->SetCurrentSelection(mData);
        return true;
    }


    // init the syntax of the command
    void CommandClearSelection::InitSyntax()
    {
    }


    // get the description
    const char* CommandClearSelection::GetDescription() const
    {
        return "This command can be used to unselect all objects.";
    }


    //--------------------------------------------------------------------------------
    // CommandToggleLockSelection
    //--------------------------------------------------------------------------------

    // destructor
    CommandToggleLockSelection::~CommandToggleLockSelection()
    {
    }


    // execute the command
    bool CommandToggleLockSelection::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        // store the selection locked flag for the undo function
        mData = GetCommandManager()->GetLockSelection();

        // toggle the flag
        GetCommandManager()->SetLockSelection(!mData);

        return true;
    }


    // undo the command
    bool CommandToggleLockSelection::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        // restore the old selection locked flag and return success
        GetCommandManager()->SetLockSelection(mData);
        return true;
    }


    // init the syntax of the command
    void CommandToggleLockSelection::InitSyntax()
    {
    }


    // get the description
    const char* CommandToggleLockSelection::GetDescription() const
    {
        return "This command can be used to (un)lock the selection.";
    }
} // namespace CommandSystem
