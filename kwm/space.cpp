#include "space.h"
#include "display.h"
#include "window.h"
#include "tree.h"
#include "border.h"

extern kwm_screen KWMScreen;
extern kwm_focus KWMFocus;
extern kwm_toggles KWMToggles;
extern kwm_mode KWMMode;
extern kwm_border FocusedBorder;
extern kwm_border MarkedBorder;

void GetTagForMonocleSpace(space_info *Space, std::string &Tag)
{
    tree_node *Node = Space->RootNode;
    bool FoundFocusedWindow = false;
    int FocusedIndex = 0;
    int NumberOfWindows = 0;

    if(Node && KWMFocus.Window)
    {
        FocusedIndex = 1;
        NumberOfWindows = 1;

        if(Node->WindowID == KWMFocus.Window->WID)
            FoundFocusedWindow = true;

        while(Node->RightChild)
        {
            if(Node->WindowID == KWMFocus.Window->WID)
                FoundFocusedWindow = true;

            if(!FoundFocusedWindow)
                ++FocusedIndex;

            ++NumberOfWindows;

            Node = Node->RightChild;
        }

        if(Node->WindowID == KWMFocus.Window->WID)
            FoundFocusedWindow = true;
    }

    if(FoundFocusedWindow)
        Tag = "[" + std::to_string(FocusedIndex) + "/" + std::to_string(NumberOfWindows) + "]";
    else
        Tag = "[" + std::to_string(NumberOfWindows) + "]";
}

void GetTagForCurrentSpace(std::string &Tag)
{
    if(IsSpaceInitializedForScreen(KWMScreen.Current))
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        if(Space->Mode == SpaceModeBSP)
            Tag = "[bsp]";
        else if(Space->Mode == SpaceModeFloating)
            Tag = "[float]";
        else if(Space->Mode == SpaceModeMonocle)
            GetTagForMonocleSpace(Space, Tag);
    }
    else
    {
        if(KWMMode.Space == SpaceModeBSP)
            Tag = "[bsp]";
        else if(KWMMode.Space == SpaceModeFloating)
            Tag = "[float]";
        else if(KWMMode.Space == SpaceModeMonocle)
            Tag = "[monocle]";
    }
}

bool IsActiveSpaceFloating()
{
    return IsSpaceFloating(KWMScreen.Current->ActiveSpace);
}

bool IsSpaceFloating(int SpaceID)
{
    bool Result = false;

    if(IsSpaceInitializedForScreen(KWMScreen.Current))
    {
        std::map<int, space_info>::iterator It = KWMScreen.Current->Space.find(SpaceID);
        if(It != KWMScreen.Current->Space.end())
            Result = KWMScreen.Current->Space[SpaceID].Mode == SpaceModeFloating;
    }

    return Result;
}

space_info *GetActiveSpaceOfScreen(screen_info *Screen)
{
    space_info *Space = NULL;
    std::map<int, space_info>::iterator It = Screen->Space.find(Screen->ActiveSpace);

    if(It == Screen->Space.end())
    {
        space_info Clear = {{0}};
        Screen->Space[Screen->ActiveSpace] = Clear;
        Space = &Screen->Space[Screen->ActiveSpace];
    }
    else
    {
        Space = &It->second;
    }

    return Space;
}

bool IsSpaceInitializedForScreen(screen_info *Screen)
{
    if(!Screen)
        return false;

    std::map<int, space_info>::iterator It = Screen->Space.find(Screen->ActiveSpace);
    if(It == Screen->Space.end())
        return false;
    else
        return It->second.Initialized;
}

bool DoesSpaceExistInMapOfScreen(screen_info *Screen)
{
    if(!Screen)
        return false;

    std::map<int, space_info>::iterator It = Screen->Space.find(Screen->ActiveSpace);
    if(It == Screen->Space.end())
        return false;
    else
        return It->second.RootNode != NULL && It->second.Initialized;
}

bool IsSpaceTransitionInProgress()
{
    if(KWMScreen.Transitioning)
        return true;

    Assert(KWMScreen.Current, "IsSpaceTransitionInProgress()")

    CFStringRef Identifier;
    if(KWMScreen.Current->Identifier)
        Identifier = KWMScreen.Current->Identifier;
    else
        Identifier = CGSCopyManagedDisplayForSpace(CGSDefaultConnection, KWMScreen.Current->ActiveSpace);

    bool Result = CGSManagedDisplayIsAnimating(CGSDefaultConnection, Identifier);
    if(Result)
    {
        DEBUG("IsSpaceTransitionInProgress() Space transition detected")
        KWMScreen.Transitioning = true;
        ClearFocusedWindow();
        ClearMarkedWindow();
    }

    return Result;
}

bool IsSpaceSystemOrFullscreen()
{
    bool Result = false;

    if(IsSpaceInitializedForScreen(KWMScreen.Current))
        Result = CGSSpaceGetType(CGSDefaultConnection, KWMScreen.Current->ActiveSpace) != CGSSpaceTypeUser;

    return Result;
}

void FloatFocusedSpace()
{
    if(IsSpaceInitializedForScreen(KWMScreen.Current) &&
       KWMToggles.EnableTilingMode &&
       !IsSpaceTransitionInProgress() &&
       !IsSpaceSystemOrFullscreen() &&
       FilterWindowList(KWMScreen.Current))
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        DestroyNodeTree(Space->RootNode, Space->Mode);
        Space->RootNode = NULL;
        Space->Mode = SpaceModeFloating;
        ClearFocusedWindow();
    }
}

void TileFocusedSpace(space_tiling_option Mode)
{
    if(IsSpaceInitializedForScreen(KWMScreen.Current) &&
       KWMToggles.EnableTilingMode &&
       !IsSpaceTransitionInProgress() &&
       !IsSpaceSystemOrFullscreen() &&
       FilterWindowList(KWMScreen.Current))
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        if(Space->Mode == Mode)
            return;

        DestroyNodeTree(Space->RootNode, Space->Mode);
        Space->RootNode = NULL;

        Space->Mode = Mode;
        std::vector<window_info*> WindowsOnDisplay = GetAllWindowsOnDisplay(KWMScreen.Current->ID);
        CreateWindowNodeTree(KWMScreen.Current, &WindowsOnDisplay);
    }
}

void ToggleFocusedSpaceFloating()
{
    if(IsSpaceInitializedForScreen(KWMScreen.Current))
    {
        if(!IsSpaceFloating(KWMScreen.Current->ActiveSpace))
            FloatFocusedSpace();
        else
            TileFocusedSpace(SpaceModeBSP);
    }
}
