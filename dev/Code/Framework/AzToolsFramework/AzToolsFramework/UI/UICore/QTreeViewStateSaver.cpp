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

#include "stdafx.h"
#include "QTreeViewStateSaver.hxx"

#include <qtreeview.h>
#include <qscrollbar.h>

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/UserSettings/UserSettings.h>

namespace AzToolsFramework
{
    class QTreeViewStateSaverData
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(QTreeViewStateSaverData, "{CA0FBE7A-232C-4595-9824-F4B5C50FA7B4}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(QTreeViewStateSaverData, AZ::SystemAllocator, 0);

        AZStd::vector<AZStd::string> m_expandedElements;
        AZStd::vector<AZStd::string> m_selectedElements;
        AZStd::string m_currentElement;
        int m_horizScrollLast;
        int m_vertScrollLast;
        bool m_bApplyingState;

        QTreeViewStateSaverData()
            : m_horizScrollLast(0)
            , m_vertScrollLast(0)
            , m_bApplyingState(false)
        {
        }

        virtual ~QTreeViewStateSaverData()
        {
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<QTreeViewStateSaverData, AZ::UserSettings>()
                    ->Version(1)
                    ->Field("m_expandedElements", &QTreeViewStateSaverData::m_expandedElements)
                    ->Field("m_selectedElements", &QTreeViewStateSaverData::m_selectedElements)
                    ->Field("m_currentElement", &QTreeViewStateSaverData::m_currentElement)
                    ->Field("m_horizScrollLast", &QTreeViewStateSaverData::m_horizScrollLast)
                    ->Field("m_vertScrollLast", &QTreeViewStateSaverData::m_vertScrollLast);
            }
        }

        void RecurseCaptureSnapshot(const QModelIndex& idxParent, AZStd::string pathSoFar, QTreeView* treeView)
        {
            int elements = treeView->model()->rowCount(idxParent);
            for (int idx = 0; idx < elements; ++idx)
            {
                QModelIndex rowIdx = treeView->model()->index(idx, 0, idxParent);
                AZStd::string displayString(treeView->model()->data(rowIdx, Qt::DisplayRole).toString().toUtf8().data());
                if (!displayString.empty())
                {
                    AZStd::string actualPath = displayString;
                    if (!pathSoFar.empty())
                    {
                        actualPath = pathSoFar + "/" + displayString;
                    }

                    if (treeView->isExpanded(rowIdx))
                    {
                        m_expandedElements.push_back(actualPath);
                    }

                    if (treeView->selectionModel() && treeView->selectionModel()->isSelected(rowIdx))
                    {
                        m_selectedElements.push_back(actualPath);
                    }

                    RecurseCaptureSnapshot(rowIdx, actualPath, treeView);
                }
            }
        }

        void CaptureSnapshot(QTreeView* treeView)
        {
            Q_ASSERT(treeView && treeView->model());

            m_selectedElements.clear();
            m_expandedElements.clear();
            RecurseCaptureSnapshot(QModelIndex(), AZStd::string(), treeView);

            _q_CurrentChanged(treeView->currentIndex(), QModelIndex());

            QScrollBar* pScroll = treeView->verticalScrollBar();
            if (pScroll)
            {
                m_vertScrollLast = pScroll->value();
            }
            else
            {
                m_vertScrollLast = 0;
            }

            pScroll = treeView->horizontalScrollBar();
            if (pScroll)
            {
                m_horizScrollLast = pScroll->value();
            }
            else
            {
                m_horizScrollLast = 0;
            }
        }
        
        static void ExpandRow(QTreeView* treeView, const QModelIndex& rowIdx)
        {
            if (!treeView->isExpanded(rowIdx))
            {
                treeView->expand(rowIdx);
            }
        }

        static void SelectRow(QTreeView* treeView, const QModelIndex& rowIdx)
        {
            if (treeView->selectionModel() && !treeView->isExpanded(rowIdx.parent()))
            {
                treeView->selectionModel()->select(rowIdx, QItemSelectionModel::Select);
            }
        }

