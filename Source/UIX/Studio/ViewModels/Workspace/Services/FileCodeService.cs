using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Avalonia.Threading;
using Bridge.CLR;
using DynamicData;
using DynamicData.Binding;
using Message.CLR;
using ReactiveUI;
using Runtime.ViewModels.Shader;
using Runtime.ViewModels.Traits;
using Studio.Services;
using Studio.Utils;
using Studio.ViewModels.Code;
using Studio.ViewModels.Setting;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Workspace.Services;

public class FileCodeService : ReactiveObject, IFileCodeService, IBridgeListener
{
    /// <summary>
    /// Has indexing finished?
    /// </summary>
    public bool Indexed
    {
        get => _indexed;
        set => this.RaiseAndSetIfChanged(ref _indexed, value);
    }
    
    /// <summary>
    /// All files
    /// </summary>
    public ObservableCollection<CodeFileViewModel> Files { get; } = new();

    /// <summary>
    /// Target property
    /// </summary>
    public IPropertyViewModel Property
    {
        set
        {
            Destruct();
            _workspaceViewModel = value.GetWorkspace();
            OnWorkspaceChanged();
        }
    }

    /// <summary>
    /// Invoked on workspace changes
    /// </summary>
    private void OnWorkspaceChanged()
    {
        // We must have a valid process assigned
        if (_workspaceViewModel?.Connection?.Application?.Process is not {} process)
        {
            return;
        }

        // Get properties and services
        _shaderCollectionViewModel = _workspaceViewModel.PropertyCollection.GetProperty<IShaderCollectionViewModel>();
        _shaderCodeService = _workspaceViewModel.PropertyCollection.GetService<IShaderCodeService>();
        
        // We must have a settings service
        if (ServiceRegistry.Get<ISettingsService>() is not { ViewModel: { } settingViewModel })
        {
            Studio.Logging.Error("Failed to get settings service");
            return;
        }
        
        // Get application settings
        ApplicationSettingViewModel appSettings = settingViewModel
            .GetItemOrAdd<ApplicationListSettingViewModel>()
            .GetProcessOrAdd(process);
        
        // Configure symbol search path
        var sourceSettings = appSettings.GetItemOrAdd<SourceSettingViewModel>();

        // No configured directories? No indexing
        if (sourceSettings.SourceDirectories.Count == 0)
        {
            return;
        }

        // Index from the settings and subscribe afterwards
        Studio.Logging.Info($"Indexing {sourceSettings.SourceDirectories.Count} source directories");
        IndexRecursive(sourceSettings).ContinueWith(_ => StartPooling());

        // Start the watchers
        CreateWatchers(sourceSettings);
    }

    /// <summary>
    /// Create all source watchers
    /// </summary>
    private void CreateWatchers(SourceSettingViewModel settings)
    {
        foreach (string directory in settings.SourceDirectories)
        {
            // Create watcher
            FileSystemWatcher watcher = new(directory)
            {
                NotifyFilter = NotifyFilters.FileName | NotifyFilters.LastWrite | NotifyFilters.Size,
                IncludeSubdirectories = settings.IndexSubFolders,
                Filter = "*.*"
            };
            
            // Bind
            watcher.Created += OnFileEvent;
            watcher.Changed += OnFileEvent;
            watcher.Deleted += OnFileEvent;
            watcher.Renamed += OnFileEvent;
            watcher.EnableRaisingEvents = true;

            // OK
            _fileRootWatches.Add(directory, watcher);
        }
    }

    /// <summary>
    /// Invoked on file events
    /// </summary>
    private void OnFileEvent(object sender, FileSystemEventArgs e)
    {
        if (_fileWatchEvents.TryGetValue(e.FullPath, out var functor))
        {
            functor.Invoke();
        }
    }

    /// <summary>
    /// Instantiate a file contents
    /// </summary>
    public void InstantiateWithWatch(CodeFileViewModel codeFileViewModel)
    {
        // Already instantiated?
        if (!string.IsNullOrEmpty(codeFileViewModel.Contents))
        {
            return;
        }

        // Common instantiation
        Action instantiate = async () =>
        {
            for (int i = 0; i < _instantiationAttempts; i++)
            {
                try
                {
                    // Read and catch
                    string contents = File.ReadAllText(codeFileViewModel.Filename);
                    
                    // Always populate from the main thread
                    if (Dispatcher.UIThread.CheckAccess())
                    {
                        codeFileViewModel.Contents = contents;
                    }
                    else
                    {
                        await Dispatcher.UIThread.InvokeAsync(() => codeFileViewModel.Contents = contents);
                    }
                    break;
                }
                catch (Exception)
                {
                    // Failed due to the external process still using it
                }

                // Wait until the next try
                await Task.Delay(TimeSpan.FromMilliseconds(_instantiationAttemptWaitTimeMS));
            }
        };
        
        // Initial instantiation
        instantiate();
    
        // Watch for new events
        _fileWatchEvents.Add(codeFileViewModel.Filename, () =>
        {
            instantiate();
        });
    }

