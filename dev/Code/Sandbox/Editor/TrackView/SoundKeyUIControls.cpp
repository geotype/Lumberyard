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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"
#include "TrackViewTrack.h"

//////////////////////////////////////////////////////////////////////////
class CSoundKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSmartVariableArray mv_table;
    CSmartVariableArray mv_options;

    CSmartVariable<QString> mv_startTrigger;
    CSmartVariable<QString> mv_stopTrigger;
    CSmartVariable<float> mv_duration;
    CSmartVariable<Vec3> mv_customColor;

    virtual void OnCreateVars()
    {
        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_startTrigger, "StartTrigger", IVariable::DT_AUDIO_TRIGGER);
        AddVariable(mv_table, mv_stopTrigger, "StopTrigger", IVariable::DT_AUDIO_TRIGGER);
        AddVariable(mv_table, mv_duration, "Duration");
        AddVariable(mv_options, "Options");
        AddVariable(mv_options, mv_customColor, "Custom Color", IVariable::DT_COLOR);
    }
    bool SupportTrackType(const CAnimParamType& paramType, EAnimCurveType trackType, EAnimValue valueType) const
    {
        return paramType == eAnimParamType_Sound;
    }
    virtual bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys);

    virtual unsigned int GetPriority() const { return 1; }

    static const GUID& GetClassID()
    {
        // {AB2226E5-D593-49d2-B7CB-989412CAAEDE}
        static const GUID guid =
        {
            0xab2226e5, 0xd593, 0x49d2, { 0xb7, 0xcb, 0x98, 0x94, 0x12, 0xca, 0xae, 0xde }
        };
        return guid;
    }
};


//////////////////////////////////////////////////////////////////////////
bool CSoundKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType())
    {
        return false;
    }

    bool bAssigned = false;
    if (selectedKeys.GetKeyCount() == 1)
    {
        const CTrackViewKeyHandle& keyHandle = selectedKeys.GetKey(0);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == eAnimParamType_Sound)
        {
            ISoundKey soundKey;
            keyHandle.GetKey(&soundKey);

            mv_startTrigger = soundKey.sStartTrigger.c_str();
            mv_stopTrigger = soundKey.sStopTrigger.c_str();
            mv_duration = soundKey.fDuration;
            mv_customColor = soundKey.customColor;

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CSoundKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!pSequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    for (unsigned int keyIndex = 0; keyIndex < selectedKeys.GetKeyCount(); ++keyIndex)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == eAnimParamType_Sound)
        {
            ISoundKey soundKey;
            keyHandle.GetKey(&soundKey);
            bool bChangedSoundFile = false;

            if (pVar == mv_startTrigger.GetVar())
            {
                QString sFilename = mv_startTrigger;
                bChangedSoundFile = sFilename != soundKey.sStartTrigger.c_str();
                soundKey.sStartTrigger = sFilename.toLatin1().data();
            }
            else if (pVar == mv_stopTrigger.GetVar())
            {
                QString sFilename = mv_stopTrigger;
                bChangedSoundFile = sFilename != soundKey.sStopTrigger.c_str();
                soundKey.sStopTrigger = sFilename.toLatin1().data();
            }

            SyncValue(mv_duration, soundKey.fDuration, false, pVar);
            SyncValue(mv_customColor, soundKey.customColor, false, pVar);

            keyHandle.SetKey(&soundKey);
        }
    }
}

REGISTER_QT_CLASS_DESC(CSoundKeyUIControls, "TrackView.KeyUI.Sound", "TrackViewKeyUI");
