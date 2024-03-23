//------------------------------------------------------------------------------
// VERSION 0.9.1
//
// LICENSE
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
//
// CREDITS
//   Written by Michal Cichon
//------------------------------------------------------------------------------
# include "imgui_node_editor_internal.h"
# include <algorithm>
#include <unordered_map>
#include <shared_mutex>

//------------------------------------------------------------------------------
thread_local ax::NodeEditor::Detail::EditorContext* t_Editor = nullptr;

std::unordered_map<ImGuiID, ax::NodeEditor::Detail::EditorContext*> s_Editors;
static std::shared_mutex s_Mutex;

ax::NodeEditor::Detail::EditorContext* GetOrCreateContext(const char* id, const ax::NodeEditor::Config* config = nullptr)
{
    auto imid = ImGui::GetID(id);
    {
        std::shared_lock lock(s_Mutex);

        auto itr = s_Editors.find(imid);

        if (itr != s_Editors.end())
            return itr->second;
    }

    {
		std::unique_lock lock(s_Mutex);


		auto editor = new ax::NodeEditor::Detail::EditorContext(config);
		s_Editors[imid] = editor;

		return editor;
	}
}

//------------------------------------------------------------------------------
template <typename C, typename I, typename F>
static int BuildIdList(C& container, I* list, int listSize, F&& accept)
{
    if (list != nullptr)
    {
        int count = 0;
        for (auto object : container)
        {
            if (listSize <= 0)
                break;

            if (accept(object))
            {
                list[count] = I(object->ID().AsPointer());
                ++count;
                --listSize;}
        }

        return count;
    }
    else
        return static_cast<int>(std::count_if(container.begin(), container.end(), accept));
}


//------------------------------------------------------------------------------
ax::NodeEditor::EditorContext* ax::NodeEditor::CreateEditor(const char* id, const Config* config)
{
    return reinterpret_cast<ax::NodeEditor::EditorContext*>(GetOrCreateContext(id, config));
}

void ax::NodeEditor::DestroyEditor(const char* id)
{
    {
        std::unique_lock lock(s_Mutex);

        auto itr = s_Editors.find(ImGui::GetID(id));

        if (itr != s_Editors.end())
        {
            delete itr->second;
            s_Editors.erase(itr);
        }
    }
}

const ax::NodeEditor::Config& ax::NodeEditor::GetConfig(EditorContext* ctx)
{
    if (ctx)
    {
        auto editor = reinterpret_cast<ax::NodeEditor::Detail::EditorContext*>(ctx);

        return editor->GetConfig();
    }
    else
    {
        static Config s_EmptyConfig;
        return s_EmptyConfig;
    }
}

ax::NodeEditor::Style& ax::NodeEditor::GetStyle()
{
    return t_Editor->GetStyle();
}

const char* ax::NodeEditor::GetStyleColorName(StyleColor colorIndex)
{
    return t_Editor->GetStyle().GetColorName(colorIndex);
}

void ax::NodeEditor::PushStyleColor(StyleColor colorIndex, const ImVec4& color)
{
    t_Editor->GetStyle().PushColor(colorIndex, color);
}

void ax::NodeEditor::PopStyleColor(int count)
{
    t_Editor->GetStyle().PopColor(count);
}

void ax::NodeEditor::PushStyleVar(StyleVar varIndex, float value)
{
    t_Editor->GetStyle().PushVar(varIndex, value);
}

void ax::NodeEditor::PushStyleVar(StyleVar varIndex, const ImVec2& value)
{
    t_Editor->GetStyle().PushVar(varIndex, value);
}

void ax::NodeEditor::PushStyleVar(StyleVar varIndex, const ImVec4& value)
{
    t_Editor->GetStyle().PushVar(varIndex, value);
}

void ax::NodeEditor::PopStyleVar(int count)
{
    t_Editor->GetStyle().PopVar(count);
}

void ax::NodeEditor::Begin(const char* id, const ImVec2& size, EditorFlags Flags)
{
    t_Editor = GetOrCreateContext(id);
    t_Editor->Begin(id, size, Flags);
}

void ax::NodeEditor::End()
{
    t_Editor->End();
}

void ax::NodeEditor::BeginNode(NodeId id)
{
    t_Editor->GetNodeBuilder().Begin(id);
}

