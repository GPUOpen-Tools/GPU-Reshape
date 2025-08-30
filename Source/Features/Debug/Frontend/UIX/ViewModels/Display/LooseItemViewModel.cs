using System;
using System.Runtime.InteropServices;
using DynamicData;
using GRS.Features.Debug.UIX.Models;
using Message.CLR;
using ReactiveUI;
using Runtime.Utils.Workspace;
using Studio.Models.Instrumentation;

namespace GRS.Features.Debug.UIX.ViewModels;

public class LooseItemViewModel : ReactiveObject
{
    /// <summary>
    /// The tree structure
    /// </summary>
    public LooseTreeItemViewModel RootItemViewModel { get; set; } = new();
    
    /// <summary>
    /// Virtual index of this item
    /// </summary>
    public required int Index { get; set; }
    
    /// <summary>
    /// Construct this loose item
    /// </summary>
    public LooseItemViewModel(LooseBreakpointDisplayViewModel breakpointDisplayViewModel, uint dwordOffset)
    {
        // Get export dword span
        Span<uint> dwordSpan = new(breakpointDisplayViewModel.DWords, (int)dwordOffset, (int)(LooseBreakpointHeader.DWordCount + breakpointDisplayViewModel.FlatInfo.dataDWordStride));
        
        // Interpret the execution info
        LooseBreakpointHeader header = MemoryMarshal.Read<LooseBreakpointHeader>(MemoryMarshal.AsBytes(dwordSpan));

        // Shared execution data
        LooseTreeItemViewModel executionInfoItem = new()
        {
            Text = "Execution Info",
            Items =
            [
                new LooseTreeItemViewModel { Text = $"{TracebackUtils.Format(header.executionInfo.executionFlags)}" },
                new LooseTreeItemViewModel { Text = $"Pipeline {header.executionInfo.pipelineUID}" },
                new LooseTreeItemViewModel { Text = $"Queue {header.executionInfo.queueUID}" },
                new LooseTreeItemViewModel { Text = $"Scope {header.executionInfo.scopeUID}" }
            ]
        };

        LooseTreeItemViewModel threadInfo = new LooseTreeItemViewModel()
        {
            Text = "Thread Info"
        };

        // Add dispatch parameters
        if (header.executionInfo.executionFlags.HasFlag(ExecutionFlag.Dispatch))
        {
            executionInfoItem.Items.Add(new LooseTreeItemViewModel()
            {
                Text = TracebackUtils.Format(header.executionInfo.executionFlags),
                Items =
                [
                    new LooseTreeItemViewModel { Text = $"Group Count X {header.executionInfo.dispatchInfo.groupCountX}" },
                    new LooseTreeItemViewModel { Text = $"Group Count Y {header.executionInfo.dispatchInfo.groupCountY}" },
                    new LooseTreeItemViewModel { Text = $"Group Count Z {header.executionInfo.dispatchInfo.groupCountZ}" }
                ]
            });

            threadInfo.Items.AddRange([
                new LooseTreeItemViewModel { Text = $"Thread X {header.threadX}" },
                new LooseTreeItemViewModel { Text = $"Thread Y {header.threadY}" },
                new LooseTreeItemViewModel { Text = $"Thread Z {header.threadZ}" }
            ]);
        }

        // Add draw parameters
        if (header.executionInfo.executionFlags.HasFlag(ExecutionFlag.Draw))
        {
            executionInfoItem.Items.Add(new LooseTreeItemViewModel()
            {
                Text = TracebackUtils.Format(header.executionInfo.executionFlags),
                Items =
                [
                    new LooseTreeItemViewModel { Text = $"Vertex Count {header.executionInfo.drawInfo.vertexCount}" },
                    new LooseTreeItemViewModel { Text = $"Index Count {header.executionInfo.drawInfo.indexCount}" }
                ]
            });

            threadInfo.Items.AddRange([
                new LooseTreeItemViewModel { Text = $"Vertex Count {header.threadX}" },
                new LooseTreeItemViewModel { Text = $"Index Count {header.threadY}" }
            ]);
        }
        
        // Data span begin
        Span<uint> dataDWordSpan = dwordSpan.Slice((int)ExecutionInfo.DWordCount);

        // TODO: Actually interpret the data
        LooseTreeItemViewModel dataItem = new()
        {
            Text = $"{dataDWordSpan[0]}"
        };
        
        // Add items
        RootItemViewModel.Items.AddRange([
            dataItem,
            threadInfo,
            executionInfoItem
        ]);
    }
}
