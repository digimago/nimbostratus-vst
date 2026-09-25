// Hands unused key presses back to the host on macOS.
//
// pugl makes its wrapper NSView the first responder the moment an embedded
// plugin view is realized, and its -keyDown:/-keyUp: never forward anything up
// the responder chain. Inside a DAW that means an open plugin window eats the
// host's key commands - most painfully the spacebar, which normally starts and
// stops transport in Ableton Live.
//
// The UI reports once per frame whether Dear ImGui is taking typed input (the
// numeric entry on the Pitch and gain trim knobs). On the first
// report the view's isa is swapped for a runtime-generated subclass whose key
// handlers consult that flag: while text input is active the original pugl
// handlers run, otherwise the event goes to the next responder, which is the
// host's own view.
//
// The subclass is generated rather than swizzled into pugl's class so that
// other DPF plugins loaded in the same host process are left untouched - their
// wrapper class carries the same name, because DPF derives it from the
// toolchain version rather than from the plugin.

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include "NimbostratusKeyboard.h"

namespace {

// Per-view state, kept as associated objects: the views already exist by the
// time the class is swapped in, so ivars are not an option.
char kCaptureKey;
char kHeldKeysKey;

Class sBaseClass        = Nil;
Class sPassthroughClass = Nil;
IMP   sOriginalKeyDown  = nullptr;
IMP   sOriginalKeyUp    = nullptr;

bool wantsCapture(id self)
{
    NSNumber* const flag = objc_getAssociatedObject(self, &kCaptureKey);
    return flag != nil && [flag boolValue];
}

// Key codes whose press was handed to pugl. A key's release has to follow its
// press, and the two are separate events with the capture flag free to change
// in between - which is exactly what happens on Return and Escape, the keys
// that end text entry: the press is captured, committing the value turns
// capture off, and the release would otherwise go to the host. Dear ImGui
// would never see it and would hold the key down forever, its auto-repeat
// re-firing inside every numeric field opened afterwards.
NSMutableSet* heldKeys(id self, const bool create)
{
    NSMutableSet* set = objc_getAssociatedObject(self, &kHeldKeysKey);

    if (set == nil && create)
    {
        set = [NSMutableSet set];
        objc_setAssociatedObject(self, &kHeldKeysKey, set,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }

    return set;
}

void keyDownIMP(id self, SEL cmd, NSEvent* event)
{
    if (! wantsCapture(self))
    {
        [[(NSView*)self nextResponder] keyDown:event];
        return;
    }

    if (! [event isARepeat])
        [heldKeys(self, true) addObject:@([event keyCode])];

    ((void (*)(id, SEL, NSEvent*))sOriginalKeyDown)(self, cmd, event);
}

void keyUpIMP(id self, SEL cmd, NSEvent* event)
{
    // Follow the press, not the flag as it stands now.
    NSNumber* const code = @([event keyCode]);
    NSMutableSet* const held = heldKeys(self, false);

    if (held != nil && [held containsObject:code])
    {
        [held removeObject:code];
        ((void (*)(id, SEL, NSEvent*))sOriginalKeyUp)(self, cmd, event);
        return;
    }

    [[(NSView*)self nextResponder] keyUp:event];
}

// Builds the subclass on first use and swaps the view over to it.
void ensurePatched(NSView* const view)
{
    Class const current = object_getClass(view);

    if (current == sPassthroughClass)
        return;

    if (sPassthroughClass == Nil)
    {
        Method const down = class_getInstanceMethod(current, @selector(keyDown:));
        Method const up   = class_getInstanceMethod(current, @selector(keyUp:));
        if (down == nullptr || up == nullptr)
            return;

        sOriginalKeyDown = method_getImplementation(down);
        sOriginalKeyUp   = method_getImplementation(up);

        // Another bundle may already have registered a class under our
        // preferred name, so keep counting until one is free.
        for (unsigned i = 0; i < 64 && sPassthroughClass == Nil; ++i)
        {
            NSString* const name =
                [NSString stringWithFormat:@"NimbostratusKeyPassthrough_%u", i];
            sPassthroughClass = objc_allocateClassPair(current, [name UTF8String], 0);
        }

        if (sPassthroughClass == Nil)
            return;

        class_addMethod(sPassthroughClass, @selector(keyDown:), (IMP)keyDownIMP, "v@:@");
        class_addMethod(sPassthroughClass, @selector(keyUp:), (IMP)keyUpIMP, "v@:@");
        objc_registerClassPair(sPassthroughClass);

        sBaseClass = current;
    }
    else if (current != sBaseClass)
    {
        // Not the class the subclass was derived from; leave the view alone.
        return;
    }

    object_setClass(view, sPassthroughClass);
}

} // namespace

void nimboSetKeyboardCapture(const uintptr_t nativeWindow, const bool capture)
{
    NSView* const view = (NSView*)nativeWindow;
    if (view == nil)
        return;

    ensurePatched(view);

    if (wantsCapture(view) != capture)
        objc_setAssociatedObject(view, &kCaptureKey,
                                 capture ? @YES : @NO,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);

    // Reclaim first responder before typing starts. Handing a key press to the
    // next responder is what lets the host keep its shortcuts, but a host that
    // acts on one usually makes its own view first responder in the process -
    // in Ableton Live, pressing space once is enough. After that this view
    // stops receiving -keyDown: entirely, so a numeric field would highlight
    // on click and then silently swallow every character. Text entry worked
    // exactly once per plugin window before this.
    if (!capture)
        return;

    NSWindow* const window = [view window];
    if (window != nil && [window firstResponder] != view)
        [window makeFirstResponder:view];
}

void nimboReleaseKeyboard(const uintptr_t nativeWindow)
{
    NSView* const view = (NSView*)nativeWindow;
    if (view == nil || object_getClass(view) != sPassthroughClass)
        return;

    objc_setAssociatedObject(view, &kCaptureKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(view, &kHeldKeysKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    object_setClass(view, sBaseClass);
}