void ax::NodeEditor::BeginPin(PinId id, PinKind kind)
{
    t_Editor->GetNodeBuilder().BeginPin(id, kind);
}

void ax::NodeEditor::PinRect(const ImVec2& a, const ImVec2& b)
{
    t_Editor->GetNodeBuilder().PinRect(a, b);
}

void ax::NodeEditor::PinPivotRect(const ImVec2& a, const ImVec2& b)
{
    t_Editor->GetNodeBuilder().PinPivotRect(a, b);
}

void ax::NodeEditor::PinPivotSize(const ImVec2& size)
{
    t_Editor->GetNodeBuilder().PinPivotSize(size);
}

void ax::NodeEditor::PinPivotScale(const ImVec2& scale)
{
    t_Editor->GetNodeBuilder().PinPivotScale(scale);
}

void ax::NodeEditor::PinPivotAlignment(const ImVec2& alignment)
{
    t_Editor->GetNodeBuilder().PinPivotAlignment(alignment);
}

void ax::NodeEditor::EndPin()
{
    t_Editor->GetNodeBuilder().EndPin();
}

void ax::NodeEditor::Group(const ImVec2& size)
{
    t_Editor->GetNodeBuilder().Group(size);
}

void ax::NodeEditor::EndNode()
{
    t_Editor->GetNodeBuilder().End();
}

bool ax::NodeEditor::BeginGroupHint(NodeId nodeId)
{
    return t_Editor->GetHintBuilder().Begin(nodeId);
}

ImVec2 ax::NodeEditor::GetGroupMin()
{
    return t_Editor->GetHintBuilder().GetGroupMin();
}

ImVec2 ax::NodeEditor::GetGroupMax()
{
    return t_Editor->GetHintBuilder().GetGroupMax();
}

ImDrawList* ax::NodeEditor::GetHintForegroundDrawList()
{
    return t_Editor->GetHintBuilder().GetForegroundDrawList();
}

ImDrawList* ax::NodeEditor::GetHintBackgroundDrawList()
{
    return t_Editor->GetHintBuilder().GetBackgroundDrawList();
}

void ax::NodeEditor::EndGroupHint()
{
    t_Editor->GetHintBuilder().End();
}

ImDrawList* ax::NodeEditor::GetNodeBackgroundDrawList(NodeId nodeId)
{
    if (auto node = t_Editor->FindNode(nodeId))
        return t_Editor->GetNodeBuilder().GetUserBackgroundDrawList(node);
    else
        return nullptr;
}

bool ax::NodeEditor::Link(LinkId id, PinId startPinId, PinId endPinId, const ImVec4& color/* = ImVec4(1, 1, 1, 1)*/, float thickness/* = 1.0f*/)
{
    return t_Editor->DoLink(id, startPinId, endPinId, ImColor(color), thickness);
}

void ax::NodeEditor::Flow(LinkId linkId, FlowDirection direction)
{
    if (auto link = t_Editor->FindLink(linkId))
        t_Editor->Flow(link, direction);
}

bool ax::NodeEditor::BeginCreate(const ImVec4& color, float thickness)
{
    auto& context = t_Editor->GetItemCreator();

    if (context.Begin())
    {
        context.SetStyle(ImColor(color), thickness);
        return true;
    }
    else
        return false;
}

bool ax::NodeEditor::QueryNewLink(PinId* startId, PinId* endId)
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = t_Editor->GetItemCreator();

    return context.QueryLink(startId, endId) == Result::True;
}

bool ax::NodeEditor::QueryNewLink(PinId* startId, PinId* endId, const ImVec4& color, float thickness)
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = t_Editor->GetItemCreator();

    auto result = context.QueryLink(startId, endId);
    if (result != Result::Indeterminate)
        context.SetStyle(ImColor(color), thickness);

    return result == Result::True;
}

bool ax::NodeEditor::QueryNewNode(PinId* pinId)
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = t_Editor->GetItemCreator();

    return context.QueryNode(pinId) == Result::True;
}

bool ax::NodeEditor::QueryNewNode(PinId* pinId, const ImVec4& color, float thickness)
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = t_Editor->GetItemCreator();

    auto result = context.QueryNode(pinId);
    if (result != Result::Indeterminate)
        context.SetStyle(ImColor(color), thickness);

    return result == Result::True;
}

