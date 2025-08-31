using System;
using System.Collections.ObjectModel;
using System.Runtime.InteropServices;
using System.Text;
using DynamicData;
using GRS.Features.Debug.UIX.Models;
using GRS.Features.Debug.UIX.ViewModels.Utils;
using Message.CLR;
using ReactiveUI;
using Runtime.Utils.Workspace;
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
    public string FlatString { get; } = string.Empty;
    
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
        LooseTreeItemViewModel dataItem = GetValueItem(breakpointDisplayViewModel, header, dataDWordSpan, dwordOffset + LooseBreakpointHeader.DWordCount);
        
        // Add items
        RootItemViewModel.Items.AddRange([
            dataItem,
            threadInfo,
            executionInfoItem
        ]);
        
        // Flatten them all
        StringBuilder builder = new();
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
    private LooseTreeItemViewModel GetValueItem(LooseBreakpointDisplayViewModel breakpointDisplayViewModel, LooseBreakpointHeader header, Span<uint> dataDWordSpan, uint dwordOffset)
    {
        StringBuilder rawBuffer = new();

        // By default, format the raw data in hex
        for (int i = 0; i < dataDWordSpan.Length; i++)
        {
            if (i != 0)
            {
                rawBuffer.Append(", ");
            }
            
            rawBuffer.Append("0x");
            rawBuffer.Append(dataDWordSpan[i].ToString("X"));
        }
        
        LooseTreeItemViewModel item = new() { Text = $"Value : Raw [{rawBuffer}]" };

        // If possible, bind the value deserialization to the given type id
        if (breakpointDisplayViewModel.ShaderProperty?.GetWorkspaceCollection() is { } workspaceCollection &&
            workspaceCollection.GetProperty<IShaderCollectionViewModel>() is { } collection &&
            workspaceCollection.GetService<IShaderCodeService>() is { } pooling)
        {
            ShaderViewModel shaderViewModel = collection.GetOrAddShader(breakpointDisplayViewModel.ShaderProperty.Shader.GUID);
            
            // Make sure it's pooling
            pooling.EnqueueShaderIL(shaderViewModel);

            // Bind on changes
            shaderViewModel.WhenAnyValue(x => x.Program).WhereNotNull().Subscribe(program =>
            {
                Type type = (Type)program.Lookup[breakpointDisplayViewModel.FlatInfo.dataTypeId];

                // Due to Span GC rules, create it anew here
                // The underlying memory is guaranteed to exist
                Span<uint> dwordSpan = new(breakpointDisplayViewModel.DWords, (int)dwordOffset, (int)breakpointDisplayViewModel.FlatInfo.dataDWordStride);

                // Just keep it under its own category
                item.Text = "Value";
                
                // Format the bytes according to its type
                Span<byte> dataSpan = MemoryMarshal.AsBytes(dwordSpan);
                FormatValue(item, type, ref dataSpan);
            });
        }

        return item;
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
    /// Format an opaque value
    /// </summary>
    /// <param name="item">item to append to</param>
    /// <param name="type">il type</param>
    /// <param name="byteSpan">current byte span</param>
    private void FormatValue(LooseTreeItemViewModel item, Type type, ref Span<byte> byteSpan)
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
                item.Items.Add(new LooseTreeItemViewModel { Text = TypeFormattingUtils.FormatBool((BoolType)type, ref byteSpan)});
                break;
            }
            case TypeKind.Int:
            {
                item.Items.Add(new LooseTreeItemViewModel { Text = TypeFormattingUtils.FormatInt((IntType)type, ref byteSpan)});
                break;
            }
            case TypeKind.FP:
            {
                item.Items.Add(new LooseTreeItemViewModel { Text = TypeFormattingUtils.FormatFP((FPType)type, ref byteSpan)});
                break;
            }
            case TypeKind.Vector:
            {
                var typed = (VectorType)type;
                
                LooseTreeItemViewModel vectorType = new() { Text = "Vector" };

                for (int i = 0; i < typed.Dimension; i++)
                {
                    FormatValue(vectorType, typed.ContainedType, ref byteSpan);
                }

                item.Items.Add(vectorType);
                break;
            }
            case TypeKind.Array:
            {
                var typed = (ArrayType)type;
                
                LooseTreeItemViewModel arrayItem = new() { Text = "Array" };

                for (int i = 0; i < typed.Count; i++)
                {
                    FormatValue(arrayItem, typed.ElementType, ref byteSpan);
                }

                item.Items.Add(arrayItem);
                break;
            }
            case TypeKind.Matrix:
            {
                var typed = (MatrixType)type;
                
                LooseTreeItemViewModel matrixItem = new() { Text = "Matrix" };

                for (int row = 0; row < typed.Rows; row++)
                {
                    LooseTreeItemViewModel rowItem = new() { Text = $"Row {row}" };
                    
                    for (int column = 0; column < typed.Columns; column++)
                    {
                        LooseTreeItemViewModel columnItem = new() { Text = $"Column {column}" };
                        FormatValue(matrixItem, typed.ContainedType, ref byteSpan);
                        rowItem.Items.Add(columnItem);
                    }
                    
                    matrixItem.Items.Add(rowItem);
                }

                item.Items.Add(matrixItem);
                break;
            }
            case TypeKind.Struct:
            {
                var typed = (StructType)type;
                
                LooseTreeItemViewModel structItem = new() { Text = "Struct" };

                for (int i = 0; i < typed.MemberTypes.Length; i++)
                {
                    LooseTreeItemViewModel memberItem = new() { Text = $"Member {i}" };
                    FormatValue(memberItem, typed.MemberTypes[i], ref byteSpan);
                    structItem.Items.Add(memberItem);
                }

                item.Items.Add(structItem);
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
}