        static void SetCurrentRow(QTreeView* treeView, const QModelIndex& rowIdx)
        {
            if (treeView->isExpanded(rowIdx.parent()))
            {
                treeView->setCurrentIndex(rowIdx);
            }
        }

        void ApplyRowOperation(QTreeView* treeView, const QModelIndex& idxParent, AZStd::string pathSoFar, 
            const AZStd::function<void(QTreeView* treeView, const QModelIndex&)>& rowOperation)
        {
            AZStd::size_t pos = pathSoFar.find_first_of('/');
            AZStd::string name = pathSoFar.substr(0, pos);
            int elements = treeView->model()->rowCount(idxParent);
            for (int idx = 0; idx < elements; ++idx)
            {
                QModelIndex rowIdx = treeView->model()->index(idx, 0, idxParent);
                AZStd::string displayString(treeView->model()->data(rowIdx, Qt::DisplayRole).toString().toUtf8().data());
                if (name == displayString)
                {                    
                    if (pos != AZStd::string::npos)
                    {
                        AZStd::string remaining = pathSoFar.substr(pos + 1);
                        ApplyRowOperation(treeView, rowIdx, remaining, rowOperation);
                    }
                    else
                    {
                        rowOperation(treeView, rowIdx);
                    }
                    return;
                }
            }
        }

        void ApplySnapshot(QTreeView* treeView, const AZStd::function<bool(const QModelIndex&)>& /* expandedFunction */)
        {
            Q_ASSERT(treeView && treeView->model());

            m_bApplyingState = true;

            // Reset the view to the 'default'/fully negative state then we can apply our positive transforms.
            treeView->collapseAll();
            treeView->clearSelection();

            //RecurseApplySnapshot(QModelIndex(), "", treeView, expandedFunction);

            for (auto& expanded : m_expandedElements)
            {
                ApplyRowOperation(treeView, QModelIndex(), expanded, &QTreeViewStateSaverData::ExpandRow);
            }

            for (auto& selected : m_selectedElements)
            {
                ApplyRowOperation(treeView, QModelIndex(), selected, &QTreeViewStateSaverData::SelectRow);
            }

            if (!m_currentElement.empty()) 
            {
                ApplyRowOperation(treeView, QModelIndex(), m_currentElement, &QTreeViewStateSaverData::SetCurrentRow);
            }

            // force a refresh of the scroll bar:
            treeView->scrollToBottom();
            treeView->scrollToTop();

            QScrollBar* pScroll = treeView->verticalScrollBar();
            if (pScroll)
            {
                pScroll->setValue(m_vertScrollLast);
            }

            pScroll = treeView->horizontalScrollBar();
            if (pScroll)
            {
                pScroll->setValue(m_horizScrollLast);
            }
            m_bApplyingState = false;
        }

        QString GenerateIdentifier(const QModelIndex& modelIndex)
        {
            QString dataAtThatRow = modelIndex.data(Qt::DisplayRole).toString();
            QModelIndex idx = modelIndex.parent();
            while (idx.isValid())
            {
                dataAtThatRow = idx.data(Qt::DisplayRole).toString() + "/" + dataAtThatRow;
                QModelIndex newIdx = idx.parent(); // its not safe to iterate on itself!
                idx = newIdx;
            }
            return dataAtThatRow;
        }

        void _q_VerticalScrollChanged(int newValue)
        {
            if (m_bApplyingState)
            {
                return;
            }
            m_vertScrollLast = newValue;
        }

        void _q_HorizontalScrollChanged(int newValue)
        {
            if (m_bApplyingState)
            {
                return;
            }
            m_horizScrollLast = newValue;
        }

        void _q_ModelReset()
        {
            if (m_bApplyingState)
            {
                return;
            }

            m_selectedElements.clear();
        }

        void _q_CurrentChanged(const QModelIndex& current, const QModelIndex&)
        {
            if (m_bApplyingState)
            {
                return;
            }

            m_currentElement = "";
            if (current != QModelIndex())
            {
                QString id = GenerateIdentifier(current);
                if (!id.isEmpty())
                {
                    m_currentElement = id.toUtf8().data();
                }
            }
        }