bool ax::NodeEditor::AcceptNewItem()
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = t_Editor->GetItemCreator();

    return context.AcceptItem() == Result::True;
}

bool ax::NodeEditor::AcceptNewItem(const ImVec4& color, float thickness)
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = t_Editor->GetItemCreator();

    auto result = context.AcceptItem();
    if (result != Result::Indeterminate)
        context.SetStyle(ImColor(color), thickness);

    return result == Result::True;
}

void ax::NodeEditor::RejectNewItem()
{
    auto& context = t_Editor->GetItemCreator();

    context.RejectItem();
}

void ax::NodeEditor::RejectNewItem(const ImVec4& color, float thickness)
{
    using Result = ax::NodeEditor::Detail::CreateItemAction::Result;

    auto& context = t_Editor->GetItemCreator();

    if (context.RejectItem() != Result::Indeterminate)
        context.SetStyle(ImColor(color), thickness);
}

void ax::NodeEditor::EndCreate()
{
    auto& context = t_Editor->GetItemCreator();

    context.End();
}

bool ax::NodeEditor::BeginDelete()
{
    auto& context = t_Editor->GetItemDeleter();

    return context.Begin();
}

bool ax::NodeEditor::QueryDeletedLink(LinkId* linkId, PinId* startId, PinId* endId)
{
    auto& context = t_Editor->GetItemDeleter();

    return context.QueryLink(linkId, startId, endId);
}

bool ax::NodeEditor::QueryDeletedNode(NodeId* nodeId)
{
    auto& context = t_Editor->GetItemDeleter();

    return context.QueryNode(nodeId);
}

bool ax::NodeEditor::AcceptDeletedItem(bool deleteDependencies)
{
    auto& context = t_Editor->GetItemDeleter();

    return context.AcceptItem(deleteDependencies);
}

void ax::NodeEditor::RejectDeletedItem()
{
    auto& context = t_Editor->GetItemDeleter();

    context.RejectItem();
}

void ax::NodeEditor::EndDelete()
{
    auto& context = t_Editor->GetItemDeleter();

    context.End();
}

void ax::NodeEditor::SetNodePosition(NodeId nodeId, const ImVec2& position)
{
    t_Editor->SetNodePosition(nodeId, position);
}

void ax::NodeEditor::SetGroupSize(NodeId nodeId, const ImVec2& size)
{
    t_Editor->SetGroupSize(nodeId, size);
}

ImVec2 ax::NodeEditor::GetNodePosition(NodeId nodeId)
{
    return t_Editor->GetNodePosition(nodeId);
}

ImVec2 ax::NodeEditor::GetNodeSize(NodeId nodeId)
{
    return t_Editor->GetNodeSize(nodeId);
}

void ax::NodeEditor::CenterNodeOnScreen(NodeId nodeId)
{
    if (auto node = t_Editor->FindNode(nodeId))
        node->CenterOnScreenInNextFrame();
}

void ax::NodeEditor::SetNodeZPosition(NodeId nodeId, float z)
{
    t_Editor->SetNodeZPosition(nodeId, z);
}

float ax::NodeEditor::GetNodeZPosition(NodeId nodeId)
{
    return t_Editor->GetNodeZPosition(nodeId);
}

void ax::NodeEditor::RestoreNodeState(NodeId nodeId)
{
    if (auto node = t_Editor->FindNode(nodeId))
        t_Editor->MarkNodeToRestoreState(node);
}

void ax::NodeEditor::Suspend()
{
    t_Editor->Suspend();
}

void ax::NodeEditor::Resume()
{
    t_Editor->Resume();
}

bool ax::NodeEditor::IsSuspended()
{
    return t_Editor->IsSuspended();
}

bool ax::NodeEditor::IsActive()
{
    return t_Editor->IsFocused();
}

bool ax::NodeEditor::HasSelectionChanged()
{
    return t_Editor->HasSelectionChanged();
}

int ax::NodeEditor::GetSelectedObjectCount()
{
    return (int)t_Editor->GetSelectedObjects().size();
}

