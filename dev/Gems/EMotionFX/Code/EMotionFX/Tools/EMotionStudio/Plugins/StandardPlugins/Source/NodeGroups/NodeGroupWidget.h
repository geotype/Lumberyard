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

#pragma once

#include "../StandardPluginsConfig.h"
#include <MysticQt/Source/DialogStack.h>
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include "../../../../EMStudioSDK/Source/NodeSelectionWindow.h"

QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QTableWidget)
QT_FORWARD_DECLARE_CLASS(QTableWidgetItem)


namespace EMStudio
{
    class NodeGroupWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(NodeGroupWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        NodeGroupWidget(QWidget* parent = nullptr);
        ~NodeGroupWidget();

        void Init();
        void UpdateInterface();

        void SetActor(EMotionFX::Actor* actor);
        void SetNodeGroup(EMotionFX::NodeGroup* nodeGroup);
        void SetWidgetEnabled(bool enabled);

    public slots:
        void SelectNodesButtonPressed();
        void RemoveNodesButtonPressed();
        void NodeSelectionFinished(MCore::Array<SelectionItem> selectionList);
        void OnItemSelectionChanged();

    private:
        virtual void keyPressEvent(QKeyEvent* event) override;
        virtual void keyReleaseEvent(QKeyEvent* event) override;

    private:
        EMotionFX::Actor*               mActor;

        NodeSelectionWindow*            mNodeSelectionWindow;
        CommandSystem::SelectionList    mNodeSelectionList;
        EMotionFX::NodeGroup*           mNodeGroup;
        uint16                          mNodeGroupIndex;
        AZStd::string                   mNodeAction;

        // widgets
        QTableWidget*                   mNodeTable;
        QPushButton*                    mSelectNodesButton;
        QPushButton*                    mAddNodesButton;
        QPushButton*                    mRemoveNodesButton;
    };
} // namespace EMStudio