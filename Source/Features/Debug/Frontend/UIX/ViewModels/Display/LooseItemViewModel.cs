using System;
using System.Runtime.InteropServices;
using System.Text;
using DynamicData;
using GRS.Features.Debug.UIX.Models;
using GRS.Features.Debug.UIX.ViewModels.Utils;
using Message.CLR;
using ReactiveUI;
using Runtime.Utils.Workspace;
using Runtime.ViewModels.IL;
using Studio.Models.IL;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Services;
using Type = Studio.Models.IL.Type;

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
    public string FlatString
    {
        get => _flatString;
        set => this.RaiseAndSetIfChanged(ref _flatString, value);
    }
    
    /// <summary>
    /// Virtual index of this item
    /// </summary>
    public required int Index { get; set; }
    
    /// <summary>
    /// Construct this loose item
    /// </summary>
    public LooseItemViewModel(LooseWatchpointDisplayViewModel watchpointDisplayViewModel, uint dwordOffset)
    {
        _reEntryState = true;
        
        // Get export dword span
        Span<uint> dwordSpan = new(watchpointDisplayViewModel.DWords, (int)dwordOffset, (int)(LooseWatchpointHeader.DWordCount + watchpointDisplayViewModel.FlatInfo.dataDWordStride));
        
        // Interpret the execution info
        LooseWatchpointHeader header = MemoryMarshal.Read<LooseWatchpointHeader>(MemoryMarshal.AsBytes(dwordSpan));

        // Shared execution data
        LooseTreeItemViewModel executionInfoItem = new()
        {
            Text = "Execution Info",
            Items =
            [
                new LooseTreeItemViewModel { Text = $"{TracebackUtils.Format(header.executionInfo.executionFlags)}" },
                GetPipelineItemViewModel(watchpointDisplayViewModel, header),
                new LooseTreeItemViewModel { Text = $"Queue : {header.executionInfo.queueUID}" }
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
            var item = new LooseTreeItemViewModel()
            {
                Text = TracebackUtils.Format(header.executionInfo.executionFlags)
            };

            if (header.executionInfo.drawInfo.drawFlags.HasFlag(ExecutionDrawFlag.VertexCountPerInstance))
            {
                item.Items.Add(new LooseTreeItemViewModel { Text = $"Vertex Count Per Instance : {header.executionInfo.drawInfo.vertexCountPerInstance}" });
            }

            if (header.executionInfo.drawInfo.drawFlags.HasFlag(ExecutionDrawFlag.IndexCountPerInstance))
            {
                item.Items.Add(new LooseTreeItemViewModel { Text = $"Index Count Per Instance : {header.executionInfo.drawInfo.indexCountPerInstance}" });
            }

            if (header.executionInfo.drawInfo.drawFlags.HasFlag(ExecutionDrawFlag.InstanceCount))
            {
                item.Items.Add(new LooseTreeItemViewModel { Text = $"Instance Count : {header.executionInfo.drawInfo.instanceCount}" });
            }

            if (header.executionInfo.drawInfo.drawFlags.HasFlag(ExecutionDrawFlag.StartVertex))
            {
                item.Items.Add(new LooseTreeItemViewModel { Text = $"Start Vertex : {header.executionInfo.drawInfo.startVertex}" });
            }

            if (header.executionInfo.drawInfo.drawFlags.HasFlag(ExecutionDrawFlag.StartIndex))
            {
                item.Items.Add(new LooseTreeItemViewModel { Text = $"Start Index : {header.executionInfo.drawInfo.startIndex}" });
            }

            if (header.executionInfo.drawInfo.drawFlags.HasFlag(ExecutionDrawFlag.StartInstance))
            {
                item.Items.Add(new LooseTreeItemViewModel { Text = $"Start Instance : {header.executionInfo.drawInfo.startInstance}" });
            }

            if (header.executionInfo.drawInfo.drawFlags.HasFlag(ExecutionDrawFlag.VertexOffset))
            {
                item.Items.Add(new LooseTreeItemViewModel { Text = $"Vertex Offset : {header.executionInfo.drawInfo.vertexOffset}" });
            }

            if (header.executionInfo.drawInfo.drawFlags.HasFlag(ExecutionDrawFlag.InstanceOffset))
            {
                item.Items.Add(new LooseTreeItemViewModel { Text = $"Instance Offset : {header.executionInfo.drawInfo.instanceOffset}" });
            }
            
            executionInfoItem.Items.Add(item);

            threadInfo.Items.AddRange([
                new LooseTreeItemViewModel { Text = $"Vertex ID : {header.threadX}" }
            ]);
        }
        
        // Data span begin
        Span<uint> dataDWordSpan = dwordSpan.Slice((int)LooseWatchpointHeader.DWordCount);

        // TODO: Actually interpret the data
        LooseTreeItemViewModel dataItem = GetValueItem(watchpointDisplayViewModel, header, dataDWordSpan, dwordOffset + LooseWatchpointHeader.DWordCount);
        
        // Add items
        RootItemViewModel.Items.AddRange([
            dataItem,
            threadInfo,
            executionInfoItem
        ]);

        // Allow update based flattening
        _reEntryState = false;
        
        // Flatten them all
        Flatten();
    }

    /// <summary>
    /// Flatten the hierarchy
    /// </summary>
    private void Flatten()
    {
        if (_reEntryState)
        {
            return;
        }
        
        StringBuilder builder = new();
        
        // Manually flatten first level
        foreach (IObservableTreeItem observableTreeItem in RootItemViewModel.Items)
        {
            FlattenHierarchy((LooseTreeItemViewModel)observableTreeItem, builder);
            builder.Append(' ');
        }
        
        FlatString = builder.ToString();
    }

    /// <summary>
    /// Get a bound value item
    /// </summary>
    private LooseTreeItemViewModel GetValueItem(LooseWatchpointDisplayViewModel watchpointDisplayViewModel, LooseWatchpointHeader header, Span<uint> dataDWordSpan, uint dwordOffset)
    {
        // Due to Span GC rules, create it anew here
        // The underlying memory is guaranteed to exist
        Span<uint> dwordSpan = new(watchpointDisplayViewModel.DWords, (int)dwordOffset, (int)watchpointDisplayViewModel.FlatInfo.dataDWordStride);
        
        // Just keep it under its own category
        LooseTreeItemViewModel item = new()
        {
            Text = $"Value {Assembler.AssembleInlineType(watchpointDisplayViewModel.Type, true)} "
        };
        
        // Format the bytes according to its type
        Span<byte> dataSpan = MemoryMarshal.AsBytes(dwordSpan);
        FormatValue(item, watchpointDisplayViewModel.Type, ref dataSpan, true);

        // Update the flat string
        Flatten();

        return item;
    }

    /// <summary>
    /// Get a bound pipeline name item
    /// </summary>
    private LooseTreeItemViewModel GetPipelineItemViewModel(LooseWatchpointDisplayViewModel watchpointDisplayViewModel, LooseWatchpointHeader header)
    {
        LooseTreeItemViewModel item = new() { Text = $"Pipeline : {header.executionInfo.pipelineUID}" };

        // If possible, bind the name
        if (watchpointDisplayViewModel.PropertyViewModel.GetProperty<IPipelineCollectionViewModel>() is { } collection &&
            watchpointDisplayViewModel.PropertyViewModel.GetService<IPipelinePoolingService>() is { } pooling)
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

                    // Update the flat string
                    Flatten();
                }
            });
        }

        return item;
    }

    /// <summary>
    /// Format an opaque value
    /// </summary>
    /// <param name="item">item to append to</param>
    /// <param name="type">il type</param>
    /// <param name="byteSpan">current byte span</param>
    private void FormatValue(LooseTreeItemViewModel item, Type type, ref Span<byte> byteSpan, bool isFirstItem)
    {
        switch (type.Kind)
        {
            default:
            {
                item.Items.Add(new LooseTreeItemViewModel { Text = type.Kind.ToString() });
                break;
            }
            case TypeKind.Bool:
            {
                item.Items.Add(new LooseTreeItemViewModel { Text = ValueTypeFormattingUtils.FormatBool((BoolType)type, ref byteSpan)});
                break;
            }
            case TypeKind.Int:
            {
                item.Items.Add(new LooseTreeItemViewModel { Text = ValueTypeFormattingUtils.FormatInt((IntType)type, ref byteSpan)});
                break;
            }
            case TypeKind.FP:
            {
                item.Items.Add(new LooseTreeItemViewModel { Text = ValueTypeFormattingUtils.FormatFP((FPType)type, ref byteSpan)});
                break;
            }
            case TypeKind.Vector:
            {
                var typed = (VectorType)type;

                if (!isFirstItem)
                {
                    LooseTreeItemViewModel subType = new();
                    item.Items.Add(subType);
                    item = subType;
                }

                for (int i = 0; i < typed.Dimension; i++)
                {
                    FormatValue(item, typed.ContainedType, ref byteSpan, false);
                }
                break;
            }
            case TypeKind.Array:
            {
                var typed = (ArrayType)type;
                
                if (!isFirstItem)
                {
                    LooseTreeItemViewModel subType = new();
                    item.Items.Add(subType);
                    item = subType;
                }

                for (int i = 0; i < typed.Count; i++)
                {
                    FormatValue(item, typed.ElementType, ref byteSpan, false);
                }
                break;
            }
            case TypeKind.Matrix:
            {
                var typed = (MatrixType)type;
                
                if (!isFirstItem)
                {
                    LooseTreeItemViewModel subType = new();
                    item.Items.Add(subType);
                    item = subType;
                }

                for (int row = 0; row < typed.Rows; row++)
                {
                    LooseTreeItemViewModel rowItem = new() { Text = $"Row {row}" };
                    
                    for (int column = 0; column < typed.Columns; column++)
                    {
                        FormatValue(rowItem, typed.ContainedType, ref byteSpan, false);
                    }
                    
                    item.Items.Add(rowItem);
                }
                break;
            }
            case TypeKind.Struct:
            {
                var typed = (StructType)type;
                
                if (!isFirstItem)
                {
                    LooseTreeItemViewModel subType = new();
                    item.Items.Add(subType);
                    item = subType;
                }

                for (int i = 0; i < typed.MemberTypes.Length; i++)
                {
                    FormatValue(item, typed.MemberTypes[i], ref byteSpan, false);
                }
                break;
            }
        }
    }

    /// <summary>
    /// Flatten an item
    /// </summary>
    private void FlattenHierarchy(LooseTreeItemViewModel itemViewModel, StringBuilder builder)
    {
        builder.Append(itemViewModel.Text);

        if (itemViewModel.Items.Count == 0)
        {
            return;
        }
        
        builder.Append(" { ");

        foreach (IObservableTreeItem observableTreeItem in itemViewModel.Items)
        {
            FlattenHierarchy((LooseTreeItemViewModel)observableTreeItem, builder);
            builder.Append(' ');
        }

        builder.Append(" } ");
    }

    /// <summary>
    /// Internal flat string
    /// </summary>
    private string _flatString;

    /// <summary>
    /// Re-entry check
    /// </summary>
    private bool _reEntryState = false;
}
