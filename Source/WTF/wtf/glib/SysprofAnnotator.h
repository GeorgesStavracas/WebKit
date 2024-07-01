/*
 * Copyright (C) 2024 Igalia, S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#pragma once

#include <cstdio>
#include <sysprof-capture.h>
#include <wtf/Lock.h>
#include <wtf/StdMap.h>
#include <wtf/text/ASCIILiteral.h>
#include <wtf/text/CString.h>

namespace WTF {

class SysprofAnnotator final {
    WTF_MAKE_NONCOPYABLE(SysprofAnnotator);

    using RawPointerPair = std::pair<const void*, const void*>;
    using TimestampAndString = std::pair<int64_t, std::string>;

public:

    static SysprofAnnotator& singleton()
    {
        static SysprofAnnotator instance;
        return instance;
    }

    void instantMark(const char *name, const char *description, ...) WTF_ATTRIBUTE_PRINTF(3, 4)
    {
        va_list args;
        va_start(args, description);
        sysprof_collector_mark_vprintf(SYSPROF_CAPTURE_CURRENT_TIME, 0, m_processName, name, description, args);
        va_end(args);
    }

    void mark(Seconds timeDelta, const char *name, const char *description, ...) WTF_ATTRIBUTE_PRINTF(4, 5)
    {
        va_list args;
        va_start(args, description);
        sysprof_collector_mark_vprintf(SYSPROF_CAPTURE_CURRENT_TIME, timeDelta.microsecondsAs<int64_t>(), m_processName, name, description, args);
        va_end(args);
    }

    void beginMark(const void* pointer, const char* name, const char* description, ...) WTF_ATTRIBUTE_PRINTF(4, 5)
    {
        auto key = std::make_pair(pointer, static_cast<const void*>(name));

        char buffer[1024];
        va_list args;
        va_start(args, description);
        vsnprintf(buffer, 1024, description, args);
        va_end(args);

        auto value = std::make_pair(SYSPROF_CAPTURE_CURRENT_TIME, buffer);

        Locker locker { m_lock };
        m_ongoingMarks[key] = value;
    }

    void endMark(const void* pointer, const char *name, const char* description, ...) WTF_ATTRIBUTE_PRINTF(4, 5)
    {
        auto key = std::make_pair(pointer, static_cast<const void*>(name));
        std::optional<TimestampAndString> value;

        Locker locker { m_lock };
        if (m_ongoingMarks.contains(key)) {
            value = WTFMove(m_ongoingMarks[key]);
            m_ongoingMarks.erase(key);
        }
        locker.unlockEarly();

        if (value) {
            int64_t startTime = std::get<0>(*value);
            const std::string& description = std::get<1>(*value);
            sysprof_collector_mark(startTime, SYSPROF_CAPTURE_CURRENT_TIME - startTime, m_processName, name, description.c_str());
        } else {
            va_list args;
            va_start(args, description);
            sysprof_collector_mark_vprintf(SYSPROF_CAPTURE_CURRENT_TIME, 0, m_processName, name, description, args);
            va_end(args);
        }
    }

    void tracePoint(TracePointCode code)
    {
        switch (code) {
        case VMEntryScopeStart:
        case WebAssemblyCompileStart:
        case WebAssemblyExecuteStart:
        case DumpJITMemoryStart:
        case FromJSStart:
        case IncrementalSweepStart:
        case MainResourceLoadDidStartProvisional:
        case SubresourceLoadWillStart:
        case FetchCookiesStart:
        case StyleRecalcStart:
        case RenderTreeBuildStart:
        case PerformLayoutStart:
        case PaintLayerStart:
        case AsyncImageDecodeStart:
        case RAFCallbackStart:
        case MemoryPressureHandlerStart:
        case UpdateTouchRegionsStart:
        case DisplayListRecordStart:
        case ComputeEventRegionsStart:
        case RenderingUpdateStart:
        case CompositingUpdateStart:
        case DispatchTouchEventsStart:
        case ParseHTMLStart:
        case DisplayListReplayStart:
        case ScrollingThreadRenderUpdateSyncStart:
        case ScrollingThreadDisplayDidRefreshStart:
        case RenderTreeLayoutStart:
        case PerformOpportunisticallyScheduledTasksStart:
        case WebXRLayerStartFrameStart:
        case WebXRLayerEndFrameStart:
        case WebXRSessionFrameCallbacksStart:
        case WebHTMLViewPaintStart:
        case BackingStoreFlushStart:
        case BuildTransactionStart:
        case SyncMessageStart:
        case SyncTouchEventStart:
        case InitializeWebProcessStart:
        case RenderingUpdateRunLoopObserverStart:
        case LayerTreeFreezeStart:
        case FlushRemoteImageBufferStart:
        case CreateInjectedBundleStart:
        case PaintSnapshotStart:
        case RenderServerSnapshotStart:
        case TakeSnapshotStart:
        case SyntheticMomentumStart:
        case CommitLayerTreeStart:
        case ProcessLaunchStart:
        case InitializeSandboxStart:
        case WebXRCPFrameWaitStart:
        case WebXRCPFrameStartSubmissionStart:
        case WebXRCPFrameEndSubmissionStart:
        case WakeUpAndApplyDisplayListStart:
            beginMark(nullptr, tracePointCodeName(code), "%s", "");
            break;

        case VMEntryScopeEnd:
        case WebAssemblyCompileEnd:
        case WebAssemblyExecuteEnd:
        case DumpJITMemoryStop:
        case FromJSStop:
        case IncrementalSweepEnd:
        case MainResourceLoadDidEnd:
        case SubresourceLoadDidEnd:
        case FetchCookiesEnd:
        case StyleRecalcEnd:
        case RenderTreeBuildEnd:
        case PerformLayoutEnd:
        case PaintLayerEnd:
        case AsyncImageDecodeEnd:
        case RAFCallbackEnd:
        case MemoryPressureHandlerEnd:
        case UpdateTouchRegionsEnd:
        case DisplayListRecordEnd:
        case ComputeEventRegionsEnd:
        case RenderingUpdateEnd:
        case CompositingUpdateEnd:
        case DispatchTouchEventsEnd:
        case ParseHTMLEnd:
        case DisplayListReplayEnd:
        case ScrollingThreadRenderUpdateSyncEnd:
        case ScrollingThreadDisplayDidRefreshEnd:
        case RenderTreeLayoutEnd:
        case PerformOpportunisticallyScheduledTasksEnd:
        case WebXRLayerStartFrameEnd:
        case WebXRLayerEndFrameEnd:
        case WebXRSessionFrameCallbacksEnd:
        case WebHTMLViewPaintEnd:
        case BackingStoreFlushEnd:
        case BuildTransactionEnd:
        case SyncMessageEnd:
        case SyncTouchEventEnd:
        case InitializeWebProcessEnd:
        case RenderingUpdateRunLoopObserverEnd:
        case LayerTreeFreezeEnd:
        case FlushRemoteImageBufferEnd:
        case CreateInjectedBundleEnd:
        case PaintSnapshotEnd:
        case RenderServerSnapshotEnd:
        case TakeSnapshotEnd:
        case SyntheticMomentumEnd:
        case CommitLayerTreeEnd:
        case ProcessLaunchEnd:
        case InitializeSandboxEnd:
        case WebXRCPFrameWaitEnd:
        case WebXRCPFrameStartSubmissionEnd:
        case WebXRCPFrameEndSubmissionEnd:
        case WakeUpAndApplyDisplayListEnd:
            endMark(nullptr, tracePointCodeName(code), "%s", "");
            break;

        case DisplayRefreshDispatchingToMainThread:
        case ScheduleRenderingUpdate:
        case TriggerRenderingUpdate:
        case ScrollingTreeDisplayDidRefresh:
        case SyntheticMomentumEvent:
        case RemoteLayerTreeScheduleRenderingUpdate:
            instantMark(tracePointCodeName(code), "%s", "");
            break;

        case WTFRange:
        case JavaScriptRange:
        case WebCoreRange:
        case WebKitRange:
        case WebKit2Range:
        case UIProcessRange:
        case GPUProcessRange:
            break;
        }
    }

    static void initialize(const char* processName)
    {
        singleton().m_processName = ASCIILiteral::fromLiteralUnsafe(processName);
        sysprof_collector_init();
    }

private:
    static const char* tracePointCodeName(TracePointCode code)
    {
        switch (code) {
        case VMEntryScopeStart:
        case VMEntryScopeEnd:
            return "VMEntryScope";
        case WebAssemblyCompileStart:
        case WebAssemblyCompileEnd:
            return "WebAssemblyCompile";
        case WebAssemblyExecuteStart:
        case WebAssemblyExecuteEnd:
            return "WebAssemblyExecute";
        case DumpJITMemoryStart:
        case DumpJITMemoryStop:
            return "DumpJITMemory";
        case FromJSStart:
        case FromJSStop:
            return "FromJS";
        case IncrementalSweepStart:
        case IncrementalSweepEnd:
            return "IncrementalSweep";

        case MainResourceLoadDidStartProvisional:
        case MainResourceLoadDidEnd:
            return "MainResourceLoad";
        case SubresourceLoadWillStart:
        case SubresourceLoadDidEnd:
            return "SubresourceLoad";
        case FetchCookiesStart:
        case FetchCookiesEnd:
            return "FetchCookies";
        case StyleRecalcStart:
        case StyleRecalcEnd:
            return "StyleRecalc";
        case RenderTreeBuildStart:
        case RenderTreeBuildEnd:
            return "RenderTreeBuild";
        case PerformLayoutStart:
        case PerformLayoutEnd:
            return "PerformLayout";
        case PaintLayerStart:
        case PaintLayerEnd:
            return "PaintLayer";
        case AsyncImageDecodeStart:
        case AsyncImageDecodeEnd:
            return "AsyncImageDecode";
        case RAFCallbackStart:
        case RAFCallbackEnd:
            return "RAFCallback";
        case MemoryPressureHandlerStart:
        case MemoryPressureHandlerEnd:
            return "MemoryPressureHandler";
        case UpdateTouchRegionsStart:
        case UpdateTouchRegionsEnd:
            return "UpdateTouchRegions";
        case DisplayListRecordStart:
        case DisplayListRecordEnd:
            return "DisplayListRecord";
        case DisplayRefreshDispatchingToMainThread:
            return "DisplayRefreshDispatchingToMainThread";
        case ComputeEventRegionsStart:
        case ComputeEventRegionsEnd:
            return "ComputeEventRegions";
        case ScheduleRenderingUpdate:
            return "ScheduleRenderingUpdate";
        case TriggerRenderingUpdate:
            return "TriggerRenderingUpdate";
        case RenderingUpdateStart:
        case RenderingUpdateEnd:
            return "RenderingUpdate";
        case CompositingUpdateStart:
        case CompositingUpdateEnd:
            return "CompositingUpdate";
        case DispatchTouchEventsStart:
        case DispatchTouchEventsEnd:
            return "DispatchTouchEvents";
        case ParseHTMLStart:
        case ParseHTMLEnd:
            return "ParseHTML";
        case DisplayListReplayStart:
        case DisplayListReplayEnd:
            return "DisplayListReplay";
        case ScrollingThreadRenderUpdateSyncStart:
        case ScrollingThreadRenderUpdateSyncEnd:
            return "ScrollingThreadRenderUpdateSync";
        case ScrollingThreadDisplayDidRefreshStart:
        case ScrollingThreadDisplayDidRefreshEnd:
            return "ScrollingThreadDisplayDidRefresh";
        case ScrollingTreeDisplayDidRefresh:
            return "ScrollingTreeDisplayDidRefresh";
        case RenderTreeLayoutStart:
        case RenderTreeLayoutEnd:
            return "RenderTreeLayout";
        case PerformOpportunisticallyScheduledTasksStart:
        case PerformOpportunisticallyScheduledTasksEnd:
            return "PerformOpportunisticallyScheduledTasks";
        case WebXRLayerStartFrameStart:
        case WebXRLayerStartFrameEnd:
            return "WebXRLayerStartFrame";
        case WebXRLayerEndFrameStart:
        case WebXRLayerEndFrameEnd:
            return "WebXRLayerEndFrame";
        case WebXRSessionFrameCallbacksStart:
        case WebXRSessionFrameCallbacksEnd:
            return "WebXRSessionFrameCallbacks";

        case WebHTMLViewPaintStart:
        case WebHTMLViewPaintEnd:
            return "WebHTMLViewPaint";

        case BackingStoreFlushStart:
        case BackingStoreFlushEnd:
            return "BackingStoreFlush";
        case BuildTransactionStart:
        case BuildTransactionEnd:
            return "BuildTransaction";
        case SyncMessageStart:
        case SyncMessageEnd:
            return "SyncMessage";
        case SyncTouchEventStart:
        case SyncTouchEventEnd:
            return "SyncTouchEvent";
        case InitializeWebProcessStart:
        case InitializeWebProcessEnd:
            return "InitializeWebProcess";
        case RenderingUpdateRunLoopObserverStart:
        case RenderingUpdateRunLoopObserverEnd:
            return "RenderingUpdateRunLoopObserver";
        case LayerTreeFreezeStart:
        case LayerTreeFreezeEnd:
            return "LayerTreeFreeze";
        case FlushRemoteImageBufferStart:
        case FlushRemoteImageBufferEnd:
            return "FlushRemoteImageBuffer";
        case CreateInjectedBundleStart:
        case CreateInjectedBundleEnd:
            return "CreateInjectedBundle";
        case PaintSnapshotStart:
        case PaintSnapshotEnd:
            return "PaintSnapshot";
        case RenderServerSnapshotStart:
        case RenderServerSnapshotEnd:
            return "RenderServerSnapshot";
        case TakeSnapshotStart:
        case TakeSnapshotEnd:
            return "TakeSnapshot";
        case SyntheticMomentumStart:
        case SyntheticMomentumEnd:
            return "SyntheticMomentum";
        case SyntheticMomentumEvent:
            return "SyntheticMomentumEvent";
        case RemoteLayerTreeScheduleRenderingUpdate:
            return "RemoteLayerTreeScheduleRenderingUpdate";

        case CommitLayerTreeStart:
        case CommitLayerTreeEnd:
            return "CommitLayerTree";
        case ProcessLaunchStart:
        case ProcessLaunchEnd:
            return "ProcessLaunch";
        case InitializeSandboxStart:
        case InitializeSandboxEnd:
            return "InitializeSandbox";
        case WebXRCPFrameWaitStart:
        case WebXRCPFrameWaitEnd:
            return "WebXRCPFrameWait";
        case WebXRCPFrameStartSubmissionStart:
        case WebXRCPFrameStartSubmissionEnd:
            return "WebXRCPFrameStartSubmission";
        case WebXRCPFrameEndSubmissionStart:
        case WebXRCPFrameEndSubmissionEnd:
            return "WebXRCPFrameEndSubmission";

        case WakeUpAndApplyDisplayListStart:
        case WakeUpAndApplyDisplayListEnd:
            return "WakeUpAndApplyDisplayList";

        case WTFRange:
        case JavaScriptRange:
        case WebCoreRange:
        case WebKitRange:
        case WebKit2Range:
        case UIProcessRange:
        case GPUProcessRange:
        default:
            return nullptr;
        }
    }

    SysprofAnnotator() { };
    ~SysprofAnnotator() { };

    ASCIILiteral m_processName;
    Lock m_lock;
    StdMap<RawPointerPair, TimestampAndString> m_ongoingMarks WTF_GUARDED_BY_LOCK(m_lock);
};

} // namespace WTF