    /// <summary>
    /// Handle incoming messages
    /// </summary>
    public void Handle(ReadOnlyMessageStream streams, uint count)
    {
        if (!streams.GetSchema().IsOrdered())
        {
            return;
        }

        // Create view
        var view = new OrderedMessageView(streams);

        // Consume all messages
        foreach (OrderedMessage message in view)
        {
            switch (message.ID)
            {
                case ObjectStatesMessage.ID:
                {
                    var typed = message.Get<ObjectStatesMessage>();

                    // Already accounted for latest?
                    if (_shaderObjectStateHead >= typed.shaderUIDHead)
                    {
                        break;
                    }

                    // Captured state
                    ObjectStatesMessage.FlatInfo flat = typed.Flat;
                    uint lastStateHead = _shaderObjectStateHead;

                    // Start indexing all the added shaders
                    Dispatcher.UIThread.InvokeAsync(() =>
                    {
                        for (uint i = lastStateHead; i < flat.shaderUIDHead; i++)
                        {
                            if (_shaderCollectionViewModel?.GetOrAddShader(i) is { } shaderViewModel)
                            {
                                // Let the code service handle it
                                // Fully deferred for interactivity
                                _shaderCodeService?.EnqueueShaderContents(shaderViewModel, true);
                            
                                // Subscribe to indexed files
                                shaderViewModel.FileViewModels.ToObservableChangeSet()
                                    .AsObservableList()
                                    .Connect()
                                    .OnItemAdded(x => IndexShaderFile(shaderViewModel, x))
                                    .Subscribe();
                            }
                        }
                    });
                    
                    // Set new head
                    _shaderObjectStateHead = typed.shaderUIDHead;
                    break;
                }
            }
        }
    }

    /// <summary>
    /// Index a given shader file
    /// </summary>
    private void IndexShaderFile(ShaderViewModel shaderViewModel, ShaderFileViewModel shaderFileViewModel)
    {
        // Try to match it against indexed
        if (FindBestMatchFile(shaderFileViewModel) is not { } codeFileViewModel)
        {
            string message = $"Failed to index shader file {shaderFileViewModel.Filename} against sources";
            if (_indexingMessageSet.Add(message))
            {
                Studio.Logging.Warning(message);
            }
            return;
        }
        
        // Find the mapping
        CodeFileShaderMappingSetViewModel setViewModel = GetOrAddMappingSet(codeFileViewModel);
        
        // Add mapping
        setViewModel.Mappings.Add(new CodeFileShaderMappingViewModel()
        {
            ShaderViewModel = shaderViewModel,
            ShaderFileViewModel = shaderFileViewModel
        });
    }

    /// <summary>
    /// Get or add a new mapping set
    /// </summary>
    public CodeFileShaderMappingSetViewModel GetOrAddMappingSet(CodeFileViewModel codeFileViewModel)
    {
        if (_mappingSetLookup.TryGetValue(codeFileViewModel, out var mappingSet))
        {
            return mappingSet;
        }
        
        _mappingSetLookup.Add(codeFileViewModel, mappingSet = new CodeFileShaderMappingSetViewModel());
        return mappingSet;
    }

    /// <summary>
    /// Find the best matching file from a shader file
    /// </summary>
    private CodeFileViewModel? FindBestMatchFile(ShaderFileViewModel shaderFileViewModel)
    {
        string searchPath = PathUtils.StandardizePartialPath(shaderFileViewModel.Filename);
        while (!string.IsNullOrEmpty(searchPath))
        {
            // Has candidate?
            if (_fileLookup.TryGetValue(searchPath, out CodeFileViewModel? file))
            {
                return file;
            }
            
            searchPath = PathUtils.RemoveLeadingDirectory(searchPath);
        }

        // Nothing
        return null;
    }

    /// <summary>
    /// Start the bus pooling
    /// </summary>
    private void StartPooling()
    {
        // Start pooling timer
        _poolTimer = new DispatcherTimer(TimeSpan.FromSeconds(1), DispatcherPriority.Background, OnPoolEvent);
        _poolTimer.Start();
        
        // Register internal listeners
        _workspaceViewModel?.Connection?.Bridge?.Register(this);
    }

