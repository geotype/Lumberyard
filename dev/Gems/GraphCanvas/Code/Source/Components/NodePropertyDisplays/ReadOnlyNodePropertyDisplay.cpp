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
#include "precompiled.h"

#include <Components/NodePropertyDisplays/ReadOnlyNodePropertyDisplay.h>

#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    ////////////////////////////////
    // ReadOnlyNodePropertyDisplay
    ////////////////////////////////

    ReadOnlyNodePropertyDisplay::ReadOnlyNodePropertyDisplay(ReadOnlyDataInterface* dataInterface)
        : m_dataInterface(dataInterface)
    {
        m_dataInterface->RegisterDisplay(this);
        
        m_disabledLabel = aznew GraphCanvasLabel();
        m_displayLabel = aznew GraphCanvasLabel();
    }
    
    ReadOnlyNodePropertyDisplay::~ReadOnlyNodePropertyDisplay()
    {
        delete m_dataInterface;
        
        delete m_displayLabel;
        delete m_disabledLabel;
    }

    void ReadOnlyNodePropertyDisplay::RefreshStyle()
    {
        m_disabledLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisabledLabelStyle("readOnly").c_str());
        m_displayLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisplayLabelStyle("readOnly").c_str());
    }
    
    void ReadOnlyNodePropertyDisplay::UpdateDisplay()
    {
        AZStd::string value = m_dataInterface->GetString();
        
        m_displayLabel->SetLabel(value);
        m_displayLabel->setToolTip(value.c_str());
    }

    QGraphicsLayoutItem* ReadOnlyNodePropertyDisplay::GetDisabledGraphicsLayoutItem() const
    {
        return m_disabledLabel;
    }

    QGraphicsLayoutItem* ReadOnlyNodePropertyDisplay::GetDisplayGraphicsLayoutItem() const
    {
        return m_displayLabel;
    }

    QGraphicsLayoutItem* ReadOnlyNodePropertyDisplay::GetEditableGraphicsLayoutItem() const
    {
        return m_displayLabel;
    }    
}