        void _q_selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
        {
            if (m_bApplyingState)
            {
                return;
            }

            QModelIndexList allsel = selected.indexes();
            for (auto it = allsel.begin(); it != allsel.end(); ++it)
            {
                QModelIndex& targ = *it;
                QString id = GenerateIdentifier(targ);
                if (!id.isEmpty())
                {
                    m_selectedElements.push_back(id.toUtf8().data());
                }
            }

            allsel = deselected.indexes();
            for (auto it = allsel.begin(); it != allsel.end(); ++it)
            {
                QModelIndex& targ = *it;
                QString id = GenerateIdentifier(targ);
                auto itToDelete = AZStd::find(m_selectedElements.begin(), m_selectedElements.end(), id.toUtf8().data());
                if (itToDelete != m_selectedElements.end())
                {
                    m_selectedElements.erase(itToDelete);
                }
            }
        }

        void _q_RowExpanded(const QModelIndex& modelIndex)
        {
            if (m_bApplyingState)
            {
                return;
            }

            QString id = GenerateIdentifier(modelIndex);
            m_expandedElements.push_back(id.toUtf8().data());
        }

        void _q_RowCollapsed(const QModelIndex& modelIndex)
        {
            if (m_bApplyingState)
            {
                return;
            }

            QString id = GenerateIdentifier(modelIndex);
            auto itToDelete = AZStd::find(m_expandedElements.begin(), m_expandedElements.end(), id.toUtf8().data());
            if (itToDelete != m_expandedElements.end())
            {
                m_expandedElements.erase(itToDelete);
            }
        }
    };

    QTreeViewStateSaver::QTreeViewStateSaver(AZ::u32 storageID, QObject* pParent)
        : QObject(pParent)
        , m_data(AZ::UserSettings::CreateFind<QTreeViewStateSaverData>(storageID, AZ::UserSettings::CT_LOCAL))
        , m_expandedFunction(&QTreeViewStateSaver::DefaultExpandedFunction)
    {
    }

    void QTreeViewStateSaver::Attach(QTreeView* attach, QAbstractItemModel* dataModel, QItemSelectionModel* selectionModel)
    {
        // these all have to match by the time we attach
        Q_ASSERT(attach);
        Q_ASSERT(attach->model() == dataModel);
        Q_ASSERT(attach->selectionModel() == selectionModel);

        if (attach != m_attachedTree)
        {
            DetachTreeView();

            AttachTreeView(attach);
        }

        if (selectionModel != m_selectionModel)
        {
            DetachSelectionModel();

            AttachSelectionModel(selectionModel);
        }

        if (dataModel != m_dataModel)
        {
            DetachDataModel();

            AttachDataModel(dataModel);
        }
    }

    void QTreeViewStateSaver::Detach()
    {
        CaptureSnapshot();

        bool success = true;

        success = success && DetachTreeView();

        success = success && DetachSelectionModel();

        success = success && DetachDataModel();

        Q_ASSERT(success);
    }

    bool QTreeViewStateSaver::AttachTreeView(QTreeView* attach)
    {
        m_attachedTree = attach;

        bool success = true;

        success = success && connect(m_attachedTree, &QTreeView::expanded, this, &QTreeViewStateSaver::RowExpanded);
        success = success && connect(m_attachedTree, &QTreeView::collapsed, this, &QTreeViewStateSaver::RowCollapsed);

        if (m_attachedTree->horizontalScrollBar())
        {
            success = success && connect(m_attachedTree->horizontalScrollBar(), &QScrollBar::valueChanged, this, &QTreeViewStateSaver::HorizontalScrollChanged);
        }

        if (m_attachedTree->verticalScrollBar())
        {
            success = success && connect(m_attachedTree->verticalScrollBar(), &QScrollBar::valueChanged, this, &QTreeViewStateSaver::VerticalScrollChanged);
        }

        return success;
    }

    bool QTreeViewStateSaver::DetachTreeView()
    {
        bool success = true;

        if (m_attachedTree)
        {
            success = success && disconnect(m_attachedTree, &QTreeView::expanded, this, &QTreeViewStateSaver::RowExpanded);
            success = success && disconnect(m_attachedTree, &QTreeView::collapsed, this, &QTreeViewStateSaver::RowCollapsed);

            if (m_attachedTree->horizontalScrollBar())
            {
                success = success && disconnect(m_attachedTree->horizontalScrollBar(), &QScrollBar::valueChanged, this, &QTreeViewStateSaver::HorizontalScrollChanged);
            }

            if (m_attachedTree->verticalScrollBar())
            {
                success = success && disconnect(m_attachedTree->verticalScrollBar(), &QScrollBar::valueChanged, this, &QTreeViewStateSaver::VerticalScrollChanged);
            }

            m_attachedTree = nullptr;
        }

        return success;
    }

    bool QTreeViewStateSaver::AttachDataModel(QAbstractItemModel* dataModel)
    {
        bool success = true;

        if (dataModel)
        {
            success = success && connect(dataModel, &QAbstractItemModel::modelReset, this, &QTreeViewStateSaver::ModelReset);
        }

        m_dataModel = dataModel;

        return success;
    }

    bool QTreeViewStateSaver::DetachDataModel()
    {
        bool success = true;

        if (m_dataModel)
        {
            success = success && disconnect(m_dataModel, &QAbstractItemModel::modelReset, this, &QTreeViewStateSaver::ModelReset);

            m_dataModel = nullptr;
        }

        return success;
    }

    bool QTreeViewStateSaver::AttachSelectionModel(QItemSelectionModel* selectionModel)
    {
        bool success = true;

        if (selectionModel)
        {
            success = success && connect(selectionModel, &QItemSelectionModel::selectionChanged, this, &QTreeViewStateSaver::SelectionChanged);
            success = success && connect(selectionModel, &QItemSelectionModel::currentChanged, this, &QTreeViewStateSaver::CurrentChanged);
        }

        m_selectionModel = selectionModel;

        return success;
    }

    bool QTreeViewStateSaver::DetachSelectionModel()
    {
        bool success = true;

        if (m_selectionModel)
        {
            success = success && disconnect(m_selectionModel, &QItemSelectionModel::selectionChanged, this, &QTreeViewStateSaver::SelectionChanged);
            success = success && disconnect(m_selectionModel, &QItemSelectionModel::currentChanged, this, &QTreeViewStateSaver::CurrentChanged);

            m_selectionModel = nullptr;
        }

        return true;
    }


    QTreeViewStateSaver::~QTreeViewStateSaver()
    {
        // We do this on close, if we can, to cover the case of the expandAll signal not getting emitted.
        // Has to be guarded, because the tree view's model is deleted but the pointer is not reset to nullptr
        CaptureSnapshot();
    }

    void QTreeViewStateSaver::WriteStateTo(QSet<QString>& target)
    {
        target.clear();
        for (auto& str : m_data->m_expandedElements)
        {
            target.insert(str.c_str());
        }
    }

    void QTreeViewStateSaver::ReadStateFrom(QSet<QString>& source)
    {
        m_data->m_expandedElements.clear();
        for (auto& qstr : source)
        {
            m_data->m_expandedElements.push_back(qstr.toUtf8().data());
        }
        ApplySnapshot();
    }

    void QTreeViewStateSaver::CaptureSnapshot() const
    {
        if (m_attachedTree && m_dataModel)
        {
            m_data->CaptureSnapshot(m_attachedTree);
        }
    }

    void QTreeViewStateSaver::ApplySnapshot() const
    {
        if (m_attachedTree)
        {
            m_data->ApplySnapshot(m_attachedTree, m_expandedFunction);
        }
    }

    void QTreeViewStateSaver::SetExpandedFunction(AZStd::function<bool(const QModelIndex&)> expandedFunction)
    {
        m_expandedFunction = expandedFunction;
    }

    void QTreeViewStateSaver::Reflect(AZ::ReflectContext* context)
    {
        QTreeViewStateSaverData::Reflect(context);
    }

    void QTreeViewStateSaver::VerticalScrollChanged(int newValue)
    {
        m_data->_q_VerticalScrollChanged(newValue);
    }

    void QTreeViewStateSaver::HorizontalScrollChanged(int newValue)
    {
        m_data->_q_HorizontalScrollChanged(newValue);
    }

    void QTreeViewStateSaver::ModelReset()
    {
        m_data->_q_ModelReset();
    }

    void QTreeViewStateSaver::CurrentChanged(const QModelIndex& current, const QModelIndex& newCurrent)
    {
        m_data->_q_CurrentChanged(current, newCurrent);
    }

    void QTreeViewStateSaver::SelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        m_data->_q_selectionChanged(selected, deselected);
    }

    void QTreeViewStateSaver::RowExpanded(const QModelIndex& modelIndex)
    {
        m_data->_q_RowExpanded(modelIndex);
    }

    void QTreeViewStateSaver::RowCollapsed(const QModelIndex& modelIndex)
    {
        m_data->_q_RowCollapsed(modelIndex);
    }





    QTreeViewWithStateSaving::QTreeViewWithStateSaving(QWidget* parent)
        : QTreeView(parent)
    {
    }

    QTreeViewWithStateSaving::~QTreeViewWithStateSaving()
    {
        if (m_treeStateSaver)
        {
            // called because Qt doesn't emit a signal on expandAll
            m_treeStateSaver->CaptureSnapshot();
        }
    }

    void QTreeViewWithStateSaving::InitializeTreeViewSaving(AZ::u32 storageID)
    {
        SetupSaver(new QTreeViewStateSaver(storageID, this));
    }

    void QTreeViewWithStateSaving::setModel(QAbstractItemModel* newModel)
    {
        QTreeView::setModel(newModel);

        if (m_treeStateSaver)
        {
            // reattach, so that the new model can be connected to by the state saver
            m_treeStateSaver->Attach(this, model(), selectionModel());
        }
    }

    void QTreeViewWithStateSaving::setSelectionModel(QItemSelectionModel* newSelectionModel)
    {
        QTreeView::setSelectionModel(newSelectionModel);

        if (m_treeStateSaver)
        {
            // reattach, so that the new selection model can be connected to by the state saver
            m_treeStateSaver->Attach(this, model(), selectionModel());
        }
    }

    void QTreeViewWithStateSaving::SetupSaver(QTreeViewStateSaver* stateSaver)
    {
        Q_ASSERT(!m_treeStateSaver); // can't call InitializeSaving twice!

        m_treeStateSaver = stateSaver;
        m_treeStateSaver->Attach(this, model(), selectionModel());
    }

    void QTreeViewWithStateSaving::CaptureTreeViewSnapshot() const
    {
        Q_ASSERT(m_treeStateSaver);

        m_treeStateSaver->CaptureSnapshot();
    }

    void QTreeViewWithStateSaving::ApplyTreeViewSnapshot() const
    {
        Q_ASSERT(m_treeStateSaver);

        m_treeStateSaver->ApplySnapshot();
    }

    void QTreeViewWithStateSaving::WriteTreeViewStateTo(QSet<QString>& target)
    {
        Q_ASSERT(m_treeStateSaver);

        m_treeStateSaver->WriteStateTo(target);
    }

    void QTreeViewWithStateSaving::ReadTreeViewStateFrom(QSet<QString>& source)
    {
        Q_ASSERT(m_treeStateSaver);

        m_treeStateSaver->ReadStateFrom(source);
    }

    void QTreeViewWithStateSaving::SetTreeViewExpandedFunction(AZStd::function<bool(const QModelIndex&)> expandedFunction)
    {
        Q_ASSERT(m_treeStateSaver);

        m_treeStateSaver->SetExpandedFunction(expandedFunction);
    }

    void QTreeViewWithStateSaving::PauseTreeViewSaving()
    {
        Q_ASSERT(m_treeStateSaver);
        
        m_treeStateSaver->Detach();
    }

    void QTreeViewWithStateSaving::UnpauseTreeViewSaving()
    {
        Q_ASSERT(m_treeStateSaver);

        m_treeStateSaver->Attach(this, model(), selectionModel());
    }
}

#include <UI/UICore/QTreeViewStateSaver.moc>
