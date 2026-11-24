#pragma once
#include "PRS/RHI/Types.h"

namespace PRS::RHI
{
class CommandList;
class Fence;

class Queue
{
public:
    virtual ~Queue() = default;

    // Submits an already recorded command list. Offscreen/upload work uses this;
    // presentable frames go through SwapChain::Submit.
    virtual void Submit(CommandList& commandList, Fence* signalFence = nullptr) = 0;
    virtual void WaitIdle() = 0;
};
} // namespace PRS::RHI