    /// <summary>
    /// Invoked on pooling
    /// </summary>
    private void OnPoolEvent(object? sender, EventArgs e)
    {
        // Just request the latest states
        if (_workspaceViewModel?.Connection?.GetSharedBus() is { } bus)
        {
            bus.Add<GetObjectStatesMessage>();
        }
    }

    /// <summary>
    /// Invoked on teardown
    /// </summary>
    public void Destruct()
    {
        _poolTimer?.Stop();
        _workspaceViewModel?.Connection?.Bridge?.Deregister(this);
    }

    /// <summary>
    /// Async indexer
    /// </summary>
    public async Task IndexRecursive(SourceSettingViewModel settings, CancellationToken token = default)
    {
        Stack<string> worklist = new();

        // Split all extensions
        string[] extensions = settings.Extensions.Split(",");
        
        // Append supplied roots
        foreach (string directory in settings.SourceDirectories)
        {
            worklist.Push(directory);
        }

        // Keep processing
        while (worklist.Count > 0)
        {
            string item = worklist.Pop();
            token.ThrowIfCancellationRequested();
            
            // Push subdirectories if requested
            if (settings.IndexSubFolders)
            {
                string[]? subDirectories = null;
                try
                {
                    subDirectories = Directory.GetDirectories(item);
                }
                catch
                {
                    // ignore
                }

                if (subDirectories != null)
                {
                    foreach (string directory in subDirectories)
                    {
                        worklist.Push(directory);
                    }
                }
            }

            // Find files
            // TODO: Use a newer API to avoid manual filtering
            string[]? files = null;
            try
            {
                files = Directory.GetFiles(item, "*");
            }
            catch
            {
                // ignore
            }

            // Append all files
            if (files != null)
            {
                foreach (string file in files)
                {
                    // Filter against accepted extensions
                    if (extensions.Length != 0 && !extensions.Any(x => file.EndsWith(x)))
                    {
                        continue;
                    }
                    
                    CodeFileViewModel codeFileViewModel = new()
                    {
                        Filename = file
                    };
                    
                    Files.Add(codeFileViewModel);
                    InsertFileHierarchyNaive(file, codeFileViewModel);
                }
            }

            await Task.Yield();
        }
        
        // Treat as indexed
        Indexed = true;
    }

    /// <summary>
    /// An exceptionally naive hierarchy inserter
    /// </summary>
    private void InsertFileHierarchyNaive(string file, CodeFileViewModel codeFileViewModel)
    {
        file = PathUtils.StandardizePartialPath(file);
        
        // This is obviously "naive", hence the name
        // It's a start
        while (true)
        {
            // Ignore existing keys
            _fileLookup.TryAdd(file, codeFileViewModel);
            
            // Otherwise, move the base up
            int separateIt = file.IndexOf('/');
            if (separateIt == -1)
            {
                return;
            }
            
            file = file.Substring(separateIt + 1);
        }
    }

    /// <summary>
    /// Internal pooling timer
    /// </summary>
    private DispatcherTimer? _poolTimer;

    /// <summary>
    /// Internal lookup
    /// </summary>
    private Dictionary<string, CodeFileViewModel> _fileLookup = new();

    /// <summary>
    /// Internal mapping lookup
    /// </summary>
    private Dictionary<CodeFileViewModel, CodeFileShaderMappingSetViewModel> _mappingSetLookup = new();
    
    /// <summary>
    /// All watchers
    /// </summary>
    private Dictionary<string, FileSystemWatcher> _fileRootWatches = new();

    /// <summary>
    /// All watched file events
    /// </summary>
    private Dictionary<string, Action> _fileWatchEvents = new();

    /// <summary>
    /// Internal view model
    /// </summary>
    private IWorkspaceViewModel? _workspaceViewModel;
    
    /// <summary>
    /// Workspace shader collection
    /// </summary>
    private IShaderCollectionViewModel? _shaderCollectionViewModel;

    /// <summary>
    /// Workspace code service
    /// </summary>
    private IShaderCodeService? _shaderCodeService;

    /// <summary>
    /// Last processed state head
    /// </summary>
    private uint _shaderObjectStateHead = 0;

    /// <summary>
    /// The number of attempts to try to read the file
    /// </summary>
    private static readonly uint _instantiationAttempts = 4;
    
    /// <summary>
    /// The wait time between each attempt
    /// </summary>
    private static readonly uint _instantiationAttemptWaitTimeMS = 100;

    /// <summary>
    /// Deduplication set
    /// </summary>
    private HashSet<string> _indexingMessageSet = new();

    /// <summary>
    /// Internal indexing state
    /// </summary>
    private bool _indexed;
}