int ax::NodeEditor::GetSelectedNodes(NodeId* nodes, int size)
{
    return BuildIdList(t_Editor->GetSelectedObjects(), nodes, size, [](auto object)
    {
        return object->AsNode() != nullptr;
    });
}

int ax::NodeEditor::GetSelectedLinks(LinkId* links, int size)
{
    return BuildIdList(t_Editor->GetSelectedObjects(), links, size, [](auto object)
    {
        return object->AsLink() != nullptr;
    });
}

bool ax::NodeEditor::IsNodeSelected(NodeId nodeId)
{
    if (auto node = t_Editor->FindNode(nodeId))
        return t_Editor->IsSelected(node);
    else
        return false;
}

bool ax::NodeEditor::IsLinkSelected(LinkId linkId)
{
    if (auto link = t_Editor->FindLink(linkId))
        return t_Editor->IsSelected(link);
    else
        return false;
}

void ax::NodeEditor::ClearSelection()
{
    t_Editor->ClearSelection();
}

void ax::NodeEditor::SelectNode(NodeId nodeId, bool append)
{
    if (auto node = t_Editor->FindNode(nodeId))
    {
        if (append)
            t_Editor->SelectObject(node);
        else
            t_Editor->SetSelectedObject(node);
    }
}

void ax::NodeEditor::SelectLink(LinkId linkId, bool append)
{
    if (auto link = t_Editor->FindLink(linkId))
    {
        if (append)
            t_Editor->SelectObject(link);
        else
            t_Editor->SetSelectedObject(link);
    }
}

void ax::NodeEditor::DeselectNode(NodeId nodeId)
{
    if (auto node = t_Editor->FindNode(nodeId))
        t_Editor->DeselectObject(node);
}

void ax::NodeEditor::DeselectLink(LinkId linkId)
{
    if (auto link = t_Editor->FindLink(linkId))
        t_Editor->DeselectObject(link);
}

bool ax::NodeEditor::DeleteNode(NodeId nodeId)
{
    if (auto node = t_Editor->FindNode(nodeId))
        return t_Editor->GetItemDeleter().Add(node);
    else
        return false;
}

bool ax::NodeEditor::DeleteLink(LinkId linkId)
{
    if (auto link = t_Editor->FindLink(linkId))
        return t_Editor->GetItemDeleter().Add(link);
    else
        return false;
}

bool ax::NodeEditor::HasAnyLinks(NodeId nodeId)
{
    return t_Editor->HasAnyLinks(nodeId);
}

bool ax::NodeEditor::HasAnyLinks(PinId pinId)
{
    return t_Editor->HasAnyLinks(pinId);
}

int ax::NodeEditor::BreakLinks(NodeId nodeId)
{
    return t_Editor->BreakLinks(nodeId);
}

int ax::NodeEditor::BreakLinks(PinId pinId)
{
    return t_Editor->BreakLinks(pinId);
}

void ax::NodeEditor::NavigateToContent(float duration)
{
    t_Editor->NavigateTo(t_Editor->GetContentBounds(), true, duration);
}

void ax::NodeEditor::NavigateToSelection(bool zoomIn, float duration)
{
    t_Editor->NavigateTo(t_Editor->GetSelectionBounds(), zoomIn, duration);
}

bool ax::NodeEditor::ShowNodeContextMenu(NodeId* nodeId)
{
    return t_Editor->GetContextMenu().ShowNodeContextMenu(nodeId);
}

bool ax::NodeEditor::ShowPinContextMenu(PinId* pinId)
{
    return t_Editor->GetContextMenu().ShowPinContextMenu(pinId);
}

bool ax::NodeEditor::ShowLinkContextMenu(LinkId* linkId)
{
    return t_Editor->GetContextMenu().ShowLinkContextMenu(linkId);
}

bool ax::NodeEditor::ShowBackgroundContextMenu()
{
    return t_Editor->GetContextMenu().ShowBackgroundContextMenu();
}

void ax::NodeEditor::EnableShortcuts(bool enable)
{
    t_Editor->EnableShortcuts(enable);
}

bool ax::NodeEditor::AreShortcutsEnabled()
{
    return t_Editor->AreShortcutsEnabled();
}

bool ax::NodeEditor::BeginShortcut()
{
    return t_Editor->GetShortcut().Begin();
}

bool ax::NodeEditor::AcceptCut()
{
    return t_Editor->GetShortcut().AcceptCut();
}

