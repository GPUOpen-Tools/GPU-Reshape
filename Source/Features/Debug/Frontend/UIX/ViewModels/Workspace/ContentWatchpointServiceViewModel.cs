using System;
using System.Collections.Generic;
using System.Reactive.Disposables;
using DynamicData;
using DynamicData.Binding;
using GRS.Features.Debug.UIX.ViewModels;
using GRS.Features.Debug.UIX.ViewModels.Utils;
using ReactiveUI;
using Runtime.ViewModels.Traits;
using Studio.Models.Workspace.Listeners;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Shader;

namespace GRS.Features.Debug.UIX.Workspace;

public class ContentWatchpointServiceViewModel : ReactiveObject, IDestructableObject
{
    /// <summary>
    /// Content we're binding for
    /// </summary>
    public ITextualContent Content
    {
        get => _content;
        set => this.RaiseAndSetIfChanged(ref _content, value);
    }

    /// <summary>
    /// Collection we're binding from
    /// </summary>
    public WatchpointCollectionViewModel WatchpointCollectionViewModel
    {
        get => _watchpointCollectionViewModel;
        set => this.RaiseAndSetIfChanged(ref _watchpointCollectionViewModel, value);
    }

    /// <summary>
    /// Bind all watchpoints
    /// </summary>
    public void Bind()
    {
        _watchpointCollectionViewModel.SourceBindings
            .ToObservableChangeSet()
            .OnItemAdded(OnSourceAdded)
            .OnItemRemoved(OnSourceRemoved)
            .Subscribe();
        
        _watchpointCollectionViewModel.TextualBindings
            .ToObservableChangeSet()
            .OnItemAdded(OnTextualAdded)
            .OnItemRemoved(OnBindingRemoved)
            .Subscribe();
    }

    /// <summary>
    /// Invoked on item adds
    /// </summary>
    private void OnSourceAdded(WatchpointViewModelSourceBinding binding)
    {
        WatchpointMappingUtils.SubscribeInstructionLineMapping(Content, binding.WatchpointViewModel.Disposable, binding.Source.Mapping, associationViewModel =>
        {
            // Create source key
            var key = Tuple.Create(
                binding.WatchpointViewModel,
                associationViewModel.Location!.Value.Line,
                associationViewModel.Location!.Value.Column
            );

            // Ignore source-wise duplicates
            if (!_sourceBindingSet.Add(key))
            {
                return;
            }

            // Create object
            WatchpointSourceObject sourceObject = new()
            {
                Content = "Watchpoint",
                DetailViewModel = binding.WatchpointViewModel,
                Segment = new ShaderSourceSegment
                {
                    Location = associationViewModel.Location!.Value
                }
            };

            // Register source object
            Content.MarkerCanvasViewModel.SourceObjects.Add(sourceObject);
            
            // Remove on watchpoint disposing
            binding.WatchpointViewModel.Disposable.Add(Disposable.Create(() =>
            {
                Content.MarkerCanvasViewModel.SourceObjects.Remove(sourceObject);
            }));
            
            // Always select by default
            Content.SelectedTextualSourceObject = sourceObject;
            Content.MarkerCanvasViewModel.DetailCommand?.Execute(sourceObject);
        });
    }

    /// <summary>
    /// Invoked on item removed
    /// </summary>
    private void OnSourceRemoved(WatchpointViewModelSourceBinding binding)
    {
        // Slow remove
        _sourceBindingSet.RemoveWhere(x => x.Item1 == binding.WatchpointViewModel);

        // Remove source object
        if (_sourceObjects.TryGetValue(binding, out WatchpointSourceObject? sourceObject))
        {
            Content.MarkerCanvasViewModel.SourceObjects.Remove(sourceObject);
            _sourceObjects.Remove(binding);
        }
    }

    /// <summary>
    /// Invoked on item adds
    /// </summary>
    private void OnTextualAdded(WatchpointViewModelTextualBinding binding)
    {
        // Create textual key
        var key = Tuple.Create(
            binding.WatchpointViewModel,
            binding.LineBase0
        );

        // Ignore textual-wise duplicates
        if (!_textualBindingSet.Add(key))
        {
            return;
        }

        // Create object
        WatchpointSourceObject sourceObject = new()
        {
            Content = "Watchpoint",
            DetailViewModel = binding.WatchpointViewModel,
            Segment = new ShaderSourceSegment
            {
                Location = new ShaderLocation()
                {
                    Line = binding.LineBase0
                }
            }
        };

        // Register source object
        Content.MarkerCanvasViewModel.SourceObjects.Add(sourceObject);
        
        // Remove on watchpoint disposing
        binding.WatchpointViewModel.Disposable.Add(Disposable.Create(() =>
        {
            Content.MarkerCanvasViewModel.SourceObjects.Remove(sourceObject);
        }));
        
        // Always select by default
        Content.SelectedTextualSourceObject = sourceObject;
        Content.MarkerCanvasViewModel.DetailCommand?.Execute(sourceObject);
    }

    /// <summary>
    /// Invoked on item removed
    /// </summary>
    private void OnBindingRemoved(WatchpointViewModelTextualBinding binding)
    {
        // Slow remove
        _sourceBindingSet.RemoveWhere(x => x.Item1 == binding.WatchpointViewModel);

        // Remove source object
        if (_sourceObjects.TryGetValue(binding, out WatchpointSourceObject? sourceObject))
        {
            Content.MarkerCanvasViewModel.SourceObjects.Remove(sourceObject);
            _sourceObjects.Remove(binding);
        }
    }

    /// <summary>
    /// Internal content
    /// </summary>
    private ITextualContent _content;
    
    /// <summary>
    /// Internal watchpoint collection
    /// </summary>
    private WatchpointCollectionViewModel _watchpointCollectionViewModel;

    /// <summary>
    /// All source binding sets
    /// </summary>
    private HashSet<Tuple<WatchpointViewModel, int, int>> _sourceBindingSet = new();

    /// <summary>
    /// All textual binding sets
    /// </summary>
    private HashSet<Tuple<WatchpointViewModel, int>> _textualBindingSet = new();

    /// <summary>
    /// All source objects
    /// </summary>
    private Dictionary<object, WatchpointSourceObject> _sourceObjects = new();
}
