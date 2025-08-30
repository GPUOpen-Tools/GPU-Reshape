using System;
using System.Collections.ObjectModel;
using System.Runtime.InteropServices;
using DynamicData;
using GRS.Features.Debug.UIX.Models;
using Message.CLR;
using ReactiveUI;
using Runtime.Utils.Workspace;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Services;

namespace GRS.Features.Debug.UIX.ViewModels;

public class LooseItemViewModel : ReactiveObject
{
    /// <summary>
    /// The tree structure
    /// </summary>
    public LooseTreeItemViewModel RootItemViewModel { get; set; } = new();

    /// <summary>
    /// All flattened items
    /// </summary>
    public ObservableCollection<LooseTreeItemViewModel> FlatItems { get; } = new();
    
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
                GetPipelineItemViewModel(breakpointDisplayViewModel, header),
                new LooseTreeItemViewModel { Text = $"Queue : {header.executionInfo.queueUID}" },
                new LooseTreeItemViewModel { Text = $"Scope : {header.executionInfo.scopeUID}" }
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
                    new LooseTreeItemViewModel { Text = $"Group Count X : {header.executionInfo.dispatchInfo.groupCountX}" },
                    new LooseTreeItemViewModel { Text = $"Group Count Y : {header.executionInfo.dispatchInfo.groupCountY}" },
                    new LooseTreeItemViewModel { Text = $"Group Count Z : {header.executionInfo.dispatchInfo.groupCountZ}" }
                ]
            });

            threadInfo.Items.AddRange([
                new LooseTreeItemViewModel { Text = $"Thread X : {header.threadX}" },
                new LooseTreeItemViewModel { Text = $"Thread Y : {header.threadY}" },
                new LooseTreeItemViewModel { Text = $"Thread Z : {header.threadZ}" }
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
                    new LooseTreeItemViewModel { Text = $"Vertex Count : {header.executionInfo.drawInfo.vertexCount}" },
                    new LooseTreeItemViewModel { Text = $"Index Count  : {header.executionInfo.drawInfo.indexCount}" }
                ]
            });

            threadInfo.Items.AddRange([
                new LooseTreeItemViewModel { Text = $"Vertex Count : {header.threadX}" },
                new LooseTreeItemViewModel { Text = $"Index Count  : {header.threadY}" }
            ]);
        }
        
        // Data span begin
        Span<uint> dataDWordSpan = dwordSpan.Slice((int)LooseBreakpointHeader.DWordCount);

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
        
        // Flatten them all
        foreach (IObservableTreeItem observableTreeItem in RootItemViewModel.Items)
        {
            FlattenHierarchy((LooseTreeItemViewModel)observableTreeItem);
        }
    }

    /// <summary>
    /// Get a bound pipeline name item
    /// </summary>
    private LooseTreeItemViewModel GetPipelineItemViewModel(LooseBreakpointDisplayViewModel breakpointDisplayViewModel, LooseBreakpointHeader header)
    {
        LooseTreeItemViewModel item = new() { Text = $"Pipeline : {header.executionInfo.pipelineUID}" };

        // If possible, bind the name
        if (breakpointDisplayViewModel.ShaderProperty?.GetWorkspaceCollection() is { } workspaceCollection &&
            workspaceCollection.GetProperty<IPipelineCollectionViewModel>() is { } collection &&
            workspaceCollection.GetService<IPipelinePoolingService>() is { } pooling)
        {
            PipelineViewModel pipelineViewModel = collection.GetOrAddPipeline(header.executionInfo.pipelineUID);
            
            // Make sure it's pooling
            pooling.EnqueuePipeline(pipelineViewModel);
        
            // Bind on changes
            pipelineViewModel.WhenAnyValue(x => x.Name).Subscribe(name =>
            {
                if (!string.IsNullOrWhiteSpace(name))
                {
                    item.Text = $"Pipeline : {name}";
                }
            });
        }

        return item;
    }

    /// <summary>
    /// Flatten an item
    /// </summary>
    private void FlattenHierarchy(LooseTreeItemViewModel itemViewModel)
    {
        FlatItems.Add(itemViewModel);

        if (itemViewModel.Items.Count == 0)
        {
            return;
        }
        
        FlatItems.Add(new LooseTreeItemViewModel { Text = "{" });

        foreach (IObservableTreeItem observableTreeItem in itemViewModel.Items)
        {
            FlattenHierarchy((LooseTreeItemViewModel)observableTreeItem);
        }

        FlatItems.Add(new LooseTreeItemViewModel { Text = "}" });
    }
}
