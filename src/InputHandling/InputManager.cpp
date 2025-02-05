#include "InputHandling/InputManager.hpp"
#include "InputHandling/TriggerHandler.hpp"
#include "InputHandling/GripHandler.hpp"
#include "InputHandling/ThumbstickHandler.hpp"
#include "config.hpp"

#include "GlobalNamespace/OVRInput.hpp"
#include "UnityEngine/XR/XRNode.hpp"
#include "UnityEngine/XR/InputDevices.hpp"
#include "UnityEngine/XR/InputDevice.hpp"
#include "UnityEngine/XR/InputTracking.hpp"

DEFINE_TYPE(TrickSaber::InputHandling, InputManager);

namespace TrickSaber::InputHandling {
    void InputManager::ctor() {
        INVOKE_CTOR();
        _trickInputHandler = TrickInputHandler::New_ctor();
    }

    void InputManager::Init(GlobalNamespace::SaberType saberType, GlobalNamespace::VRControllersInputManager* vrControllersInputManager) {
        GlobalNamespace::OVRInput::Controller oculusController;
        UnityEngine::XR::XRNode node;
        if (saberType == GlobalNamespace::SaberType::SaberA)
        {
            oculusController = GlobalNamespace::OVRInput::Controller::LTouch;
            node = UnityEngine::XR::XRNode::LeftHand;
        }
        else
        {
            oculusController = GlobalNamespace::OVRInput::Controller::RTouch;
            node = UnityEngine::XR::XRNode::RightHand;
        }

        using GetDeviceIdAtXRNode = function_ptr_t<uint64_t, UnityEngine::XR::XRNode>;
        static auto getDeviceIdAtXRNode = reinterpret_cast<GetDeviceIdAtXRNode>(il2cpp_functions::resolve_icall("UnityEngine.XR.InputTracking::GetDeviceIdAtXRNode"));
        auto controllerInputDevice = UnityEngine::XR::InputDevice(getDeviceIdAtXRNode(node), true);
        auto dir = config.thumbstickDirection;

        auto triggerHandler = TriggerHandler::New_ctor(node, config.triggerThreshold, config.reverseTrigger);
        auto gripHandler = GripHandler::New_ctor(oculusController, controllerInputDevice, config.gripThreshold, config.reverseGrip);
        auto thumbstickAction = ThumbstickHandler::New_ctor(node, dir, config.thumbstickThreshold, config.reverseThumbstick);

        _trickInputHandler->Add(config.triggerAction, triggerHandler);
        _trickInputHandler->Add(config.gripAction, gripHandler);
        _trickInputHandler->Add(config.thumbstickAction, thumbstickAction);
    }

    // Using ITickable seems to result in GetHandlers returning no handlers (?!?)
    // So we need to manually tick
    void InputManager::Tick() {
        auto keys = _trickInputHandler->trickHandlerSets->get_Keys();
        auto iter = keys->GetEnumerator();
        while (iter.MoveNext()) {
            auto trickAction = iter.get_Current();
            auto handlers = _trickInputHandler->GetHandlers(trickAction);
            float val = 0.0f;
            if (CheckHandlersDown(handlers, val))
                trickActivated.invoke(trickAction, val);
            else if (CheckHandlersUp(handlers)) trickDeactivated.invoke(trickAction);
        }
        iter.Dispose();
    }

    bool InputManager::CheckHandlersDown(TrickInputHandler::TrickHandlerHashSet* handlers, float& val) {
        val = 0;
        if (handlers->get_Count() == 0) return false;
        bool output = true;
        auto iter = handlers->GetEnumerator();
        while (iter.MoveNext()) {
            auto handler = iter.get_Current();
            float handlerValue = 0.0f;
            output &= handler->Activated(handlerValue);
            val += handlerValue;
        }
        iter.Dispose();

        if (output) val /= handlers->get_Count();

        return output;
    }

    bool InputManager::CheckHandlersUp(TrickInputHandler::TrickHandlerHashSet* handlers) {
        auto iter = handlers->GetEnumerator();
        while (iter.MoveNext()) {
            auto handler = iter.get_Current();
            if (handler->Deactivated()) {
                iter.Dispose();
                return true;
            }
        }
        iter.Dispose();
        return false;
    }

}