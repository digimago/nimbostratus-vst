// Hands unused key presses back to the host on macOS.
//
// pugl makes its wrapper NSView the first responder the moment an embedded
// plugin view is realized, and its -keyDown:/-keyUp: never forward anything up
// the responder chain. Inside a DAW that means an open plugin window eats the
// host's key commands - most painfully the spacebar, which normally starts and
// stops transport in Ableton Live.
//
// The UI reports once per frame whether Dear ImGui is taking typed input (the
// pitch knob's numeric entry is the only place that happens). On the first
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

// Per-view flag, kept as an associated object: the views already exist by the
// time the class is swapped in, so an ivar is not an option.
char kCaptureKey;

Class sBaseClass        = Nil;
Class sPassthroughClass = Nil;
IMP   sOriginalKeyDown  = nullptr;
IMP   sOriginalKeyUp    = nullptr;

bool wantsCapture(id self)
{
    NSNumber* const flag = objc_getAssociatedObject(self, &kCaptureKey);
    return flag != nil && [flag boolValue];
}

void keyDownIMP(id self, SEL cmd, NSEvent* event)
{
    if (wantsCapture(self))
        ((void (*)(id, SEL, NSEvent*))sOriginalKeyDown)(self, cmd, event);
    else
        [[(NSView*)self nextResponder] keyDown:event];
}

void keyUpIMP(id self, SEL cmd, NSEvent* event)
{
    if (wantsCapture(self))
        ((void (*)(id, SEL, NSEvent*))sOriginalKeyUp)(self, cmd, event);
    else
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
}

void nimboReleaseKeyboard(const uintptr_t nativeWindow)
{
    NSView* const view = (NSView*)nativeWindow;
    if (view == nil || object_getClass(view) != sPassthroughClass)
        return;

    objc_setAssociatedObject(view, &kCaptureKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    object_setClass(view, sBaseClass);
}