bool ax::NodeEditor::AcceptCopy()
{
    return t_Editor->GetShortcut().AcceptCopy();
}

bool ax::NodeEditor::AcceptPaste()
{
    return t_Editor->GetShortcut().AcceptPaste();
}

bool ax::NodeEditor::AcceptDuplicate()
{
    return t_Editor->GetShortcut().AcceptDuplicate();
}

bool ax::NodeEditor::AcceptCreateNode()
{
    return t_Editor->GetShortcut().AcceptCreateNode();
}

int ax::NodeEditor::GetActionContextSize()
{
    return static_cast<int>(t_Editor->GetShortcut().m_Context.size());
}

int ax::NodeEditor::GetActionContextNodes(NodeId* nodes, int size)
{
    return BuildIdList(t_Editor->GetSelectedObjects(), nodes, size, [](auto object)
    {
        return object->AsNode() != nullptr;
    });
}

int ax::NodeEditor::GetActionContextLinks(LinkId* links, int size)
{
    return BuildIdList(t_Editor->GetSelectedObjects(), links, size, [](auto object)
    {
        return object->AsLink() != nullptr;
    });
}

void ax::NodeEditor::EndShortcut()
{
    return t_Editor->GetShortcut().End();
}

float ax::NodeEditor::GetCurrentZoom()
{
    return t_Editor->GetView().InvScale;
}

ax::NodeEditor::NodeId ax::NodeEditor::GetHoveredNode()
{
    return t_Editor->GetHoveredNode();
}

ax::NodeEditor::PinId ax::NodeEditor::GetHoveredPin()
{
    return t_Editor->GetHoveredPin();
}

ax::NodeEditor::LinkId ax::NodeEditor::GetHoveredLink()
{
    return t_Editor->GetHoveredLink();
}

ax::NodeEditor::NodeId ax::NodeEditor::GetDoubleClickedNode()
{
    return t_Editor->GetDoubleClickedNode();
}

ax::NodeEditor::PinId ax::NodeEditor::GetDoubleClickedPin()
{
    return t_Editor->GetDoubleClickedPin();
}

ax::NodeEditor::LinkId ax::NodeEditor::GetDoubleClickedLink()
{
    return t_Editor->GetDoubleClickedLink();
}

bool ax::NodeEditor::IsBackgroundClicked()
{
    return t_Editor->IsBackgroundClicked();
}

bool ax::NodeEditor::IsBackgroundDoubleClicked()
{
    return t_Editor->IsBackgroundDoubleClicked();
}

ImGuiMouseButton ax::NodeEditor::GetBackgroundClickButtonIndex()
{
    return t_Editor->GetBackgroundClickButtonIndex();
}

ImGuiMouseButton ax::NodeEditor::GetBackgroundDoubleClickButtonIndex()
{
    return t_Editor->GetBackgroundDoubleClickButtonIndex();
}

bool ax::NodeEditor::GetLinkPins(LinkId linkId, PinId* startPinId, PinId* endPinId)
{
    auto link = t_Editor->FindLink(linkId);
    if (!link)
        return false;

    if (startPinId)
        *startPinId = link->m_StartPin->m_ID;
    if (endPinId)
        *endPinId = link->m_EndPin->m_ID;

    return true;
}

bool ax::NodeEditor::PinHadAnyLinks(PinId pinId)
{
    return t_Editor->PinHadAnyLinks(pinId);
}

ImVec2 ax::NodeEditor::GetScreenSize()
{
    return t_Editor->GetRect().GetSize();
}

ImVec2 ax::NodeEditor::ScreenToCanvas(const ImVec2& pos)
{
    return t_Editor->ToCanvas(pos);
}

ImVec2 ax::NodeEditor::CanvasToScreen(const ImVec2& pos)
{
    return t_Editor->ToScreen(pos);
}

int ax::NodeEditor::GetNodeCount()
{
    return t_Editor->CountLiveNodes();
}

int ax::NodeEditor::GetOrderedNodeIds(NodeId* nodes, int size)
{
    return t_Editor->GetNodeIds(nodes, size);
}

void ax::NodeEditor::Layout(LayoutFlags Flags)
{
    return t_Editor->Layout(Flags);
}